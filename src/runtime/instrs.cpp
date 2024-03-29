#include "lib.h"


void init(STREAM &s) {
  s.buffer = s.internal_buffer;
  s.capacity = 32;
  s.count = 0;
}

void increase_stream_capacity(STREAM &s) {
  uint32 count = s.count;
  uint32 capacity = s.capacity;
  OBJ *buffer = s.buffer;
  uint32 new_capacity = capacity == 0 ? 32 : 2 * capacity;
  OBJ *new_buffer = new_obj_array(new_capacity);
  for (uint32 i=0 ; i < count ; i++)
    new_buffer[i] = buffer[i];
  s.buffer = new_buffer;
  s.capacity = new_capacity;

}

// void append(STREAM &s, OBJ obj) {
//   assert(s.count <= s.capacity);

//   uint32 count = s.count;
//   OBJ *buffer = s.buffer;

//   if (count == s.capacity)
//     increase_stream_capacity(s);

//   s.buffer[count] = obj;
//   s.count++;
// }

inline OBJ make_raw_set(OBJ *elts, uint32 size) {
  return make_set((SET_OBJ *) elts, size);
}

OBJ build_set_in_place(OBJ *elts, uint32 size) {
  if (size == 0)
    return make_empty_rel();

  if (size == 2) {
    OBJ elt0 = elts[0];
    OBJ elt1 = elts[1];

    int cr = comp_objs(elt0, elt1);

    if (cr == 0)
      return make_raw_set(elts, 1);

    if (cr < 0) {
      // elts[0] > elts[1], swapping
      elts[0] = elt1;
      elts[1] = elt0;
    }

    return make_raw_set(elts, 2);
  }

  if (size > 1)
    size = sort_unique(elts, size);

  return make_raw_set(elts, size);
}

OBJ build_set(STREAM &s) {
  // assert((s.count == 0 && s.capacity == 0 && s.buffer == NULL) || (s.count > 0 && s.capacity > 0 && s.buffer != NULL));

  uint32 size = s.count;
  if (size == 0)
    return make_empty_rel();

  OBJ *buffer = s.buffer;

  if (buffer != s.internal_buffer)
    return build_set_in_place(buffer, size);

  if (size == 2) {
    OBJ elt0 = buffer[0];
    OBJ elt1 = buffer[1];

    int cr = comp_objs(elt0, elt1);

    if (cr == 0) {
      SET_OBJ *set = new_set(1);
      set->buffer[0] = elt0;
      return make_set(set, 1);
    }

    SET_OBJ *set = new_set(2);
    if (cr > 0) { // elt0 < elt1
      set->buffer[0] = elt0;
      set->buffer[1] = elt1;
    }
    else { // elt0 > elt1
      set->buffer[0] = elt1;
      set->buffer[1] = elt0;
    }
    return make_set(set, 2);
  }

  // ## Showed no improvement

  // if (size == 3) {
  //   OBJ elt0 = buffer[0];
  //   OBJ elt1 = buffer[1];
  //   OBJ elt2 = buffer[2];

  //   int cr01 = comp_objs(elt0, elt1);

  //   if (cr01 == 0) {
  //     int cr12 = comp_objs(elt1, elt2);

  //     if (cr12 == 0) {
  //       // elt0 == elt1 == elt2
  //       SET_OBJ *set = new_set(1);
  //       set->buffer[0] = elt0;
  //       return make_set(set, 1);
  //     }

  //     SET_OBJ *set = new_set(2);
  //     if (cr12 > 0) {
  //       // elt0 == elt1, elt1 < elt2
  //       set->buffer[0] = elt1;
  //       set->buffer[1] = elt2;
  //     }
  //     else {
  //       // elt0 == elt1, elt1 > elt2
  //       set->buffer[0] = elt2;
  //       set->buffer[1] = elt1;
  //     }
  //     return make_set(set, 2);
  //   }

  //   if (cr01 < 0) {
  //     OBJ tmp = elt0;
  //     elt0 = elt1;
  //     elt1 = tmp;
  //   }

  //   // elt0 < elt1
  //   int cr12 = comp_objs(elt1, elt2);

  //   if (cr12 > 0) {
  //     // elt0 < elt1 < elt2
  //     SET_OBJ *set = new_set(3);
  //     set->buffer[0] = elt0;
  //     set->buffer[1] = elt1;
  //     set->buffer[2] = elt2;
  //     return make_set(set, 3);
  //   }
  //   else if (cr12 < 0) {
  //     int cr02 = comp_objs(elt0, elt2);

  //     if (cr02 > 0) {
  //       // elt0 < elt2 < elt1
  //       SET_OBJ *set = new_set(3);
  //       set->buffer[0] = elt0;
  //       set->buffer[1] = elt2;
  //       set->buffer[2] = elt1;
  //       return make_set(set, 3);
  //     }
  //     else if (cr02 < 0) {
  //       // elt2 < elt0 < elt1
  //       SET_OBJ *set = new_set(3);
  //       set->buffer[0] = elt2;
  //       set->buffer[1] = elt0;
  //       set->buffer[2] = elt1;
  //       return make_set(set, 3);
  //     }
  //     else {
  //       // elt0 < elt1, elt0 == elt2
  //       SET_OBJ *set = new_set(2);
  //       set->buffer[0] = elt0;
  //       set->buffer[1] = elt1;
  //       return make_set(set, 2);
  //     }
  //   }
  //   else {
  //     // elt0 < elt1, elts[1] == elt2
  //     SET_OBJ *set = new_set(2);
  //     set->buffer[0] = elt0;
  //     set->buffer[1] = elt1;
  //     return make_set(set, 2);
  //   }
  // }

  if (size > 1)
    size = sort_unique(buffer, size);

  SET_OBJ *ptr = new_set(size);
  memcpy(ptr->buffer, buffer, size * sizeof(OBJ));

  return make_set(ptr, size);
}

