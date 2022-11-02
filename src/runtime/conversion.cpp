#include "lib.h"
#include "conversion.h"


OBJ import_string(const std::string &str) {
  return str_to_obj(str.c_str()); //## BUG BUG BUG: WHAT HAPPENS IF THE STRING IS NOT WELL-FORMED?
}

OBJ import_date(int64 days_since_epoch) {
  return make_tag_int(symb_id_date, days_since_epoch);
}

OBJ import_time(int64 nanosecs_since_epoch) {
  return make_tag_int(symb_id_time, nanosecs_since_epoch);
}

OBJ import_obj(const std::string &str) {
  OBJ obj;
  uint32 error_offset;

  if (parse(str.c_str(), str.length(), &obj, &error_offset))
    return obj;

  throw std::exception(); //## ADD INFORMATION ABOUT THE ERROR
}

OBJ import_symbol(const std::string &str) {
  OBJ obj = import_obj(str);

  if (is_symb(obj))
    return obj;

  throw std::exception(); //## ADD INFORMATION ABOUT THE ERROR
}

OBJ import_date(const std::string &str) {
  OBJ obj = import_obj(str);

  if (is_tag_obj(obj) && get_tag_id(obj) == symb_id_date && is_int(get_inner_obj(obj)))
    return obj;

  throw std::exception(); //## ADD INFORMATION ABOUT THE ERROR
}

OBJ import_time(const std::string &str) {
  OBJ obj = import_obj(str);

  if (is_tag_obj(obj) && get_tag_id(obj) == symb_id_time && is_int(get_inner_obj(obj)))
    return obj;

  throw std::exception(); //## ADD INFORMATION ABOUT THE ERROR
}

OBJ import_byte_array(const std::vector<unsigned char> &vector) {
  uint64 size = vector.size();

  if (size == 0)
    return make_empty_seq();

  uint8 *src_array = const_cast<unsigned char *>(vector.data());

  if (size <= 8)
    return make_seq_uint8_inline(inline_uint8_pack(src_array, size), size);

  //## BUG BUG BUG: CHECK THAT vector.size() IS NOT TOO LARGE

  //## TODO: COULDN'T WE JUST MAKE A SEQUENCE THAT POINTS DIRECTLY TO THE UNDERLYING ARRAY OF THE VECTOR?
  uint8 *array = new_uint8_array(size);
  memcpy(array, src_array, size * sizeof(uint8));
  return make_slice_uint8(array, size);
}

OBJ import_int_array(const std::vector<int> &vector) {
  uint64 size = vector.size();

  if (size == 0)
    return make_empty_seq();

  int32 *src_array = const_cast<int *>(vector.data());

  if (size <= 8) {
    int32 min = 0;
    int32 max = 0;

    for (uint32 i=0 ; i < size ; i++) {
      int32 elt = src_array[i];
      if (elt < min)
        min = elt;
      if (elt > max)
        max = elt;
    }

    if (is_uint8_range(min, max)) {
      int64 elts = 0;
      for (uint32 i=0 ; i < size ; i++)
        elts = inline_uint8_init_at(elts, i, (uint8) src_array[i]);
      return make_seq_uint8_inline(elts, size);
    }

    if (size <= 4 & is_int16_range(min, max)) {
      uint64 elts = 0;
      for (uint32 i=0 ; i < size ; i++)
        elts = inline_int16_init_at(elts, i, (int16) src_array[i]);
      return make_seq_int16_inline(elts, size);
    }

    if (size <= 2) {
      uint64 data = inline_int32_init_at(0, 0, src_array[0]);
      if (size == 2)
        data = inline_int32_init_at(data, 1, src_array[1]);
      return make_seq_int32_inline(data, size);
    }
  }

  //## BUG BUG BUG: CHECK THAT vector.size() IS NOT TOO LARGE

  //## TODO: COULDN'T WE JUST MAKE A SEQUENCE THAT POINTS DIRECTLY TO THE UNDERLYING ARRAY OF THE VECTOR?
  int32 *array = new_int32_array(size);
  memcpy(array, src_array, size * sizeof(int32));
  return make_slice_int32(array, size);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

std::string export_as_text(OBJ obj) {
  OBJ str = print_value(obj);
  return export_string(str);
}

std::string export_symbol(OBJ obj) {
  return export_as_text(obj);
}

std::string export_date(OBJ obj) {
  return export_as_text(obj);
}

std::string export_time(OBJ obj) {
  return export_as_text(obj);
}

std::string export_string(OBJ obj) {
  char inline_array[1024];
  uint32 size = utf8_size(obj);
  const char *str = size <= 1024 ? inline_array : (char *) new_uint8_array(size);
  obj_to_str(obj, const_cast<char *>(str), size);
  return std::string(str);
}

std::vector<bool> export_bool_vector(OBJ obj) {
  uint32 len = get_size(obj);
  std::vector<bool> result(len);
  if (len > 0) {
    OBJ *ptr = get_seq_elts_ptr(obj);
    for (uint32 i=0 ; i < len ; i++)
      result[i] = get_bool(ptr[i]);
  }
  return result;
}

std::vector<unsigned char> export_byte_vector(OBJ obj) {
  uint32 len = get_size(obj);
  std::vector<unsigned char> result(len);
  for (uint32 i=0 ; i < len ; i++)
    result[i] = get_int_at(obj, i);
  return result;
}

std::vector<int> export_int32_vector(OBJ obj) {
  uint32 len = get_size(obj);
  std::vector<int> result(len);
  for (uint32 i=0 ; i < len ; i++)
    result[i] = get_int_at(obj, i);
  return result;
}

std::vector<long long> export_int64_vector(OBJ obj) {
  uint32 len = get_size(obj);
  std::vector<long long> result(len);
  for (uint32 i=0 ; i < len ; i++)
    result[i] = get_int_at(obj, i);
  return result;
}

std::vector<double> export_double_vector(OBJ obj) {
  uint32 len = get_size(obj);
  std::vector<double> result(len);
  if (len > 0) {
    double *ptr = get_seq_elts_ptr_float(obj);
    for (uint32 i=0 ; i < len ; i++)
      result[i] = ptr[i];
  }
  return result;
}

std::vector<std::string> export_string_vector(OBJ obj) {
  uint32 len = get_size(obj);
  std::vector<std::string> result(len);
  if (len > 0) {
    OBJ *ptr = get_seq_elts_ptr(obj);
    for (uint32 i=0 ; i < len ; i++)
      result[i] = export_string(ptr[i]);
  }
  return result;
}

std::vector<std::string> export_text_vector(OBJ obj) {
  uint32 len = get_size(obj);
  std::vector<std::string> result(len);
  if (len > 0) {
    OBJ *ptr = get_seq_elts_ptr(obj);
    for (uint32 i=0 ; i < len ; i++)
      result[i] = export_as_text(ptr[i]);
  }
  return result;
}
