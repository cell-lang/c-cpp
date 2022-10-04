#include "lib.h"
#include "os-interface.h"


OBJ FileRead_P(OBJ filename) {
  char *fname = obj_to_str(filename);
  int size;
  uint8 *data = file_read(fname, size);

  if (size == -1)
    return make_symb(symb_id_nothing);

  OBJ seq = size != 0 ? make_slice_uint8(data, size) : make_empty_seq();
  return make_tag_obj(symb_id_just, seq);

  // OBJ seq_obj = make_empty_seq();
  // if (size > 0) {
  //   SEQ_OBJ *seq = new_obj_seq(size);
  //   for (uint32 i=0 ; i < size ; i++)
  //     seq->buffer.obj[i] = make_int((uint8) data[i]);
  //   seq_obj = make_seq(seq, size);
  // }

  // return make_tag_obj(symb_id_just, seq_obj);
}

OBJ FileWrite_P(OBJ filename, OBJ data) {
  char *fname = obj_to_str(filename);
  uint32 size;
  uint8 *buffer = obj_to_byte_array(data, size);
  bool res;
  if (size > 0) {
    res = file_write(fname, (char *) buffer, size, false);
  }
  else {
    char empty_buff[1];
    res = file_write(fname, empty_buff, 0, false);
  }
  return make_bool(res);
}

OBJ GetChar_P() {
  int ch = getchar();
  if (ch == EOF)
    return make_symb(symb_id_nothing);
  return make_tag_int(symb_id_just, ch);
}

OBJ Now_P() {
  int64 time = get_epoc_time_nsec();
  return make_tag_int(symb_id_time, time);
}

OBJ Ticks_P() {
  const uint64 BLANK_TICKS = 0xFFFFFFFFFFFFFFFFULL;
  static uint64 init_ticks = BLANK_TICKS;

  uint64 ticks = get_tick_count();
  if (init_ticks == BLANK_TICKS)
    init_ticks = ticks;
  return make_int(ticks - init_ticks);
}

void Print_P(OBJ str_obj) {
  char *str = obj_to_str(str_obj);
  fputs(str, stdout);
  fflush(stdout);
}

void Exit_P(OBJ exit_code) {
  exit(get_int(exit_code));
}

OBJ Error_P(void *, void *) {
  return make_tag_obj(symb_id_string, make_empty_seq());
}