OBJ int_to_float(OBJ obj) {
  return make_float(get_int(obj));
}

OBJ internal_sort(OBJ set) {
  if (is_empty_rel(set))
    return make_empty_seq();

  uint32 size = read_size_field(set);
  OBJ *src = rearrange_if_needed_and_get_set_elts_ptr(set);

  OBJ *copy = new_obj_array(size);
  memcpy(copy, src, size * sizeof(OBJ));
  return build_seq(copy, size);
}

OBJ parse_value(OBJ str_obj) {
  char *raw_str = obj_to_str(str_obj);
  uint32 len = strlen(raw_str);
  OBJ obj;
  uint32 error_offset;
  bool ok = parse(raw_str, len, &obj, &error_offset);
  if (ok)
    return make_tag_obj(symb_id_success, obj);
  else
    return make_tag_obj(symb_id_failure, make_int(error_offset));
}

void get_set_iter(SET_ITER &it, OBJ set) {
  uint32 size = read_size_field(set);
  it.idx = 0;
  it.size = size;

  if (size != 0)
    it.buffer = rearrange_if_needed_and_get_set_elts_ptr(set);
#ifndef NDEBUG
  else
    it.buffer = 0;
#endif
}

void move_forward(SET_ITER &it) {
  assert(!is_out_of_range(it));
  it.idx++;
}

void move_forward(SEQ_ITER &it) {
  assert(!is_out_of_range(it));
  it.idx++;
}

void move_forward(BIN_REL_ITER &it) {
  assert(!is_out_of_range(it));

  if (it.type == BIN_REL_ITER::BRIT_OPT_REC) {
    for ( ; ; ) {
      if (++it.idx >= it.end)
        break;
      uint16 field = it.iter.opt_rec.fields[it.idx];
      if (opt_repr_has_field(it.iter.opt_rec.ptr, it.iter.opt_rec.repr_id, field))
        break;
    }
  }
  else {
    assert(it.type == BIN_REL_ITER::BRIT_BIN_REL);
    it.idx++;
  }
}

void move_forward(TERN_REL_ITER &it) {
  assert(!is_out_of_range(it));
  it.idx++;
}

void fail() {
#ifndef NDEBUG
  const char *MSG = "\nFail statement reached. Call stack:\n\n";
#else
  const char *MSG = "\nFail statement reached\n";
#endif

  soft_fail(MSG);
}

void runtime_check(OBJ cond) {
  assert(is_bool(cond));

  if (!get_bool(cond)) {
#ifndef NDEBUG
    fputs("\nAssertion failed. Call stack:\n\n", stderr);
#else
    fputs("\nAssertion failed\n", stderr);
#endif
    fflush(stderr);
    print_call_stack();
    *(char *)0 = 0; // Causing a runtime crash, useful for debugging
  }
}
