inline void append(STREAM &s, OBJ obj) {
  void increase_stream_capacity(STREAM &s);

  assert(s.count <= s.capacity);

  uint32 count = s.count;
  OBJ *buffer = s.buffer;

  if (count == s.capacity)
    increase_stream_capacity(s);

  s.buffer[count] = obj;
  s.count++;
}
