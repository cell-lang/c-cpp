#include "lib.h"
#include "extern.h"


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
  uint32 len = get_seq_length(chars);
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

  size = get_seq_length(byte_seq_obj);

  OBJ_TYPE type = get_physical_type(byte_seq_obj);

  if (type == TYPE_NE_SEQ_UINT8 | type == TYPE_NE_SLICE_UINT8)
    return get_seq_elts_ptr_uint8(byte_seq_obj);

  uint8 *buffer = new_uint8_array(size);
  for (int i=0 ; i < size ; i++)
    buffer[i] = (uint8) get_int_at(byte_seq_obj, i);
  return buffer;
}

char *obj_to_str(OBJ str_obj) {
  uint32 size = utf8_size(str_obj);
  char *buffer = new_byte_array(size);
  obj_to_str(str_obj, buffer, size);
  return buffer;
}

////////////////////////////////////////////////////////////////////////////////

static uint32 string_hashcode(const char *str, uint32 len) {
  uint32 hcode = 0;
  for (int i=0 ; i < len ; i++)
    hcode = 31 * hcode + str[i];
  return hcode;
}

static bool string_eq(const char *str1, const char *str2, uint32 len) {
  for (int i=0 ; i < len ; i++)
    if (str1[i] != str2[i])
      return false;
  return str1[len] == '\0';
}

////////////////////////////////////////////////////////////////////////////////

typedef struct {
  const char *str;
  uint16 id;
} SYMBOL_HASHTABLE_ENTRY;

typedef struct {
  SYMBOL_HASHTABLE_ENTRY *entries;
  uint32 capacity;
  uint32 count;
} SYMBOL_HASHTABLE;


inline uint32 hashtable_step(uint32 hashcode) {
  // Using an odd step size should guarantee that all slots are eventually visited
  //## I'M STILL NOT ENTIRELY SURE ABOUT THIS. CHECK!
  return (hashcode & 0xF) | 1;
}

SYMBOL_HASHTABLE_ENTRY *symbol_hashtable_lookup(SYMBOL_HASHTABLE *hashtable, const char *str, uint32 len) {
  uint32 hcode = string_hashcode(str, len);
  uint32 step = hashtable_step(hcode);

  SYMBOL_HASHTABLE_ENTRY *entries = hashtable->entries;
  uint32 capacity = hashtable->capacity;
  if (capacity == 0)
    return NULL;

  uint32 idx = hcode % capacity;
  for ( ; ; ) {
    SYMBOL_HASHTABLE_ENTRY *entry = entries + idx;
    const char *entry_str = entry->str;
    if (entry_str == NULL)
      return NULL;
    if (string_eq(entry_str, str, len))
      return entry;
    idx = (idx + step) % capacity;
  }
}

void symbol_hashtable_insert_unsafe(SYMBOL_HASHTABLE *hashtable, uint16 id, const char *str) {
  // Assuming the hashtable is not full and the symbol has not been inserted yet
  assert(hashtable->count < hashtable->capacity);
  assert(symbol_hashtable_lookup(hashtable, str, strlen(str)) == NULL);

  uint32 hcode = string_hashcode(str, strlen(str));
  uint32 step = hashtable_step(hcode);

  SYMBOL_HASHTABLE_ENTRY *entries = hashtable->entries;
  uint32 capacity = hashtable->capacity;
  hashtable->count++;

  uint32 idx = hcode % capacity;
  for ( ; ; ) {
    SYMBOL_HASHTABLE_ENTRY *entry = entries + idx;
    if (entry->str == NULL) {
      entry->str = str;
      entry->id = id;
      return;
    }
    idx = (idx + step) % capacity;
  }
}

