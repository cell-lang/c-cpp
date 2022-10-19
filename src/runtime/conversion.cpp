#include "lib.h"
#include "conversion.h"


OBJ import_string(std::string &str) {
  return str_to_obj(str.c_str());
}

OBJ import_date(int64 days_since_epoch) {
  return make_tag_int(symb_id_date, days_since_epoch);
}

OBJ import_time(int64 nanosecs_since_epoch) {
  return make_tag_int(symb_id_time, nanosecs_since_epoch);
}

OBJ import_obj(std::string &str) {
  OBJ obj;
  uint32 error_offset;

  if (parse(str.c_str(), str.length(), &obj, &error_offset))
    return obj;

  throw std::exception(); //## ADD INFORMATION ABOUT THE ERROR
}

OBJ import_symbol(std::string &str) {
  OBJ obj = import_obj(str);

  if (is_symb(obj))
    return obj;

  throw std::exception(); //## ADD INFORMATION ABOUT THE ERROR
}

OBJ import_date(std::string &str) {
  OBJ obj = import_obj(str);

  if (is_tag_obj(obj) && get_tag_id(obj) == symb_id_date && is_int(get_inner_obj(obj)))
    return obj;

  throw std::exception(); //## ADD INFORMATION ABOUT THE ERROR
}

OBJ import_time(std::string &str) {
  OBJ obj = import_obj(str);

  if (is_tag_obj(obj) && get_tag_id(obj) == symb_id_time && is_int(get_inner_obj(obj)))
    return obj;

  throw std::exception(); //## ADD INFORMATION ABOUT THE ERROR
}

OBJ import_byte_array(std::vector<unsigned char> &vector) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

OBJ import_byte_array(std::vector<int> &vector) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

std::string export_as_text(OBJ) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

std::string export_symbol(OBJ) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

std::string export_date(OBJ) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

std::string export_time(OBJ) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

std::string export_string(OBJ) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

std::vector<bool> export_bool_vector(OBJ obj) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

std::vector<unsigned char> export_byte_vector(OBJ obj) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

std::vector<int> export_int32_vector(OBJ obj) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

std::vector<long long> export_int64_vector(OBJ obj) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

std::vector<double> export_double_vector(OBJ obj) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

std::vector<std::string> export_string_vector(OBJ obj) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}

std::vector<std::string> export_text_vector(OBJ obj) {
  throw 0; //## IMPLEMENT IMPLEMENT IMPLEMENT
}
