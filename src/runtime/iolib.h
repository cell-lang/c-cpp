OBJ FileRead_P(OBJ);
OBJ FileWrite_P(OBJ, OBJ);
OBJ GetChar_P();
OBJ Ticks_P();
void Print_P(OBJ);
void Exit_P(OBJ);
OBJ Error_P(void *, void *);


struct ENV_;

inline OBJ FileRead_P(OBJ fname, struct ENV_ &) {
  return FileRead_P(fname);
}

inline OBJ FileWrite_P(OBJ fname, OBJ data, struct ENV_ &) {
  return FileWrite_P(fname, data);
}

inline OBJ GetChar_P(struct ENV_ &env) {
  return GetChar_P();
}

inline OBJ Ticks_P(struct ENV_ &env) {
  return Ticks_P();
}

inline void Print_P(OBJ str, struct ENV_ &env) {
  Print_P(str);
}

inline void Exit_P(OBJ exit_code, struct ENV_ &env) {
  Exit_P(exit_code);
}

inline OBJ Error_P(void *rel_auto, void *rel_auto_aux, struct ENV_ &env) {
  return Error_P(rel_auto, rel_auto_aux);
}
