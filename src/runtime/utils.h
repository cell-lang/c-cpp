#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdio.h>

#include <algorithm>
#include <unordered_set>
#include <unordered_map>

using std::unordered_set;
using std::unordered_map;


typedef signed   char       int8;
typedef signed   short      int16;
typedef signed   int        int32;
typedef signed   long long  int64;

typedef unsigned char       uint8;
typedef unsigned short      uint16;
typedef unsigned int        uint32;
typedef unsigned long long  uint64;

////////////////////////////////////////////////////////////////////////////////

inline bool no_sum32_overflow(uint64 x, uint64 y) {
  return x + y <= 0XFFFFFFFF;
}

inline bool is_int8(int64 value) {
  return value >= -128 & value < 128;
}

inline bool is_uint8(int64 value) {
  return value >= 0 & value < 256;
}

inline bool is_int16(int64 value) {
  return value >= -32768 & value < 32768;
}

inline bool is_uint16(int64 value) {
  return value >= 0 & value < 65536;
}

inline bool is_int32(int64 value) {
  return value >= -2147483648 & value < 2147483648;
}

inline bool is_int8_range(int64 min, int64 max) {
  return min >= -128 & max < 128;
}

inline bool is_uint8_range(int64 min, int64 max) {
  return min >= 0 & max < 256;
}

inline bool is_int8_or_uint8_range(int64 min, int64 max) {
  return (min >= 0 && max < 256) || (min >= -128 && max < 128);
}

inline bool is_int16_range(int64 min, int64 max) {
  return min >= -32768 & max < 32768;
}

inline bool is_int32_range(int64 min, int64 max) {
  return min >= -2147483648 & max < 2147483648;
}

////////////////////////////////////////////////////////////////////////////////

static bool is_digit(int32 ch) {
  return ch >= '0' & ch <= '9';
}

static bool is_hex(int32 ch) {
  return (ch >= '0' & ch <= '9') | (ch >= 'a' & ch <= 'f');
}

static bool is_lower(int32 ch) {
  return ch >= 'a' & ch <= 'z';
}

static bool is_alpha_num(int32 ch) {
  return is_digit(ch) | is_lower(ch);
}

static bool is_printable(int32 ch) {
  return ch >= ' ' & ch <= '~';
}

static bool is_white_space(int32 ch) {
  return ch == ' ' | ch == '\t' | ch == '\n' | ch == '\r';
}

static int32 hex_digit_value(int32 ch) {
  return ch - (is_digit(ch) ? '0' : 'a' - 10);
}

////////////////////////////////////////////////////////////////////////////////

static int32 min32(int32 x, int32 y) {
  return x < y ? x : y;
}

////////////////////////////////////////////////////////////////////////////////

inline bool is_nan(double value) {
  return isnan(value);
}

static uint64 bits_cast_double_uint64(double value) {
  return * (uint64 *) &value;
}

inline double bits_cast_uint64_double(uint64 value) {
  return * (double *) &value;
}

////////////////////////////////////////////////////////////////////////////////

#define lengthof(X) (sizeof(X) / sizeof((X)[0]))

////////////////////////////////////////////////////////////////////////////////

#ifndef NDEBUG
  //#define assert(_E_) (void)(_assert_((_E_), #_E_, __FILE__, __LINE__) ? 0 : (*((char *)0)) = 0)
  #define assert(_E_) (void)(_assert_((_E_), #_E_, __FILE__, __LINE__))
#else
  #define assert(_E_)
#endif

#define halt (void)(_assert_(0, "Halt reached", __FILE__, __LINE__))

bool _assert_(int exp, const char *exp_text, const char *file, int line);

////////////////////////////////////////////////////////////////////////////////

inline uint32 get_high_32(uint64 word) {
  return (uint32) (word >> 32);
}

inline uint32 get_low_32(uint64 word) {
  return (uint32) word;
}

inline uint64 pack(uint32 low, uint32 high) {
  uint64 word = (((uint64) high) << 32) | low;
  assert(get_low_32(word) == low);
  assert(get_high_32(word) == high);
  return word;
}
