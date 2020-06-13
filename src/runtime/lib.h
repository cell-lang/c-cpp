#include "utils.h"


typedef signed char       int8;
typedef signed short      int16;
typedef signed int        int32;
typedef signed long long  int64;

typedef unsigned char       uint8;
typedef unsigned short      uint16;
typedef unsigned int        uint32;
typedef unsigned long long  uint64;


enum OBJ_TYPE {
  // Always inline
  TYPE_BLANK_OBJ    = 0,
  TYPE_NULL_OBJ     = 1,
  TYPE_SYMBOL       = 2,
  TYPE_INTEGER      = 3,
  TYPE_FLOAT        = 4,
  TYPE_EMPTY_SEQ    = 5,
  TYPE_EMPTY_REL    = 6,
  // Always references
  TYPE_NE_SEQ       = 7,
  TYPE_NE_SET       = 8,
  TYPE_NE_BIN_REL   = 9,
  TYPE_NE_TERN_REL  = 10,
  TYPE_TAG_OBJ      = 11,
  // Purely physical types
  TYPE_NE_SLICE     = 12,
  TYPE_NE_MAP       = 13,
  TYPE_NE_LOG_MAP   = 14,
  TYPE_OPT_REC      = 15,
  TYPE_OPT_TAG_REC  = 16
};

// Heap object can never be of the following types: TYPE_NE_SLICE, TYPE_NE_LOG_MAP

const uint32 MAX_INLINE_OBJ_TYPE_VALUE = TYPE_EMPTY_REL;


struct OBJ {
  union {
    int64   int_;
    double  float_;
    void*   ptr;
  } core_data;

  uint64 extra_data;

  // union {
  //   struct {
  //     uint16   symb_id;
  //     uint16   inner_tag;
  //     uint16   tag;
  //     uint8    unused_byte;
  //     unsigned type        : 4;
  //     unsigned mem_layout  : 2;
  //     unsigned num_tags    : 2;
  //   } std;
  //
  //   struct {
  //     uint32   length;
  //     uint16   tag;
  //     uint8    unused_byte;
  //     unsigned type        : 4;
  //     unsigned mem_layout  : 2;
  //     unsigned num_tags    : 2;
  //   } seq;
  //
  //   struct {
  //     uint32   length;
  //     unsigned offset      : 24;
  //     unsigned type        : 4;
  //     unsigned mem_layout  : 2;
  //     unsigned num_tags    : 2;
  //   } slice;
  //
  //   uint64 word;
  // } extra_data;
};

////////////////////////////////////////////////////////////////////////////////

struct SEQ_OBJ {
  uint32  capacity;
  uint32  size;
  OBJ     buffer[1];
};

struct SET_OBJ {
  uint32  size;
  OBJ     buffer[1];
};

struct BIN_REL_OBJ {
  uint32  size;
  OBJ     buffer[1];
};

struct TERN_REL_OBJ {
  uint32  size;
  OBJ     buffer[1];
};

struct TAG_OBJ {
  uint16 tag_id;
  uint16 unused_field;
  OBJ    obj;
};

////////////////////////////////////////////////////////////////////////////////

struct SEQ_ITER {
  OBJ    *buffer;
  uint32  idx;
  uint32  len;
};

struct SET_ITER {
  OBJ    *buffer;
  uint32  idx;
  uint32  size;
};

struct BIN_REL_ITER {
  union {
    struct {
      OBJ    *left_col;
      OBJ    *right_col;
      uint32 *rev_idxs; // If this is not null, we are iterating in right column order
    } bin_rel;

    struct {
      uint16 *fields;
      void *ptr;
      uint16 repr_id;
    } opt_rec;
  } iter;

  uint32  idx;
  uint32  end; // Non-inclusive upper bound

  enum {BRIT_BIN_REL, BRIT_OPT_REC} type;
};

struct TERN_REL_ITER {
  OBJ    *col1;
  OBJ    *col2;
  OBJ    *col3;
  uint32 *ordered_idxs; // If this is null, we iterate directly over the columns
  uint32  idx;
  uint32  end; // Non-inclusive upper bound
};

struct STREAM {
  OBJ    *buffer;
  uint32  capacity;
  uint32  count;
};

////////////////////////////////////////////////////////////////////////////////

const uint64 MAX_SEQ_LEN = 0xFFFFFFFF;

const uint32 INVALID_INDEX = 0xFFFFFFFFU;

const uint16 symb_id_false   = 0;
const uint16 symb_id_true    = 1;
const uint16 symb_id_void    = 2;
const uint16 symb_id_string  = 3;
const uint16 symb_id_nothing = 4;
const uint16 symb_id_just    = 5;
const uint16 symb_id_success = 6;
const uint16 symb_id_failure = 7;

