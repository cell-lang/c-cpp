#include "utils.h"


enum OBJ_TYPE {
  // Inline objects
  TYPE_NOT_A_VALUE_OBJ      = 0,
  TYPE_SYMBOL               = 1,
  TYPE_INTEGER              = 2,
  TYPE_FLOAT                = 3,
  TYPE_EMPTY_REL            = 4,
  TYPE_EMPTY_SEQ            = 5,
  TYPE_NE_SEQ_UINT8_INLINE  = 6,
  TYPE_NE_SEQ_INT16_INLINE  = 7,
  TYPE_NE_SEQ_INT32_INLINE  = 8,

  // Always references
  TYPE_NE_INT_SEQ           =  9,
  TYPE_NE_FLOAT_SEQ         = 10,
  TYPE_NE_BOOL_SEQ          = 11,
  TYPE_NE_SEQ               = 12,
  TYPE_NE_SET               = 13,
  TYPE_NE_MAP               = 14,
  TYPE_NE_BIN_REL           = 15,
  TYPE_NE_TERN_REL          = 16,
  TYPE_AD_HOC_TAG_REC       = 17,
  TYPE_BOXED_OBJ            = 18
};

enum INT_BITS_TAG {
  INT_BITS_TAG_8  = 0,
  INT_BITS_TAG_16 = 1,
  INT_BITS_TAG_32 = 2,
  INT_BITS_TAG_64 = 3
};

const OBJ_TYPE MAX_INLINE_OBJ_TYPE          = TYPE_NE_SEQ_INT32_INLINE;

const OBJ_TYPE NE_SEQ_TYPE_RANGE_START      = TYPE_NE_SEQ_UINT8_INLINE;
const OBJ_TYPE NE_SEQ_TYPE_RANGE_END        = TYPE_NE_SEQ;

const OBJ_TYPE NE_INT_SEQ_TYPE_RANGE_START  = TYPE_NE_SEQ_UINT8_INLINE;
const OBJ_TYPE NE_INT_SEQ_TYPE_RANGE_END    = TYPE_NE_INT_SEQ;

////////////////////////////////////////////////////////////////////////////////

struct OBJ {
  union {
    int64   int_;
    double  float_;
    void   *ptr;
  } core_data;

  uint64 extra_data;
};

////////////////////////////////////////////////////////////////////////////////

struct SEQ_OBJ {
  uint32 capacity;
  uint32 used;
  union {
    OBJ    obj[1];
    double float_[1];
    uint8  uint8_[1];
    int8   int8_[1];
    int16  int16_[1];
    int32  int32_[1];
    int64  int64_[1];
  } buffer;
};

struct SET_OBJ {
  OBJ buffer[1];
};

struct BIN_REL_OBJ {
  OBJ buffer[1];
};

struct TERN_REL_OBJ {
  OBJ buffer[1];
};

struct BOXED_OBJ {
  OBJ obj;
};

////////////////////////////////////////////////////////////////////////////////

struct SEQ_ITER {
  OBJ    seq;
  uint32 idx;
  uint32 len;
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
  OBJ     internal_buffer[32];
};

////////////////////////////////////////////////////////////////////////////////

struct PARSER;

////////////////////////////////////////////////////////////////////////////////

const uint64 MAX_SEQ_LEN = 0xFFFFFFFF;

const uint32 INVALID_INDEX = 0xFFFFFFFFU;

const uint16 symb_id_false    = 0;
const uint16 symb_id_true     = 1;
const uint16 symb_id_void     = 2;
const uint16 symb_id_string   = 3;
const uint16 symb_id_date     = 4;
const uint16 symb_id_time     = 5;
const uint16 symb_id_nothing  = 6;
const uint16 symb_id_just     = 7;
const uint16 symb_id_success  = 8;
const uint16 symb_id_failure  = 9;

////////////////////////////// auto-mem-alloc.cpp //////////////////////////////

typedef struct {
  void *subpools[9];
} STATE_MEM_POOL;

void init_mem_pool(STATE_MEM_POOL *);
void release_mem_pool(STATE_MEM_POOL *);

OBJ copy_to_pool(STATE_MEM_POOL *, OBJ);
void remove_from_pool(STATE_MEM_POOL *, OBJ);

uint32 obj_mem_size(OBJ);
OBJ copy_obj_to(OBJ, void **);

void *grab_mem(void **, uint32);

inline uint32 round_up_8(uint32 mem_size) {
  return (mem_size + 7) & ~7;
}

