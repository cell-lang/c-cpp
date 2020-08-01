inline uint8 inline_uint8_at(uint64 packed_elts, uint32 idx) {
  return (packed_elts >> (8 * idx)) & 0xFF;
}

inline int16 inline_int16_at(uint64 packed_elts, uint32 idx) {
  return (int16) ((packed_elts >> (16 * idx)) & 0xFFFF);
}

inline int32 inline_int32_at(uint64 packed_elts, uint32 idx) {
  return (int32) ((packed_elts >> (32 * idx)) & 0xFFFFFFFF);
}

inline uint64 inline_uint8_init_at(uint64 packed_elts, uint32 idx, uint8 value) {
  assert(idx >= 0 & idx <= 8);
  assert((packed_elts >> (8 * idx)) == 0);
  uint64 updated_packed_elts = packed_elts | (((uint64) value) << (8 * idx));
  assert(idx == 7 || (updated_packed_elts >> (8 * (idx + 1))) == 0);
  assert(inline_uint8_at(updated_packed_elts, idx) == value);
  for (int i=0 ; i < idx ; i++)
    assert(inline_uint8_at(updated_packed_elts, i) == inline_uint8_at(packed_elts, i));
  for (int i = idx + 1 ; i < 8 ; i++)
    assert(inline_uint8_at(updated_packed_elts, i) == 0);
  return updated_packed_elts;
}

inline uint64 inline_int16_init_at(uint64 packed_elts, uint32 idx, int16 value) {
  assert(idx >= 0 & idx <= 4);
  assert((packed_elts >> (16 * idx)) == 0);
  uint64 updated_packed_elts = packed_elts | ((((uint64) value) & 0xFFFF) << (16 * idx));
  assert(idx == 3 || (updated_packed_elts >> (16 * (idx + 1))) == 0);
  assert(inline_int16_at(updated_packed_elts, idx) == value);
  for (int i=0 ; i < idx ; i++)
    assert(inline_int16_at(updated_packed_elts, i) == inline_int16_at(packed_elts, i));
  for (int i = idx + 1 ; i < 4 ; i++)
    assert(inline_int16_at(updated_packed_elts, i) == 0);
  return updated_packed_elts;
}

inline uint64 inline_int32_init_at(uint64 packed_elts, uint32 idx, int32 value) {
  assert(idx == 0 | idx == 1);
  assert((packed_elts >> (32 * idx)) == 0);
  uint64 updated_packed_elts = packed_elts | ((((uint64) value) & 0xFFFFFFFF) << (32 * idx));
  assert(idx == 1 || (updated_packed_elts >> 32) == 0);
  if (idx == 1)
    assert(inline_int32_at(updated_packed_elts, 0) == inline_int32_at(packed_elts, 0));
  assert(inline_int32_at(updated_packed_elts, idx) == value);
  for (int i=0 ; i < idx ; i++)
    assert(inline_int32_at(updated_packed_elts, i) == inline_int32_at(packed_elts, i));
  for (int i = idx + 1 ; i < 2 ; i++)
    assert(inline_int32_at(updated_packed_elts, i) == 0);
  return updated_packed_elts;
}

inline uint64 inline_uint8_pack(uint8 *array, uint32 size) {
  assert(size <= 8);

  uint64 packed_elts = 0;
  for (int i=0 ; i < size ; i++)
    packed_elts |= ((uint64) array[i]) << (8 * i);
  for (int i=0 ; i < size ; i++)
    assert(inline_uint8_at(packed_elts, i) == array[i]);
  for (int i=size ; i < 8 ; i++)
    assert(inline_uint8_at(packed_elts, i) == 0);
  return packed_elts;
}

inline uint64 inline_int16_pack(int16 *array, uint32 size) {
  assert(size <= 4);

  uint64 packed_elts = 0;
  for (int i=0 ; i < size ; i++)
    packed_elts |= (((uint64) array[i]) & 0xFFFF) << (16 * i);
  for (int i=0 ; i < size ; i++)
    assert(inline_int16_at(packed_elts, i) == array[i]);
  for (int i=size ; i < 4 ; i++)
    assert(inline_int16_at(packed_elts, i) == 0);
  return packed_elts;
}

inline uint64 inline_uint8_slice(uint64 packed_elts, uint32 idx_first, uint32 count) {
  assert(idx_first < 8 & count <= 8 & idx_first + count <= 8);

  uint64 slice = (packed_elts >> (8 * idx_first)) & ((1ULL << (8 * count)) - 1);
  for (int i=0 ; i < count ; i++)
    assert(inline_uint8_at(slice, i) == inline_uint8_at(packed_elts, i + idx_first));
  for (int i=count ; i < 8 ; i++)
    assert(inline_uint8_at(slice, i) == 0);
  return slice;
}

inline uint64 inline_int16_slice(uint64 packed_elts, uint32 idx_first, uint32 count) {
  assert(idx_first < 4 & count <= 4 & idx_first + count <= 4);

  uint64 slice = (packed_elts >> (16 * idx_first)) & ((1ULL << (16 * count)) - 1);
  for (int i=0 ; i < count ; i++)
    assert(inline_int16_at(slice, i) == inline_int16_at(packed_elts, i + idx_first));
  for (int i=count ; i < 4 ; i++)
    assert(inline_int16_at(slice, i) == 0);
  return slice;
}

inline uint64 inline_uint8_concat(uint64 left, uint32 left_len, uint64 right, uint32 right_len) {
  assert(left_len <= 8 & right_len <= 8 & left_len + right_len <= 8);

  uint64 elts = left | (right << (8 * left_len));

  for (int i=0 ; i < left_len ; i++)
    assert(inline_uint8_at(elts, i) == inline_uint8_at(left, i));
  for (int i=0 ; i < right_len ; i++)
    assert(inline_uint8_at(elts, i + left_len) == inline_uint8_at(right, i));
  for (int i = left_len + right_len ; i < 8 ; i++)
    assert(inline_uint8_at(elts, i) == 0);

  return elts;
}

inline uint64 inline_int16_concat(uint64 left, uint32 left_len, uint64 right, uint32 right_len) {
  assert(left_len <= 4 & right_len <= 4 & left_len + right_len <= 4);

  uint64 elts = left | (right << (16 * left_len));

  for (int i=0 ; i < left_len ; i++)
    assert(inline_int16_at(elts, i) == inline_int16_at(left, i));
  for (int i=0 ; i < right_len ; i++)
    assert(inline_int16_at(elts, i + left_len) == inline_int16_at(right, i));
  for (int i = left_len + right_len ; i < 4 ; i++)
    assert(inline_int16_at(elts, i) == 0);

  return elts;
}