///////////////////////////////// mem-core.cpp /////////////////////////////////

void* new_obj(uint32 byte_size);
void* new_obj(uint32 requested_byte_size, uint32 &returned_byte_size);

/////////////////////////////////// mem.cpp ////////////////////////////////////

OBJ* get_left_col_array_ptr(BIN_REL_OBJ*);
OBJ* get_right_col_array_ptr(BIN_REL_OBJ*);
uint32 *get_right_to_left_indexes(BIN_REL_OBJ*);

OBJ *get_col_array_ptr(TERN_REL_OBJ *rel, int idx);
uint32 *get_rotated_index(TERN_REL_OBJ *rel, int amount);

SET_OBJ      *new_set(uint32 size);       // Sets size
SEQ_OBJ      *new_seq(uint32 length);     // Sets length, capacity, used_capacity and elems
BIN_REL_OBJ  *new_map(uint32 size);       // Sets size, and clears rev_idxs
BIN_REL_OBJ  *new_bin_rel(uint32 size);   // Sets size
TERN_REL_OBJ *new_tern_rel(uint32 size);  // Sets size
TAG_OBJ      *new_tag_obj();

OBJ* new_obj_array(uint32 size);
OBJ* resize_obj_array(OBJ* buffer, uint32 size, uint32 new_size);

bool *new_bool_array(uint32 size);
double *new_double_array(uint32 size);

int64  *new_int64_array(uint32 size);
int32  *new_int32_array(uint32 size);
uint32 *new_uint32_array(uint32 size);
int16  *new_int16_array(uint32 size);
uint16 *new_uint16_array(uint32 size);
int8   *new_int8_array(uint32 size);
uint8  *new_uint8_array(uint32 size);

char *new_byte_array(uint32 size);
void *new_void_array(uint32 size);

//////////////////////////////// mem-utils.cpp /////////////////////////////////

OBJ_TYPE get_logical_type(OBJ); //## SHOULD IT EXPOSE THE DIFFERENCE BETWEEN MAPS AND NON-MAP BINARY RELATIONS?

bool is_blank_obj(OBJ);
bool is_null_obj(OBJ);
bool is_symb(OBJ);
bool is_bool(OBJ);
bool is_int(OBJ);
bool is_float(OBJ);
bool is_seq(OBJ);
bool is_empty_seq(OBJ);
bool is_ne_seq(OBJ);
bool is_empty_rel(OBJ);
bool is_set(OBJ);
bool is_ne_set(OBJ);
bool is_bin_rel(OBJ);
bool is_ne_bin_rel(OBJ);
bool is_ne_map(OBJ);
bool is_tern_rel(OBJ);
bool is_ne_tern_rel(OBJ);
bool is_tag_obj(OBJ);

bool is_symb(OBJ, uint16);
bool is_int(OBJ, int64);

uint16 get_symb_id(OBJ);
bool   get_bool(OBJ);
int64  get_int(OBJ);
double get_float(OBJ);
uint32 get_seq_length(OBJ);
uint16 get_tag_id(OBJ);
OBJ    get_inner_obj(OBJ);

OBJ make_blank_obj();
OBJ make_null_obj();
OBJ make_empty_seq();
OBJ make_empty_rel();
OBJ make_symb(uint16 symb_id);
OBJ make_bool(bool b);
OBJ make_int(uint64 value);
OBJ make_float(double value);
OBJ make_seq(SEQ_OBJ* ptr, uint32 length);
OBJ make_slice(SEQ_OBJ* ptr, uint32 offset, uint32 length);
OBJ make_set(SET_OBJ*);
OBJ make_bin_rel(BIN_REL_OBJ*);
OBJ make_tern_rel(TERN_REL_OBJ*);
OBJ make_log_map(BIN_REL_OBJ*);
OBJ make_map(BIN_REL_OBJ*);
OBJ make_tag_obj(uint16 tag_id, OBJ obj);
OBJ make_opt_tag_rec(void *ptr, uint16 type_id);

// These functions exist in a limbo between the logical and physical world

uint32 get_seq_offset(OBJ);
OBJ* get_seq_buffer_ptr(OBJ);

// Purely physical representation functions

OBJ_TYPE get_physical_type(OBJ);

bool is_opt_rec(OBJ);
bool is_opt_rec_or_tag_rec(OBJ);

SEQ_OBJ*      get_seq_ptr(OBJ);
SET_OBJ*      get_set_ptr(OBJ);
BIN_REL_OBJ*  get_bin_rel_ptr(OBJ);
TERN_REL_OBJ* get_tern_rel_ptr(OBJ);
TAG_OBJ*      get_tag_obj_ptr(OBJ);