///////////////////////////////// mem-core.cpp /////////////////////////////////

void *alloc_eternal_block(uint32 byte_size);

void *alloc_static_block(uint32 byte_size);
void *release_static_block(void *ptr, uint32 byte_size);

void* new_obj(uint32 byte_size);

/////////////////////////////////// mem.cpp ////////////////////////////////////

uint64 seq_obj_mem_size(uint64 capacity);
uint64 uint8_seq_obj_mem_size(uint64 capacity);
uint64 int8_seq_obj_mem_size(uint64 capacity);
uint64 int16_seq_obj_mem_size(uint64 capacity);
uint64 int32_seq_obj_mem_size(uint64 capacity);
uint64 int64_seq_obj_mem_size(uint64 capacity);
uint64 set_obj_mem_size(uint64 size);
uint64 bin_rel_obj_mem_size(uint64 size);
uint64 tern_rel_obj_mem_size(uint64 size);
uint64 map_obj_mem_size(uint64 size);
uint64 boxed_obj_mem_size();

OBJ* get_left_col_array_ptr(BIN_REL_OBJ*);
OBJ* get_right_col_array_ptr(BIN_REL_OBJ *rel, uint32 size);
uint32 *get_right_to_left_indexes(BIN_REL_OBJ *rel, uint32 size);

OBJ *get_col_array_ptr(TERN_REL_OBJ *rel, uint32 size, int idx);
uint32 *get_rotated_index(TERN_REL_OBJ *rel, uint32 size, int amount);

SET_OBJ      *new_set(uint32 size);
BIN_REL_OBJ  *new_map(uint32 size); // Clears rev_idxs
BIN_REL_OBJ  *new_bin_rel(uint32 size);
TERN_REL_OBJ *new_tern_rel(uint32 size);
BOXED_OBJ    *new_boxed_obj();

// Set used and capacity fields
SEQ_OBJ *new_obj_seq(uint32 length);
SEQ_OBJ *new_obj_seq(uint32 length, uint32 capacity);
SEQ_OBJ *new_float_seq(uint32 length);
SEQ_OBJ *new_float_seq(uint32 length, uint32 capacity);
SEQ_OBJ *new_uint8_seq(uint32 length);
SEQ_OBJ *new_uint8_seq(uint32 length, uint32 capacity);
SEQ_OBJ *new_int8_seq(uint32 length);
SEQ_OBJ *new_int8_seq(uint32 length, uint32 capacity);
SEQ_OBJ *new_int16_seq(uint32 length);
SEQ_OBJ *new_int16_seq(uint32 length, uint32 capacity);
SEQ_OBJ *new_int32_seq(uint32 length);
SEQ_OBJ *new_int32_seq(uint32 length, uint32 capacity);
SEQ_OBJ *new_int64_seq(uint32 length);
SEQ_OBJ *new_int64_seq(uint32 length, uint32 capacity);

OBJ* new_obj_array(uint32 size);
OBJ* resize_obj_array(OBJ* buffer, uint32 size, uint32 new_size);

bool *new_bool_array(uint32 size);
double *new_float_array(uint32 size);

int64  *new_int64_array(uint32 size);
uint64 *new_uint64_array(uint32 size);
int32  *new_int32_array(uint32 size);
uint32 *new_uint32_array(uint32 size);
int16  *new_int16_array(uint32 size);
uint16 *new_uint16_array(uint32 size);
int8   *new_int8_array(uint32 size);
uint8  *new_uint8_array(uint32 size);

char *new_byte_array(uint32 size);
void *new_void_array(uint32 size);

//////////////////////////////// mem-utils.cpp /////////////////////////////////

bool is_blank(OBJ);
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
uint32 read_size_field(OBJ);
uint16 get_tag_id(OBJ);
OBJ    get_inner_obj(OBJ);

OBJ make_blank_obj();
OBJ make_empty_seq();
OBJ make_empty_rel();
OBJ make_symb(uint16 symb_id);
OBJ make_bool(bool b);
OBJ make_int(uint64 value);
OBJ make_float(double value);
OBJ make_set(SET_OBJ*, uint32 size);
OBJ make_bin_rel(BIN_REL_OBJ*, uint32 size);
OBJ make_tern_rel(TERN_REL_OBJ*, uint32 size);
OBJ make_map(BIN_REL_OBJ*, uint32 size);
OBJ make_tag_obj(uint16 tag_id, OBJ obj);
OBJ make_opt_tag_rec(void *ptr, uint16 repr_id);

