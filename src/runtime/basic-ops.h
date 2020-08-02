inline bool inline_eq(OBJ obj1, OBJ obj2) {
  assert(is_inline_obj(obj2));
  return are_shallow_eq(obj1, obj2);
}

inline bool is_out_of_range(SET_ITER &it) {
  return it.idx >= it.size;
}

inline bool is_out_of_range(SEQ_ITER &it) {
  return it.idx >= it.len;
}

inline bool is_out_of_range(BIN_REL_ITER &it) {
  return it.idx >= it.end;
}

inline bool is_out_of_range(TERN_REL_ITER &it) {
  return it.idx >= it.end;
}

inline uint32 get_size(OBJ coll) {
  assert(is_seq(coll) | is_set(coll) | is_bin_rel(coll) | is_tern_rel(coll));

  if (is_opt_rec(coll))
    return opt_repr_get_fields_count(get_opt_repr_ptr(coll), get_opt_repr_id(coll));

  return read_size_field(coll);
}

////////////////////////////////////////////////////////////////////////////////

inline OBJ get_curr_obj(SEQ_ITER &it) {
  assert(!is_out_of_range(it));
  return get_obj_at(it.seq, it.idx);
}

inline OBJ get_curr_obj(SET_ITER &it) {
  assert(!is_out_of_range(it));
  return it.buffer[it.idx];
}

////////////////////////////////////////////////////////////////////////////////

inline int64 float_bits(OBJ obj) {
  double x = get_float(obj);
  return *((int64 *) &x);
}

inline double float_pow(double base, double exp) {
  return pow(base, exp);
}

inline double float_sqrt(double x) {
  return sqrt(x);
}

inline int64 float_round(double x) {
  return (int64) x;
}

////////////////////////////////////////////////////////////////////////////////

inline int64 rand_nat(int64 max) {
  assert(max > 0);
  return rand() % max; //## BUG: THE FUNCTION rand() ONLY GENERATES A LIMITED RANGE OF INTEGERS
}

inline int64 unique_nat() {
  static int64 next_val = 0;
  return next_val++;
}
