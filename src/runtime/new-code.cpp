#include "lib.h"


OBJ make_tag_int(uint16 tag, int64 value) {
  return make_tag_obj(tag, make_int(value));
}

int64 get_inner_long(OBJ obj) {
  return get_int(get_inner_obj(obj));
}

////////////////////////////////////////////////////////////////////////////////

int32 cast_int32(int64 val64) {
  int32 val32 = (int32) val64;
  if (val32 != val64)
    soft_fail("Invalid 64 to 32 bit integer conversion");
  return val32;
}

OBJ set_insert(OBJ set, OBJ elt) {
  impl_fail("Not implemented yet");
}

OBJ set_key_value(OBJ map, OBJ key, OBJ value) {
  impl_fail("Not implemented yet");
}

////////////////////////////////////////////////////////////////////////////////

double float_pow(double base, double exp) {
  return pow(base, exp);
}

double float_sqrt(double x) {
  return sqrt(x);
}

int64 float_round(double x) {
  return (int64) x;
}

////////////////////////////////////////////////////////////////////////////////

bool is_ne_int_seq(OBJ obj) {
  if (!is_ne_seq(obj))
    return false;

  int len = get_size(obj);
  OBJ *elts = get_seq_elts_ptr(obj);
  for (int i=0 ; i < len ; i++)
    if (!is_int(elts[i]))
      return false;

  return true;
}

bool is_ne_float_seq(OBJ obj) {
  if (!is_ne_seq(obj))
    return false;

  int len = get_size(obj);
  OBJ *elts = get_seq_elts_ptr(obj);
  for (int i=0 ; i < len ; i++)
    if (!is_float(elts[i]))
      return false;

  return true;
}

////////////////////////////////////////////////////////////////////////////////

OBJ build_record(uint16 *labels, OBJ *values, int32 count) {
  OBJ buffer[1024];
  if (count > 1024)
    impl_fail("Record with more than 1024 fields");
  for (int i=0 ; i < count ; i++)
    buffer[i] = make_symb(labels[i]);
  return build_map(buffer, values, count);
}