OBJ make_seq(SEQ_OBJ *ptr, uint32 length);
OBJ make_slice(OBJ *ptr, uint32 length);

OBJ make_seq_float(SEQ_OBJ *ptr, uint32 length);
OBJ make_slice_float(double *ptr, uint32 length);

OBJ make_seq_uint8(SEQ_OBJ *ptr, uint32 length);
OBJ make_slice_uint8(uint8 *ptr, uint32 length);
OBJ make_seq_uint8_inline(uint64 data, uint32 length);

OBJ make_seq_int8(SEQ_OBJ *ptr, uint32 length);
OBJ make_slice_int8(int8 *ptr, uint32 length);

OBJ make_seq_int16(SEQ_OBJ *ptr, uint32 length);
OBJ make_slice_int16(int16 *ptr, uint32 length);
OBJ make_seq_int16_inline(uint64 data, uint32 length);

OBJ make_seq_int32(SEQ_OBJ *ptr, uint32 length);
OBJ make_slice_int32(int32 *ptr, uint32 length);
OBJ make_seq_int32_inline(uint64 data, uint32 length);

OBJ make_seq_int64(SEQ_OBJ *ptr, uint32 length);
OBJ make_slice_int64(int64 *ptr, uint32 length);

// Purely physical representation functions

bool is_inline_obj(OBJ);

bool is_array_map(OBJ);
bool is_bin_tree_map(OBJ);
bool is_opt_rec(OBJ);

bool is_opt_rec_or_tag_rec(OBJ);

bool is_signed(OBJ);

bool is_array_obj(OBJ);

int intrl_cmp(OBJ, OBJ);

uint32 read_size_field_unchecked(OBJ);

INT_BITS_TAG get_int_bits_tag(OBJ);

OBJ* get_seq_elts_ptr(OBJ);
OBJ* get_set_elts_ptr(OBJ);

double *get_seq_elts_ptr_float(OBJ);

uint8 *get_seq_elts_ptr_uint8(OBJ obj);
int8  *get_seq_elts_ptr_int8(OBJ obj);
void  *get_seq_elts_ptr_int8_or_uint8(OBJ seq);
int16 *get_seq_elts_ptr_int16(OBJ obj);
int32 *get_seq_elts_ptr_int32(OBJ obj);
int64 *get_seq_elts_ptr_int64(OBJ obj);

OBJ_TYPE get_obj_type(OBJ);

SEQ_OBJ      *get_seq_ptr(OBJ);
SET_OBJ      *get_set_ptr(OBJ);
BIN_REL_OBJ  *get_bin_rel_ptr(OBJ);
TERN_REL_OBJ *get_tern_rel_ptr(OBJ);
BOXED_OBJ    *get_boxed_obj_ptr(OBJ);

void* get_opt_tag_rec_ptr(OBJ);

void* get_opt_repr_ptr(OBJ);
uint16 get_opt_repr_id(OBJ);

OBJ_TYPE get_ref_obj_type(OBJ);
void* get_ref_obj_ptr(OBJ);

bool are_shallow_eq(OBJ, OBJ);
int shallow_cmp(OBJ, OBJ);

int comp_floats(double, double);

OBJ repoint_to_copy(OBJ, void*);
OBJ repoint_to_sliced_copy(OBJ, void*);

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
void append(STREAM &s, OBJ obj);
OBJ build_seq(OBJ* elems, uint32 length);
OBJ build_set(OBJ* elems, uint32 size);
OBJ build_set(STREAM &s);
OBJ int_to_float(OBJ val);
OBJ get_seq_slice(OBJ seq, int64 idx_first, int64 len);
// OBJ update_seq_at(OBJ seq, OBJ idx, OBJ value);
OBJ rev_seq(OBJ seq);
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

OBJ build_const_seq_uint8(const uint8 *array, uint32 size);
// OBJ build_const_seq_uint16(const uint16 *array, uint32 size);
// OBJ build_const_seq_uint32(const uint32 *array, uint32 size);
OBJ build_const_seq_int8(const int8 *array, uint32 size);
OBJ build_const_seq_int16(const int16 *array, uint32 size);
OBJ build_const_seq_int32(const int32 *array, uint32 size);
OBJ build_const_seq_int64(const int64 *array, uint32 size);

OBJ build_inline_const_seq_uint8(uint64 coded_seq, uint32 size);
OBJ build_inline_const_seq_int16(uint64 coded_seq, uint32 size);
OBJ build_inline_const_seq_int32(uint64 coded_seq, uint32 size);

