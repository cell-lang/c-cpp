#include "lib.h"


struct ENV_;
typedef struct ENV_ ENV;


static std::map<void*, OBJ> attachments;

OBJ attach_F2(OBJ obj_V, OBJ data_V, ENV &env) {
  if (!is_inline_obj(obj_V)) {
    void *ptr = get_ref_obj_ptr(obj_V);
    attachments[ptr] = data_V;
  }
  return obj_V;
}

OBJ fetch_F1(OBJ obj_V, ENV &env) {
  if (!is_inline_obj(obj_V)) {
    void *ptr = get_ref_obj_ptr(obj_V);
    std::map<void*, OBJ>::iterator it = attachments.find(ptr);
    if (it != attachments.end())
      return make_tag_obj(symb_id_just, it->second);
  }
  return make_symb(symb_id_nothing);
}

////////////////////////////////////////////////////////////////////////////////

OBJ source_file_location_F1(OBJ mtc_V, ENV &env) {
  OBJ source_file_location_F1_(OBJ mtc_V, ENV &env);

  static std::map<void*, OBJ> cache;

  if (is_inline_obj(mtc_V))
    return source_file_location_F1_(mtc_V, env);

  void *ptr = get_ref_obj_ptr(mtc_V);
  std::map<void*, OBJ>::iterator it = cache.find(ptr);
  if (it != cache.end())
    return it->second;

  OBJ value = source_file_location_F1_(mtc_V, env);
  cache[ptr] = value;
  return value;
}
