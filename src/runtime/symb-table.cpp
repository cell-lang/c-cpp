#include "lib.h"


//  0         Empty
//  1 - 26    Letter
// 27 - 36    Digit
// 37         Underscore (followed by a digit)
// 38 - 63    Underscore + letter

static uint64 encoded_letter(uint8 ch) {
  return ch - 'a' + 1;
}

static uint64 encoded_underscored_letter(uint8 ch) {
  return ch - 'a' + 38;
}

static uint64 encoded_digit(uint8 ch) {
  return ch - '0' + 27;
}

const uint64 encoded_underscore = 37;

static uint8 decode_chars(uint8 code, uint8 *chars) {
  assert(code > 0 && code < 64);
  if (code <= 26) {
    chars[0] = code - 1 + 'a';
    return 1;
  }
  if (code <= 36) {
    chars[0] = code - 27 + '0';
    return 1;
  }
  if (code == 37) {
    chars[0] = '_';
    return 1;
  }
  chars[0] = '_';
  chars[1] = code - 38 + 'a';
  return 2;
}

////////////////////////////////////////////////////////////////////////////////

// Tightly coupled to read_enc_symb_words() in parsing.cpp
// The two locations have to be kept in lockstep

static int32 decode_symb(const uint64 *enc_symb, int32 enc_len, uint8 *str) {
  int idx = 0;
  for (int i=0 ; i < enc_len ; i++) {
    uint64 enc_word = enc_symb[i];
    for (int shift=54 ; shift >= 0 ; shift -= 6) {
      uint8 code = (enc_word >> shift) & 0x3F;
      if (code > 0)
        idx += decode_chars(code, str + idx);
    }
  }
  str[idx] = '\0';
  return idx;
}

static int32 decoded_symb_length(const uint64 *enc_symb, int32 enc_len) {
  int len = 0;
  for (int i=0 ; i < enc_len ; i++) {
    uint64 enc_word = enc_symb[i];
    for (int shift=54 ; shift >= 0 ; shift -= 6) {
      uint8 code = (enc_word >> shift) & 0x3F;
      if (code > 0) {
        uint8 unused[2];
        len += decode_chars(code, unused);
      }
    }
  }
  return len;
}

static int32 encode_symb(const uint8 *str, uint32 len, uint64 *enc_symb) {
  uint32 idx = 0;
  uint32 enc_len = 0;
  uint64 enc_word = 0;
  for ( ; ; ) {
    if (idx >= len) {
      if (enc_word != 0) {
        if (enc_symb != NULL)
          enc_symb[enc_len] = enc_word;
        enc_len++;
      }
      break;
    }

    uint8 ch = str[idx++];

    if (is_lower(ch)) {
      uint64 code = encoded_letter(ch);
      enc_word = (enc_word << 6) + code;
    }
    else if (is_digit(ch)) {
      uint64 code = encoded_digit(ch);
      enc_word = (enc_word << 6) + code;
    }
    else {
      assert(ch == '_');
      assert(idx < len);
      uint8 next_ch = str[idx];
      if (is_lower(next_ch)) {
        uint64 code = encoded_underscored_letter(next_ch);
        enc_word = (enc_word << 6) + code;
        idx++;
      }
      else {
        assert(is_digit(next_ch));
        enc_word = (enc_word << 6) + encoded_underscore;
      }
    }

    if (enc_word >> 54 != 0) {
      if (enc_symb != NULL)
        enc_symb[enc_len] = enc_word;
      enc_len++;
      enc_word = 0;
    }
  }

#ifndef NDEBUG
  if (enc_symb != NULL && len < 4096) {
    uint8 rec_str[4096];
    assert(decoded_symb_length(enc_symb, enc_len) == len);
    int32 rec_len = decode_symb(enc_symb, enc_len, rec_str);
    assert(rec_len == len);
    for (int i=0 ; i < len ; i++)
      assert(rec_str[i] == str[i]);
  }
#endif

  return enc_len;
}

////////////////////////////////////////////////////////////////////////////////

static uint32 symb_hashcode(const uint64 *enc_symb, uint32 enc_len) {
  uint64 hcode = 0;
  for (int i=0 ; i < enc_len ; i++)
    hcode = 31 * hcode + enc_symb[i];
  return hcode ^ (hcode >> 32);
}