////////////////////////////////////////////////////////////////////////////////

uint8 inline_uint8_at(uint64 packed_elts, uint32 idx);
int16 inline_int16_at(uint64 packed_elts, uint32 idx);
int32 inline_int32_at(uint64 packed_elts, uint32 idx);

uint64 inline_uint8_init_at(uint64 packed_elts, uint32 idx, uint8 value);
uint64 inline_int16_init_at(uint64 packed_elts, uint32 idx, int16 value);
uint64 inline_int32_init_at(uint64 packed_elts, uint32 idx, int32 value);

uint64 inline_uint8_pack(uint8 *array, uint32 size);
uint64 inline_int16_pack(int16 *array, uint32 size);

uint64 inline_uint8_slice(uint64 packed_elts, uint32 idx_first, uint32 count);
uint64 inline_int16_slice(uint64 packed_elts, uint32 idx_first, uint32 count);

uint64 inline_uint8_concat(uint64 left, uint32 left_len, uint64 right, uint32 right_len);
uint64 inline_int16_concat(uint64 left, uint32 left_len, uint64 right, uint32 right_len);

//////////////////////////////// bin-rel-obj.cpp ///////////////////////////////

bool index_has_been_built(BIN_REL_OBJ *rel, uint32 size);
void build_map_right_to_left_sorted_idx_array(OBJ map); //## CHANGE THE TYPE OF map TO BIN_REL_OBJ*

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

void stable_index_sort(uint32 *indexes, uint32 count, OBJ *values);
void stable_index_sort(uint32 *indexes, uint32 count, OBJ *major_sort, OBJ *minor_sort);
void stable_index_sort(uint32 *indexes, uint32 count, OBJ *major_sort, OBJ *middle_sort, OBJ *minor_sort);

void index_sort(uint32 *indexes, uint32 count, OBJ *values);
void index_sort(uint32 *indexes, uint32 count, OBJ *major_sort, OBJ *minor_sort);
void index_sort(uint32 *indexes, uint32 count, OBJ *major_sort, OBJ *middle_sort, OBJ *minor_sort);

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
int cmp_objs(OBJ obj1, OBJ obj2);

///////////////////////////////// datetime.cpp /////////////////////////////////

void get_year_month_day(int32 epoc_days, int32 &year, int32 &month, int32 &day);
bool is_valid_date(int32 year, int32 month, int32 day);
bool date_time_is_within_range(int32 days_since_epoc, int64 day_time_ns);
int32 days_since_epoc(int32 year, int32 month, int32 day);
int64 epoc_time_ns(int32 days_since_epoc, int64 day_time_ns);

//////////////////////////////// symb-table.cpp ////////////////////////////////

const char *symb_to_raw_str(uint16);
uint16 lookup_enc_symb_id(const uint64 *, uint32);
uint16 lookup_symb_id(const char *, uint32);

/////////////////////////////// inter-utils.cpp ////////////////////////////////

OBJ /*owned_*/str_to_obj(const char* c_str);

char* obj_to_str(OBJ str_obj);
void obj_to_str(OBJ str_obj, char *buffer, uint32 size);

uint8* obj_to_byte_array(OBJ byte_seq_obj, uint32 &size);

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

const uint32 PARSER_BUFF_SIZE = 4096;

struct PARSER {
  uint32 (*read)(void *, uint8 *, uint32);
  void *read_state;

  // Byte stream state
  uint8 buffer[PARSER_BUFF_SIZE];
  uint32 offset;
  uint32 count;
  bool eof;
  bool read_error;

  // Character stream state
  int32 next_char;
  uint32 line;
  uint32 column;
};

void init_parser(PARSER *, uint32 (*)(void *, uint8 *, uint32), void *);
bool consume_non_ws_char(PARSER *, char);
bool read_label(PARSER *parser, uint16 *value);
bool parse_obj(PARSER *, OBJ *);
bool skip_value(PARSER *);

bool parse(const char *text, uint32 size, OBJ *var, uint32 *error_offset);


typedef struct {
  FILE *fp;
} READ_FILE_STATE;

uint32 read_file(void *read_state, uint8 *buffer, uint32 capacity);

///////////////////////////////// writing.cpp //////////////////////////////////

typedef struct {
  FILE *fp;
  bool success;
} WRITE_FILE_STATE;

