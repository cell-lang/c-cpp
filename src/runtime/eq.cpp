#include "lib.h"
#include "extern.h"


uint32 get_seq_length_(OBJ seq);



__attribute__ ((noinline)) bool slow_eq(OBJ obj1, OBJ obj2) {
  return comp_objs(obj1, obj2) == 0;
}

bool eq_tag_obj(OBJ obj1, OBJ_TYPE type1, OBJ obj2) {
  assert(get_physical_type(obj2) == TYPE_TAG_OBJ);
  return slow_eq(obj1, obj2);
}

////////////////////////////////////////////////////////////////////////////////

bool eq(OBJ obj1, OBJ obj2) {
  if (are_shallow_eq(obj1, obj2))
    return true;

  if (is_inline_obj(obj1), is_inline_obj(obj2))
    return false;

  OBJ_TYPE type1 = get_physical_type(obj1);
  OBJ_TYPE type2 = get_physical_type(obj2);

  if (type1 == type2) {
    switch (type1) {
      case TYPE_NE_SEQ:
      case TYPE_NE_SLICE: {
        if (obj1.extra_data != obj2.extra_data)
          return false;

        int len = get_seq_length_(obj1);
        OBJ *elts1 = (OBJ *) obj1.core_data.ptr;
        OBJ *elts2 = (OBJ *) obj2.core_data.ptr;

        for (int i=0 ; i < len ; i++)
          if (!eq(elts1[i], elts2[i]))
            return false;

        return true;
      }

      case TYPE_NE_SET: {
        if (obj1.extra_data != obj2.extra_data)
          return false;

        SET_OBJ *ptr1 = (SET_OBJ *) obj1.core_data.ptr;
        SET_OBJ *ptr2 = (SET_OBJ *) obj2.core_data.ptr;

        int size = ptr1->size;
        if (ptr2->size != size)
          return false;

        OBJ *elts1 = ptr1->buffer;
        OBJ *elts2 = ptr2->buffer;

        for (int i=0 ; i < size ; i++)
          if (!eq(elts1[i], elts2[i]))
            return false;

        return true;
      }

      case TYPE_NE_BIN_REL:
      case TYPE_NE_MAP:
      case TYPE_NE_LOG_MAP: {
        if (obj1.extra_data != obj2.extra_data)
          return false;

        BIN_REL_OBJ *ptr1 = (BIN_REL_OBJ *) obj1.core_data.ptr;
        BIN_REL_OBJ *ptr2 = (BIN_REL_OBJ *) obj2.core_data.ptr;

        int size = ptr1->size;
        if (ptr2->size != size)
          return false;

        OBJ *args1 = ptr1->buffer;
        OBJ *args2 = ptr2->buffer;

        for (int i=0 ; i < 2 * size ; i++)
          if (!eq(args1[i], args2[i]))
            return false;

        return true;
      }

      case TYPE_NE_TERN_REL: {
        if (obj1.extra_data != obj2.extra_data)
          return false;

        TERN_REL_OBJ *ptr1 = (TERN_REL_OBJ *) obj1.core_data.ptr;
        TERN_REL_OBJ *ptr2 = (TERN_REL_OBJ *) obj2.core_data.ptr;

        int size = ptr1->size;
        if (ptr2->size != size)
          return false;

        OBJ *args1 = ptr1->buffer;
        OBJ *args2 = ptr2->buffer;

        for (int i=0 ; i < 3 * size ; i++)
          if (!eq(args1[i], args2[i]))
            return false;

        return true;
      }

      case TYPE_TAG_OBJ: {
        return eq_tag_obj(obj1, type1, obj2);
      }

      case TYPE_OPT_REC:
      case TYPE_OPT_TAG_REC: {
        if (obj1.extra_data != obj2.extra_data)
          return false;

        void *ptr1 = obj1.core_data.ptr;
        void *ptr2 = obj2.core_data.ptr;
        uint16 repr_id = get_opt_repr_id(obj1);

        if (!opt_repr_may_have_opt_fields(repr_id)) {
          uint32 count;
          uint16 *labels = opt_repr_get_fields(repr_id, count);

          for (int i=0 ; i < count ; i++) {
            uint16 label = labels[i];
            OBJ value1 = opt_repr_lookup_field(ptr1, repr_id, label);
            OBJ value2 = opt_repr_lookup_field(ptr2, repr_id, label);

            if (!eq(value1, value2))
              return false;
          }

          return true;
        }
        else {
          if (opt_repr_get_fields_count(ptr1, repr_id) != opt_repr_get_fields_count(ptr2, repr_id))
            return false;

          uint32 count;
          uint16 *labels = opt_repr_get_fields(repr_id, count);

          for (int i=0 ; i < count ; i++) {
            uint16 label = labels[i];

            bool has_field_1 = opt_repr_has_field(ptr1, repr_id, label);
            bool has_field_2 = opt_repr_has_field(ptr2, repr_id, label);

            if (has_field_1) {
              if (has_field_2) {
                OBJ value1 = opt_repr_lookup_field(ptr1, repr_id, label);
                OBJ value2 = opt_repr_lookup_field(ptr2, repr_id, label);

                if (!eq(value1, value2))
                  return false;
              }
              else
                return false;
            }
            else if (has_field_2)
              return false;
          }

          return true;
        }
      }
    }
  }
  else {
    return slow_eq(obj1, obj2);
    // if (type2 == TYPE_TAG_OBJ)
    //   return eq_tag_obj(obj1, type1, obj2);
  }
}

////////////////////////////////////////////////////////////////////////////////

__attribute__ ((noinline)) bool are_eq(OBJ obj1, OBJ obj2) {
  // if (eq(obj1, obj2) != slow_eq(obj1, obj2)) {
  //   print(obj1);
  //   print(obj2);
  //   printf("\n%s\n", slow_eq(obj1, obj2) ? "equal" : "not equal");
  // }

  return eq(obj1, obj2);
}