void symbol_hashtable_insert(SYMBOL_HASHTABLE *hashtable, uint16 id, const char *str) {
  // Assuming the symbol is not there yet
  assert(symbol_hashtable_lookup(hashtable, str, strlen(str)) == NULL);

  if (hashtable->entries == NULL) {
    uint32 mem_size = 1024 * sizeof(SYMBOL_HASHTABLE_ENTRY);
    hashtable->entries = (SYMBOL_HASHTABLE_ENTRY *) alloc_static_block(mem_size);
    memset(hashtable->entries, 0, mem_size);
    hashtable->capacity = 1024;
    hashtable->count = 0;
  }
  else if (hashtable->capacity < 2 * hashtable->count) {
    SYMBOL_HASHTABLE_ENTRY *entries = hashtable->entries;
    uint32 capacity = hashtable->capacity;

    uint32 mem_size = 2 * capacity * sizeof(SYMBOL_HASHTABLE_ENTRY);
    hashtable->entries = (SYMBOL_HASHTABLE_ENTRY *) alloc_static_block(mem_size);
    memset(hashtable->entries, 0, mem_size);
    hashtable->capacity = 2 * capacity;
    hashtable->count = 0;

    for (int i=0 ; i < capacity ; i++) {
      SYMBOL_HASHTABLE_ENTRY *entry = entries + i;
      if (entry->str != NULL) {
        symbol_hashtable_insert_unsafe(hashtable, entry->id, entry->str);
        assert(symbol_hashtable_lookup(hashtable, entry->str, strlen(entry->str))->id == entry->id);
      }
    }

    release_static_block(entries, capacity * sizeof(SYMBOL_HASHTABLE_ENTRY));
  }

  symbol_hashtable_insert_unsafe(hashtable, id, str);
  assert(symbol_hashtable_lookup(hashtable, str, strlen(str))->id == id);
}

////////////////////////////////////////////////////////////////////////////////

static SYMBOL_HASHTABLE symbols_hashtable;
static const char **dynamic_symbs_strs;
static uint32 dynamic_symbs_strs_capacity;
static uint32 dynamic_symbs_count;


const char *symb_to_raw_str(uint16 symb_id) {
  uint32 count = embedded_symbs_count();
  assert(symb_id < count + dynamic_symbs_count);
  if (symb_id < count)
    return symb_repr(symb_id);
  else
    return dynamic_symbs_strs[symb_id - count];
}

uint16 lookup_symb_id(const char *str, uint32 len) {
  uint32 count = embedded_symbs_count();

  if (symbols_hashtable.capacity == 0)
    for (uint32 i=0 ; i < count ; i++) {
      const char *symb_str = symb_repr(i);
      uint32 len = strlen(symb_str);
      assert(symbol_hashtable_lookup(&symbols_hashtable, symb_str, len) == NULL);
      symbol_hashtable_insert(&symbols_hashtable, i, symb_str);
      assert(symbol_hashtable_lookup(&symbols_hashtable, symb_str, strlen(symb_str))->id == i);
    }

  SYMBOL_HASHTABLE_ENTRY *entry = symbol_hashtable_lookup(&symbols_hashtable, str, len);
  if (entry != NULL)
    return entry->id;

  uint32 next_symb_id = count + dynamic_symbs_count;
  if (next_symb_id > 0xFFFF)
    impl_fail("Exceeded maximum permitted number of symbols (= 2^16)");

  if (dynamic_symbs_count == dynamic_symbs_strs_capacity) {
    uint32 new_capacity = dynamic_symbs_strs_capacity != 0 ? 2 * dynamic_symbs_strs_capacity : 256;
    const char **new_strs = (const char **) alloc_static_block(new_capacity * sizeof(const char *));
    memcpy(new_strs, dynamic_symbs_strs, dynamic_symbs_count * sizeof(const char *));
    dynamic_symbs_strs = new_strs;
    dynamic_symbs_strs_capacity = new_capacity;
  }

  char *str_copy = (char *) alloc_static_block(len + 1);
  memcpy(str_copy, str, len);
  str_copy[len] = '\0';

  dynamic_symbs_strs[dynamic_symbs_count++] = str_copy;
  symbol_hashtable_insert(&symbols_hashtable, next_symb_id, str_copy);

  return next_symb_id;
}