static bool enc_symb_eq(const uint64 *enc_symb_1, const uint64 *enc_symb_2, uint32 len) {
  for (int i=0 ; i < len ; i++)
    if (enc_symb_1[i] != enc_symb_2[i])
      return false;
  return true;
}

////////////////////////////////////////////////////////////////////////////////

typedef struct {
  const uint64 *enc_symb;
  uint32 enc_len;
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

static SYMBOL_HASHTABLE_ENTRY *symbol_hashtable_lookup(SYMBOL_HASHTABLE *hashtable, const uint64 *enc_symb, uint32 enc_len) {
  uint32 hcode = symb_hashcode(enc_symb, enc_len);
  uint32 step = hashtable_step(hcode);

  SYMBOL_HASHTABLE_ENTRY *entries = hashtable->entries;
  uint32 capacity = hashtable->capacity;
  if (capacity == 0)
    return NULL;

  uint32 idx = hcode % capacity;
  for ( ; ; ) {
    SYMBOL_HASHTABLE_ENTRY *entry = entries + idx;
    const uint32 entry_enc_len = entry->enc_len;
    if (entry_enc_len == 0)
      return NULL;
    if (entry_enc_len == enc_len && enc_symb_eq(entry->enc_symb, enc_symb, enc_len))
      return entry;
    idx = (idx + step) % capacity;
  }
}

void symbol_hashtable_insert_unsafe(SYMBOL_HASHTABLE *hashtable, uint16 id, const uint64 *enc_symb, uint32 enc_len) {
  // Assuming the hashtable is not full and the symbol has not been inserted yet
  assert(hashtable->count < hashtable->capacity);
  assert(symbol_hashtable_lookup(hashtable, enc_symb, enc_len) == NULL);

  uint32 hcode = symb_hashcode(enc_symb, enc_len);
  uint32 step = hashtable_step(hcode);

  SYMBOL_HASHTABLE_ENTRY *entries = hashtable->entries;
  uint32 capacity = hashtable->capacity;
  hashtable->count++;

  uint32 idx = hcode % capacity;
  for ( ; ; ) {
    SYMBOL_HASHTABLE_ENTRY *entry = entries + idx;
    if (entry->enc_len == 0) {
      entry->enc_symb = enc_symb;
      entry->enc_len = enc_len;
      entry->id = id;
      return;
    }
    idx = (idx + step) % capacity;
  }
}

// The memory block pointed to by enc_symb must be already allocated in permanent storage
void symbol_hashtable_insert(SYMBOL_HASHTABLE *hashtable, uint16 id, const uint64 *enc_symb, uint32 enc_len) {
  // Assuming the symbol is not there yet
  assert(symbol_hashtable_lookup(hashtable, enc_symb, enc_len) == NULL);

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
      const uint64 *entry_enc_symb = entry->enc_symb;
      if (entry_enc_symb != NULL) {
        symbol_hashtable_insert_unsafe(hashtable, entry->id, entry_enc_symb, entry->enc_len);
        assert(symbol_hashtable_lookup(hashtable, entry_enc_symb, entry->enc_len)->id == entry->id);
      }
    }

    release_static_block(entries, capacity * sizeof(SYMBOL_HASHTABLE_ENTRY));
  }

  symbol_hashtable_insert_unsafe(hashtable, id, enc_symb, enc_len);
  assert(symbol_hashtable_lookup(hashtable, enc_symb, enc_len)->id == id);
}

////////////////////////////////////////////////////////////////////////////////

typedef struct {
  const char **strs;
  uint32 capacity;
  uint32 count;
} DYNAMIC_SYMBOLS;

typedef struct {
  SYMBOL_HASHTABLE hashtable;
  DYNAMIC_SYMBOLS dynamic_symbs;
} SYMBOL_TABLE;


static const char *internal_symb_to_raw_str(SYMBOL_TABLE *symbol_table, uint16 symb_id) {
  uint32 count = embedded_symbs_count();
  assert(symb_id < count + symbol_table->dynamic_symbs.count);
  if (symb_id < count)
    return symb_repr(symb_id);
  else
    return symbol_table->dynamic_symbs.strs[symb_id - count];
}