void* get_opt_tag_rec_ptr(OBJ);

void* get_opt_repr_ptr(OBJ);
uint16 get_opt_repr_id(OBJ);

bool is_inline_obj(OBJ);

OBJ_TYPE get_ref_obj_type(OBJ);
void* get_ref_obj_ptr(OBJ);

bool are_shallow_eq(OBJ, OBJ);
int shallow_cmp(OBJ, OBJ);

//////////////////////////////// basic-ops.cpp /////////////////////////////////

bool inline_eq(OBJ obj1, OBJ obj2);
bool are_eq(OBJ obj1, OBJ obj2);
bool is_out_of_range(SET_ITER &it);
bool is_out_of_range(SEQ_ITER &it);
bool is_out_of_range(BIN_REL_ITER &it);
bool is_out_of_range(TERN_REL_ITER &it);

bool contains(OBJ set, OBJ elem);
bool contains_br(OBJ rel, OBJ arg1, OBJ arg2);
bool contains_br_1(OBJ rel, OBJ arg1);
bool contains_br_2(OBJ rel, OBJ arg2);
bool contains_tr(OBJ rel, OBJ arg1, OBJ arg2, OBJ arg3);
bool contains_tr_1(OBJ rel, OBJ arg1);
bool contains_tr_2(OBJ rel, OBJ arg2);
bool contains_tr_3(OBJ rel, OBJ arg3);
bool contains_tr_12(OBJ rel, OBJ arg1, OBJ arg2);
bool contains_tr_13(OBJ rel, OBJ arg1, OBJ arg3);
bool contains_tr_23(OBJ rel, OBJ arg2, OBJ arg3);

bool has_field(OBJ rec, uint16 field_symb_id);

uint32 get_size(OBJ);
int64 float_bits(OBJ);
int64 rand_nat(int64 max);  // Non-deterministic
int64 unique_nat();         // Non-deterministic

OBJ at(OBJ seq, int64 idx);

OBJ get_curr_obj(SET_ITER &it);
OBJ get_curr_obj(SEQ_ITER &it);
OBJ get_curr_left_arg(BIN_REL_ITER &it);
OBJ get_curr_right_arg(BIN_REL_ITER &it);
OBJ tern_rel_it_get_left_arg(TERN_REL_ITER &it);
OBJ tern_rel_it_get_mid_arg(TERN_REL_ITER &it);
OBJ tern_rel_it_get_right_arg(TERN_REL_ITER &it);
OBJ rand_set_elem(OBJ set);   // Non-deterministic

OBJ lookup(OBJ rel, OBJ key);
OBJ lookup_field(OBJ rec, uint16 field_symb_id);

////////////////////////////////// instrs.cpp //////////////////////////////////

void init(STREAM &s);
void append(STREAM &s, OBJ obj);                // obj must be already reference-counted
OBJ build_seq(OBJ* elems, uint32 length);       // Objects in elems must be already reference-counted
OBJ build_seq(STREAM &s);
OBJ build_set(OBJ* elems, uint32 size);
OBJ build_set(STREAM &s);
OBJ int_to_float(OBJ val);
OBJ blank_array(int64 size);
OBJ get_seq_slice(OBJ seq, int64 idx_first, int64 len);
OBJ append_to_seq(OBJ seq, OBJ obj);            // Both seq and obj must already be reference counted
OBJ update_seq_at(OBJ seq, OBJ idx, OBJ value); // Value must be reference counted already
OBJ join_seqs(OBJ left, OBJ right);
OBJ rev_seq(OBJ seq);
void set_at(OBJ seq, uint32 idx, OBJ value);    // Value must be already reference counted
OBJ internal_sort(OBJ set);
OBJ parse_value(OBJ str);
OBJ print_value(OBJ);
void get_set_iter(SET_ITER &it, OBJ set);
void get_seq_iter(SEQ_ITER &it, OBJ seq);
void move_forward(SET_ITER &it);
void move_forward(SEQ_ITER &it);
void move_forward(BIN_REL_ITER &it);
void move_forward(TERN_REL_ITER &it);
void fail();
void runtime_check(OBJ cond);

OBJ build_const_uint8_seq(const uint8* buffer, uint32 len);
OBJ build_const_uint16_seq(const uint16* buffer, uint32 len);
OBJ build_const_uint32_seq(const uint32* buffer, uint32 len);
OBJ build_const_int8_seq(const int8* buffer, uint32 len);
OBJ build_const_int16_seq(const int16* buffer, uint32 len);
OBJ build_const_int32_seq(const int32* buffer, uint32 len);
OBJ build_const_int64_seq(const int64* buffer, uint32 len);

