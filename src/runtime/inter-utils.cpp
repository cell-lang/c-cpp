#include "lib.h"


int64 from_utf8(const char *input, int32 *output) {
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
      output[count] = val;
  }
}

OBJ /*owned_*/str_to_obj(const char *c_str) {
  OBJ raw_str_obj;

  if (c_str[0] == 0) {
    raw_str_obj = make_empty_seq();
  }
  else {
    bool is_ascii = true;
    uint32 idx = 0;
    for ( ; ; idx++) {
      uint8 ch = c_str[idx];
      if (ch == 0)
        break;
      if (ch > 127)
        is_ascii = false;
    }

    if (is_ascii) {
      raw_str_obj = build_seq_uint8((uint8 *) c_str, idx);
    }
    else {
      int64 size = from_utf8(c_str, NULL);
      int32 *buffer = new_int32_array(size);
      from_utf8(c_str, buffer);
      raw_str_obj = build_seq_int32(buffer, size);
    }
  }

  return make_tag_obj(symb_id_string, raw_str_obj);
}

////////////////////////////////////////////////////////////////////////////////

int64 to_utf8(OBJ chars, char *output) {
  uint32 len = read_size_field(chars);
  int offset = 0;
  for (uint32 i=0 ; i < len ; i++) {
    int64 cp = get_int_at(chars, i);
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
  OBJ chars_obj = get_inner_obj(str_obj);
  return !is_empty_seq(chars_obj) ? to_utf8(chars_obj, NULL) : 1;
}

void obj_to_str(OBJ str_obj, char *buffer, uint32 size) {
  OBJ chars_obj = get_inner_obj(str_obj);

  if (!is_empty_seq(chars_obj)) {
    int64 min_size = to_utf8(chars_obj, NULL);
    if (size < min_size)
      internal_fail();
    to_utf8(chars_obj, buffer);
  }
  else
    buffer[0] = '\0';
}

uint8 *obj_to_byte_array(OBJ byte_seq_obj, uint32 &size) {
  if (is_empty_seq(byte_seq_obj)) {
    size = 0;
    return NULL;
  }

  size = read_size_field(byte_seq_obj);

  if (get_obj_type(byte_seq_obj) == TYPE_NE_INT_SEQ)
    if (get_int_bits_tag(byte_seq_obj) == INT_BITS_TAG_8 & !is_signed(byte_seq_obj))
      return get_seq_elts_ptr_uint8(byte_seq_obj);

  uint8 *buffer = new_uint8_array(size);
  for (int i=0 ; i < size ; i++)
    buffer[i] = (uint8) get_int_at_unchecked(byte_seq_obj, i);
  return buffer;
}

char *obj_to_str(OBJ str_obj) {
  uint32 size = utf8_size(str_obj);
  char *buffer = new_byte_array(size);
  obj_to_str(str_obj, buffer, size);
  return buffer;
}