static void init_symb_hashtable(SYMBOL_TABLE *symbol_table) {
  uint32 count = embedded_symbs_count();
  for (uint32 i=0 ; i < count ; i++) {
    const char *symb_str = symb_repr(i);
    uint32 len = strlen(symb_str);

    uint32 enc_len = encode_symb((const uint8 *) symb_str, len, NULL);
    uint64 *enc_symb = (uint64 *) alloc_eternal_block(sizeof(uint64) * enc_len);
    encode_symb((const uint8 *) symb_str, len, enc_symb);

    assert(symbol_hashtable_lookup(&symbol_table->hashtable, enc_symb, enc_len) == NULL);
    symbol_hashtable_insert(&symbol_table->hashtable, i, enc_symb, enc_len);
    assert(symbol_hashtable_lookup(&symbol_table->hashtable, enc_symb, enc_len)->id == i);
  }
}

static void resize_symb_table(SYMBOL_TABLE *symbol_table) {
  assert(symbol_table->dynamic_symbs.capacity == symbol_table->dynamic_symbs.count);
  uint32 capacity = symbol_table->dynamic_symbs.capacity;
  uint32 new_capacity = capacity != 0 ? 2 * capacity : 256;
  const char **new_strs = (const char **) alloc_static_block(new_capacity * sizeof(const char *));
  memcpy(new_strs, symbol_table->dynamic_symbs.strs, capacity * sizeof(const char *));
  if (symbol_table->dynamic_symbs.strs != NULL)
    release_static_block(symbol_table->dynamic_symbs.strs, capacity * sizeof(const char *));
  symbol_table->dynamic_symbs.strs = new_strs;
  symbol_table->dynamic_symbs.capacity = new_capacity;
}

////////////////////////////////////////////////////////////////////////////////

static uint16 internal_lookup_enc_symb_id(SYMBOL_TABLE *symbol_table, const uint64 *enc_symb, uint32 enc_len) {
  if (symbol_table->hashtable.capacity == 0)
    init_symb_hashtable(symbol_table);

  SYMBOL_HASHTABLE_ENTRY *entry = symbol_hashtable_lookup(&symbol_table->hashtable, enc_symb, enc_len);
  if (entry != NULL)
    return entry->id;

  uint32 dyn_symbs_count = symbol_table->dynamic_symbs.count;

  uint32 next_symb_id = embedded_symbs_count() + dyn_symbs_count;
  if (next_symb_id > 0xFFFF)
    impl_fail("Exceeded maximum permitted number of symbols (= 2^16)");

  if (dyn_symbs_count == symbol_table->dynamic_symbs.capacity)
    resize_symb_table(symbol_table);

  int32 str_len = decoded_symb_length(enc_symb, enc_len);
  uint8 *str_copy = (uint8 *) alloc_eternal_block(str_len + 1);
  decode_symb(enc_symb, enc_len, str_copy);
  symbol_table->dynamic_symbs.strs[dyn_symbs_count] = (const char *) str_copy;
  symbol_table->dynamic_symbs.count = dyn_symbs_count + 1;

  uint64 *enc_symb_copy = (uint64 *) alloc_eternal_block(enc_len * sizeof(uint64));
  memcpy(enc_symb_copy, enc_symb, enc_len * sizeof(uint64));
  symbol_hashtable_insert(&symbol_table->hashtable, next_symb_id, enc_symb_copy, enc_len);

  return next_symb_id;
}

////////////////////////////////////////////////////////////////////////////////

static SYMBOL_TABLE STATIC__symbol_table;


const char *symb_to_raw_str(uint16 symb_id) {
  return internal_symb_to_raw_str(&STATIC__symbol_table, symb_id);
}

uint16 lookup_enc_symb_id(const uint64 *enc_symb, uint32 enc_len) {
  char str[16384];
  decode_symb(enc_symb, enc_len, (uint8 *) str);
  return internal_lookup_enc_symb_id(&STATIC__symbol_table, enc_symb, enc_len);
}

uint16 lookup_symb_id(const char *str, uint32 len) {
  if (len > 16384)
    impl_fail("Symbols cannot be longer that 1024 characters");

  uint64 enc_symb[4096];
  uint32 enc_len = encode_symb((const uint8 *) str, len, enc_symb);
  return internal_lookup_enc_symb_id(&STATIC__symbol_table, enc_symb, enc_len);
}