//////////////////////////////// bin-rel-obj.cpp ///////////////////////////////

OBJ build_bin_rel(OBJ *col1, OBJ *col2, uint32 size);
OBJ build_bin_rel(STREAM &strm1, STREAM &strm2);

OBJ build_map(OBJ* keys, OBJ* values, uint32 size);
OBJ build_map(STREAM &key_stream, STREAM &value_stream);

void get_bin_rel_iter(BIN_REL_ITER &it, OBJ rel);
void get_bin_rel_iter_1(BIN_REL_ITER &it, OBJ rel, OBJ arg1);
void get_bin_rel_iter_2(BIN_REL_ITER &it, OBJ rel, OBJ arg2);

/////////////////////////////// tern-rel-obj.cpp ///////////////////////////////

OBJ build_tern_rel(OBJ *col1, OBJ *col2, OBJ *col3, uint32 size);
OBJ build_tern_rel(STREAM &strm1, STREAM &strm2, STREAM &strm3);

void get_tern_rel_iter(TERN_REL_ITER &it, OBJ rel);
void get_tern_rel_iter_by(TERN_REL_ITER &it, OBJ rel, int col_idx, OBJ arg);
void get_tern_rel_iter_by(TERN_REL_ITER &it, OBJ rel, int major_col_idx, OBJ major_arg, OBJ minor_arg);

/////////////////////////////////// debug.cpp //////////////////////////////////

int get_call_stack_depth();

void push_call_info(const char* fn_name, uint32 arity, OBJ* params);
void pop_call_info();
void pop_try_mode_call_info(int depth);
void print_call_stack();
void dump_var(const char* name, OBJ value);
void print_assertion_failed_msg(const char* file, uint32 line, const char* text);

void soft_fail(const char *msg);
void impl_fail(const char *msg);
// void physical_fail();
void internal_fail();

////////////////////////////////// sorting.cpp /////////////////////////////////

void stable_index_sort(uint32 *index, OBJ *values, uint32 count);
void stable_index_sort(uint32 *index, OBJ *major_sort, OBJ *minor_sort, uint32 count);
void stable_index_sort(uint32 *index, OBJ *major_sort, OBJ *middle_sort, OBJ *minor_sort, uint32 count);

void index_sort(uint32 *index, OBJ *values, uint32 count);
void index_sort(uint32 *index, OBJ *major_sort, OBJ *minor_sort, uint32 count);
void index_sort(uint32 *index, OBJ *major_sort, OBJ *middle_sort, OBJ *minor_sort, uint32 count);

/////////////////////////////////// algs.cpp ///////////////////////////////////

uint32 sort_unique(OBJ* objs, uint32 size);
uint32 sort_and_check_no_dups(OBJ* keys, OBJ* values, uint32 size);
void sort_obj_array(OBJ* objs, uint32 len);

uint32 find_obj(OBJ* sorted_array, uint32 len, OBJ obj, bool &found); //## WHAT SHOULD THIS RETURN? ANY VALUE IN THE [0, 2^32-1] IS A VALID SEQUENCE INDEX, SO WHAT COULD BE USED TO REPRESENT "NOT FOUND"?
uint32 find_objs_range(OBJ *sorted_array, uint32 len, OBJ obj, uint32 &count);
uint32 find_idxs_range(uint32 *sorted_idx_array, OBJ *values, uint32 len, OBJ obj, uint32 &count);
uint32 find_objs_range(OBJ *major_col, OBJ *minor_col, uint32 len, OBJ major_arg, OBJ minor_arg, uint32 &count);
uint32 find_idxs_range(uint32 *index, OBJ *major_col, OBJ *minor_col, uint32 len, OBJ major_arg, OBJ minor_arg, uint32 &count);

int comp_objs(OBJ obj1, OBJ obj2);

/////////////////////////////// inter-utils.cpp ////////////////////////////////

void add_obj_to_cache(OBJ);

uint16 lookup_symb_id(const char *, uint32);

const char *symb_to_raw_str(uint16);

OBJ to_str(OBJ);
OBJ to_symb(OBJ);

OBJ extern_str_to_symb(const char *);

OBJ str_to_obj(const char* c_str);

char* obj_to_str(OBJ str_obj);
void obj_to_str(OBJ str_obj, char *buffer, uint32 size);

char* obj_to_byte_array(OBJ byte_seq_obj, uint32 &size);

uint64 char_buffer_size(OBJ str_obj);

//////////////////////////////// conversion.cpp ////////////////////////////////

