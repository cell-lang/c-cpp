#include "lib.h"
#include "extern.h"


int64 from_utf8(const char *input, OBJ *output) {
  uint32 idx = 0;
  for (uint32 count=0 ; ; count++) {
    unsigned char ch = input[idx++];
    if (ch == 0)
      return count;
    uint32 val;
    int size;
    if (ch >> 7 == 0) { // 0xxxxxxx
      size = 0;
      val = ch;
    }
    else if (ch >> 5 == 6) { // 110xxxxx  10xxxxxx
      val = (ch & 0x1F) << 6;
      size = 1;
    }
    else if (ch >> 4 == 0xE) { // 1110xxxx  10xxxxxx  10xxxxxx
      val = (ch & 0xF) << 12;
      size = 2;
    }
    else if (ch >> 3 == 0x1E) { // 11110xxx  10xxxxxx  10xxxxxx  10xxxxxx
      val = (ch & 0xF) << 18;
      size = 3;
    }
    else
      return -idx;

    for (int i=0 ; i < size ; i++) {
      ch = input[idx++];
      if (ch >> 6 != 2)
        return -idx;
      val |= (ch & 0x3F) << (6 * (size - i - 1));
    }

    if (output != NULL)
      output[count] = make_int(val);
  }
}

OBJ str_to_obj(const char *c_str) {
  OBJ raw_str_obj;

  if (c_str[0] == 0) {
    raw_str_obj = make_empty_seq();
  }
  else {
    int64 size = from_utf8(c_str, NULL);
    SEQ_OBJ *raw_str = new_seq(size);
    from_utf8(c_str, raw_str->buffer);
    raw_str_obj = make_seq(raw_str, size);
  }

  return make_tag_obj(symb_id_string, raw_str_obj);
}

////////////////////////////////////////////////////////////////////////////////

int64 to_utf8(const OBJ *chars, uint32 len, char *output) {
  int offset = 0;
  for (uint32 i=0 ; i < len ; i++) {
    int64 cp = get_int(chars[i]);
    if (cp < 0x80) {
      if (output != NULL)
        output[offset] = cp;
      offset++;
    }
    else if (cp < 0x800) {
      if (output != NULL) {
        output[offset]   = 0xC0 | (cp >> 6);
        output[offset+1] = 0x80 | (cp & 0x3F);
      }
      offset += 2;
    }
    else if (cp < 0x10000) {
      if (output != NULL) {
        output[offset]   = 0xE0 | (cp >> 12);
        output[offset+1] = 0x80 | ((cp >> 6) & 0x3F);
        output[offset+2] = 0x80 | (cp & 0x3F);
      }
      offset += 3;
    }
    else {
      if (output != NULL) {
        output[offset]   = 0xF0 | (cp >> 18);
        output[offset+1] = 0x80 | ((cp >> 12) & 0x3F);
        output[offset+2] = 0x80 | ((cp >> 6) & 0x3F);
        output[offset+3] = 0x80 | (cp & 0x3F);
      }
      offset += 4;
    }
  }
  if (output != NULL)
    output[offset] = 0;
  return offset + 1;
}

int64 utf8_size(OBJ str_obj) {
  OBJ raw_str_obj = get_inner_obj(str_obj);
  if (is_empty_seq(raw_str_obj))
    return 1;
  OBJ *seq_buffer = get_seq_elts_ptr(raw_str_obj);
  uint32 len = get_seq_length(raw_str_obj);
  return to_utf8(seq_buffer, len, NULL);
}

void obj_to_str(OBJ str_obj, char *buffer, uint32 size) {
  OBJ raw_str_obj = get_inner_obj(str_obj);

  if (!is_empty_seq(raw_str_obj)) {
    OBJ *seq_buffer = get_seq_elts_ptr(raw_str_obj);
    uint32 len = get_seq_length(raw_str_obj);
    int64 min_size = to_utf8(seq_buffer, len, NULL);
    if (size < min_size)
      internal_fail();
    to_utf8(seq_buffer, len, buffer);
  }
  else
    buffer[0] = '\0';
}

char *obj_to_byte_array(OBJ byte_seq_obj, uint32 &size) {
  if (is_empty_seq(byte_seq_obj)) {
    size = 0;
    return NULL;
  }

  uint32 len = get_seq_length(byte_seq_obj);
  OBJ *elems = get_seq_elts_ptr(byte_seq_obj);
  char *buffer = new_byte_array(len);
  for (uint32 i=0 ; i < len ; i++) {
    long long val = get_int(elems[i]);
    assert(val >= 0 && val <= 255);
    buffer[i] = (char) val;
  }
  size = len;
  return buffer;
}

char *obj_to_str(OBJ str_obj) {
  uint32 size = utf8_size(str_obj);
  char *buffer = new_byte_array(size);
  obj_to_str(str_obj, buffer, size);
  return buffer;
}

////////////////////////////////////////////////////////////////////////////////

bool str_ord(const char *str1, const char *str2) {
  return strcmp(str1, str2) > 0;
}

typedef std::map<const char *, uint32, bool(*)(const char *, const char *)> str_idx_map_type;

str_idx_map_type str_to_symb_map(str_ord);
//## THESE STRINGS ARE NEVER CLEANED UP. NOT MUCH OF A PROBLEM IN PRACTICE, BUT STILL A BUG...
std::vector<const char *> dynamic_symbs_strs;

const char *symb_to_raw_str(uint16 symb_id) {
  uint32 count = embedded_symbs_count();
  if (symb_id < count)
    return symb_repr(symb_id);
  else
    return dynamic_symbs_strs[symb_id - count];
}

OBJ to_str(OBJ obj) {
  return str_to_obj(symb_to_raw_str(get_symb_id(obj)));
}

uint16 lookup_symb_id(const char *str_, uint32 len) {
  uint32 count = embedded_symbs_count();

  if (str_to_symb_map.size() == 0)
    for (uint32 i=0 ; i < count ; i++)
      str_to_symb_map[symb_repr(i)] = i;

  char *str = strndup(str_, len);

  str_idx_map_type::iterator it = str_to_symb_map.find(str);
  if (it != str_to_symb_map.end()) {
    free(str);
    return it->second;
  }

  uint32 next_symb_id = count + dynamic_symbs_strs.size();
  if (next_symb_id > 0xFFFF)
    impl_fail("Exceeded maximum permitted number of symbols (= 2^16)");
  dynamic_symbs_strs.push_back(str);
  str_to_symb_map[str] = next_symb_id;
  return next_symb_id;
}

OBJ to_symb(OBJ obj) {
  char *str = obj_to_str(obj);
  uint32 len = strlen(str);
  uint16 symb_id = lookup_symb_id(str, len);
  return make_symb(symb_id);
}

OBJ extern_str_to_symb(const char *str) {
  //## CHECK THAT IT'S A VALID SYMBOL, AND THAT IT'S AMONG THE "STATIC" ONES
  return make_symb(lookup_symb_id(str, strlen(str)));
}
