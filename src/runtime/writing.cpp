#include "lib.h"


void write_str(WRITE_FILE_STATE *write_state, const char *str) {
  uint32 len = strlen(str);
  bool success = write_state->write(write_state->state, (const uint8 *) str, len);
  if (!success) {
    //## SAVE ERROR INFORMATION
    write_state->success = false;
  }
}

void write_symb(WRITE_FILE_STATE *write_state, uint16 symb_id) {
  const char *str = symb_to_raw_str(symb_id);
  write_str(write_state, str);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

static void emit_file_write(void *state, const void *data, EMIT_ACTION action) {
  if (action == TEXT) {
    write_str((WRITE_FILE_STATE *) state, (const char *) data);
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