OBJ convert_bool_seq(const bool *array, uint32 size);
OBJ convert_int32_seq(const int32 *array, uint32 size);
OBJ convert_int_seq(const int64 *array, uint32 size);
OBJ convert_float_seq(const double *array, uint32 size);
OBJ convert_text(const char *buffer);

// void export_as_c_string(OBJ obj, char *buffer, uint32 capacity);
uint32 export_as_bool_array(OBJ obj, bool *array, uint32 capacity);
uint32 export_as_long_long_array(OBJ obj, int64 *array, uint32 capacity);
uint32 export_as_float_array(OBJ obj, double *array, uint32 capacity);
void export_literal_as_c_string(OBJ obj, char *buffer, uint32 capacity);

///////////////////////////////// printing.cpp /////////////////////////////////

typedef enum {TEXT, SUB_START, SUB_END} EMIT_ACTION;

void print_obj(OBJ obj, void (*emit)(void *, const void *, EMIT_ACTION), void *data);

void print(OBJ);
void print_to_buffer_or_file(OBJ obj, char* buffer, uint32 max_size, const char* fname);
uint32 printed_obj(OBJ obj, char* buffer, uint32 max_size);
char *printed_obj(OBJ obj, char *alloc(void *, uint32), void *data);

////////////////////////////////// parsing.cpp /////////////////////////////////

bool parse(const char *text, uint32 size, OBJ *var, uint32 *error_offset);

///////////////////////////// os-interface-xxx.cpp /////////////////////////////

uint64 get_tick_count();   // Impure

////////////////////////////////// hashing.cpp /////////////////////////////////

uint32 compute_hash_code(OBJ obj);

///////////////////////////// not implemented yet //////////////////////////////

int64  get_inner_long(OBJ);

OBJ*    get_obj_array(OBJ seq, OBJ* buffer, int32 size);
int64*  get_long_array(OBJ seq, int64 *buffer, int32 size);
double* get_double_array(OBJ seq, double *buffer, int32 size);
bool*   get_bool_array(OBJ seq, bool *buffer, int32 size);

OBJ build_seq(int64* array, int32 size);
OBJ build_seq(int32* array, int32 size);
OBJ build_seq(uint32* array, int32 size);
OBJ build_seq(int16* array, int32 size);
OBJ build_seq(uint16* array, int32 size);
OBJ build_seq(int8* array, int32 size);
OBJ build_seq(uint8* array, int32 size);

OBJ build_seq(bool* array, int32 size);
OBJ build_seq(double* array, int32 size);

OBJ build_record(uint16 *labels, OBJ *value, int32 count);

double float_pow(double, double);
double float_sqrt(double);
int64 float_round(double);
int32 cast_int32(int64);
OBJ set_insert(OBJ, OBJ);
OBJ set_key_value(OBJ, OBJ, OBJ);
OBJ make_tag_int(uint16, int64);

uint8 as_byte(int64);
int16 as_short(int64);
int32 as_int(int64);

OBJ    *array_append(OBJ*,    int32, int32&, OBJ);
bool   *array_append(bool*,   int32, int32&, bool);
double *array_append(double*, int32, int32&, double);
int64  *array_append(int64*,  int32, int32&, int64);
int32  *array_append(int32*,  int32, int32&, int32);
uint32 *array_append(uint32*, int32, int32&, uint32);
int16  *array_append(int16*,  int32, int32&, int16);
uint16 *array_append(uint16*, int32, int32&, uint16);
int8   *array_append(int8*,   int32, int32&, int8);
uint8  *array_append(uint8*,  int32, int32&, uint8);

OBJ array_at(OBJ*,    int32, int32);
OBJ array_at(bool*,   int32, int32);
OBJ array_at(double*, int32, int32);
OBJ array_at(int64*,  int32, int32);
OBJ array_at(int32*,  int32, int32);
OBJ array_at(uint32*, int32, int32);
OBJ array_at(int16*,  int32, int32);
OBJ array_at(uint16*, int32, int32);
OBJ array_at(int8*,   int32, int32);
OBJ array_at(uint8*,  int32, int32);

bool bool_array_at(bool*, int32, int32);
double float_array_at(double*, int32, int32);

int64 int_array_at(int64*,  int32, int32);
int64 int_array_at(int32*,  int32, int32);
int64 int_array_at(uint32*, int32, int32);
int64 int_array_at(int16*,  int32, int32);
int64 int_array_at(uint16*, int32, int32);
int64 int_array_at(int8*,   int32, int32);
int64 int_array_at(uint8*,  int32, int32);

double get_float_at(OBJ, int64);
int64  get_int_at(OBJ, int64);
