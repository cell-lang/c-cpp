#include "lib.h"


void write_str(WRITE_FILE_STATE *write_state, const char *str) {
  uint32 len = strlen(str);
  size_t written = fwrite(str, 1, len, write_state->fp);
  if (written != len) {
    //## SAVE ERROR INFORMATION
    write_state->success = false;
  }
}

void write_symb(WRITE_FILE_STATE *write_state, uint16 symb_id) {
  const char *str = symb_to_raw_str(symb_id);
  write_str(write_state, str);
}

bool finish_write(WRITE_FILE_STATE *write_state) {
  if (fflush(write_state->fp) != 0) {
    //## SAVE ERROR INFORMATION
    write_state->success = false;
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

static void file_write(void *state, const char *text, uint32 len) {
  WRITE_FILE_STATE *write_state = (WRITE_FILE_STATE *) state;
  size_t written = fwrite(text, 1, len, write_state->fp);
  if (written != len) {
    //## SAVE ERROR INFORMATION
    write_state->success = false;
  }
}

static void emit_file_write(void *state, const void *data, EMIT_ACTION action) {
  if (action == TEXT) {
    const char *text = (const char *) data;
    file_write(state, text, strlen(text));
  }
  else if (action == SUB_START) {

  }
  else {
    assert(action == SUB_END);
  }
}

void write_obj(WRITE_FILE_STATE *write_state, OBJ obj) {
  print_obj(obj, emit_file_write, write_state);
}
