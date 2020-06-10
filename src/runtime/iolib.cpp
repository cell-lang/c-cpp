#include "lib.h"
#include "os-interface.h"


struct ENV_;
typedef struct ENV_ ENV;


OBJ FileRead_P(OBJ filename, ENV &) {
  char *fname = obj_to_str(filename);
  int size;
  char *data = file_read(fname, size);

  if (size == -1)
    return make_symb(symb_idx_nothing);

  OBJ seq_obj = make_empty_seq();
  if (size > 0) {
    SEQ_OBJ *seq = new_seq(size);
    for (uint32 i=0 ; i < size ; i++)
      seq->buffer[i] = make_int((uint8) data[i]);
    seq_obj = make_seq(seq, size);
  }

  return make_tag_obj(symb_idx_just, seq_obj);
}


OBJ FileWrite_P(OBJ filename, OBJ data, ENV &) {
  char *fname = obj_to_str(filename);
  uint32 size;
  char *buffer = obj_to_byte_array(data, size);
  bool res;
  if (size > 0) {
    res = file_write(fname, buffer, size, false);
  }
  else {
    char empty_buff[1];
    res = file_write(fname, empty_buff, 0, false);
  }
  return make_bool(res);
}


OBJ Print_P(OBJ str_obj, ENV &env) {
  char *str = obj_to_str(str_obj);
  fputs(str, stdout);
  fflush(stdout);
  return make_blank_obj();
}


OBJ GetChar_P(ENV &env) {
  int ch = getchar();
  if (ch == EOF)
    return make_symb(symb_idx_nothing);
  return make_tag_obj(symb_idx_just, make_int(ch));
}