void write_str(WRITE_FILE_STATE *, const char *);
void write_symb(WRITE_FILE_STATE *, uint16);
void write_obj(WRITE_FILE_STATE *, OBJ);
bool finish_write(WRITE_FILE_STATE *);

///////////////////////////// os-interface-xxx.cpp /////////////////////////////

uint64 get_tick_count();   // Impure

////////////////////////////////// hashing.cpp /////////////////////////////////

uint32 compute_hash_code(OBJ obj);

////////////////////////////////// concat.cpp //////////////////////////////////

bool no_sum32_overflow(uint64 x, uint64 y);

OBJ concat(OBJ left, OBJ right);
OBJ append(OBJ seq, OBJ obj);

OBJ in_place_concat_uint8(SEQ_OBJ *seq_ptr, uint32 length, uint8 *new_elts, uint32 count);
OBJ in_place_concat_obj(SEQ_OBJ *seq_ptr, uint32 length, OBJ *new_elts, uint32 count);

OBJ in_place_concat_obj_uint8(SEQ_OBJ *seq_ptr, uint32 length, uint8 *new_elts, uint32 count);

OBJ concat_uint8(uint8 *elts1, uint32 len1, uint8 *elts2, uint32 len2);
OBJ concat_obj(OBJ *elts1, uint32 len1, OBJ *elts2, uint32 len2);

OBJ concat_uint8_obj(uint8 *elts1, uint32 len1, OBJ *elts2, uint32 len2);
OBJ concat_obj_uint8(OBJ *elts1, uint32 len1, uint8 *elts2, uint32 len2);

///////////////////////////// not implemented yet //////////////////////////////

int64  get_inner_long(OBJ);

bool   *get_seq_next_frag_bool(OBJ seq, uint32 offset, bool *buffer, uint32 capacity, uint32 *count_var);
int64  *get_seq_next_frag_int64(OBJ seq, uint32 offset, int64 *buffer, uint32 capacity, uint32 *count_var);
double *get_seq_next_frag_double(OBJ seq, uint32 offset, double *buffer, uint32 capacity, uint32 *count_var);
OBJ    *get_seq_next_frag_obj(OBJ seq, uint32 offset, OBJ *buffer, uint32 capacity, uint32 *count_var);

OBJ build_seq_int64(int64* array, int32 size);
OBJ build_seq_int32(int32* array, int32 size);
OBJ build_seq_uint32(uint32* array, int32 size);
OBJ build_seq_int16(int16* array, int32 size);
OBJ build_seq_uint16(uint16* array, int32 size);
OBJ build_seq_int8(int8* array, int32 size);
OBJ build_seq_uint8(uint8* array, int32 size);

OBJ build_seq_bool(bool* array, int32 size);
OBJ build_seq_double(double* array, int32 size);

OBJ build_record(uint16 *labels, OBJ *value, int32 count);

double float_pow(double, double);
double float_sqrt(double);
int64 float_round(double);
int32 cast_int32(int64);
OBJ set_insert(OBJ, OBJ);
OBJ set_key_value(OBJ, OBJ, OBJ);
OBJ drop_key(OBJ, OBJ);
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

OBJ    get_obj_at(OBJ, int64);
double get_float_at(OBJ, int64);
int64  get_int_at(OBJ, int64);

int64 get_int_at_unchecked(OBJ seq, uint32 idx);

void copy_int64_range_unchecked(OBJ seq, uint32 start, uint32 len, int64 *buffer);
void copy_int32_range_unchecked(OBJ seq, uint32 start, uint32 len, int32 *buffer);
void copy_int16_range_unchecked(OBJ seq, uint32 start, uint32 len, int16 *buffer);
void copy_int8_range_unchecked(OBJ seq, uint32 start, uint32 len, int8 *buffer);
void copy_uint8_range_unchecked(OBJ seq, uint32 start, uint32 len, uint8 *buffer);

bool is_ne_int_seq(OBJ);
bool is_ne_float_seq(OBJ);

uint32 next_size(uint32 base_size, uint32 min_size);

OBJ copy_obj(OBJ); // mem-copying.cpp

bool needs_copying(void*);

//////////////////////////////// mem-alloc.cpp /////////////////////////////////

void switch_mem_stacks();
void unswitch_mem_stacks();
void clear_unused_mem();

///////////////////////////////// mem-core.cpp /////////////////////////////////

void switch_to_static_allocator();
void switch_to_twin_stacks_allocator();

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

#include "extern.h"
#include "mem-utils.h"
#include "basic-ops.h"
#include "seqs.h"
#include "instrs.h"
