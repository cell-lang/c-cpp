#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <stdio.h>



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

inline bool is_int16_range(int64 min, int64 max) {
  return min >= -32768 & max < 32768;
}

inline bool is_int32_range(int64 min, int64 max) {
  return min >= -2147483648 & max < 2147483648;
}

////////////////////////////////////////////////////////////////////////////////

#ifndef NDEBUG
  //#define assert(_E_) (void)(_assert_((_E_), #_E_, __FILE__, __LINE__) ? 0 : (*((char *)0)) = 0)
  #define assert(_E_) (void)(_assert_((_E_), #_E_, __FILE__, __LINE__))
#else
  #define assert(_E_)
#endif

#define halt (void)(_assert_(0, "Halt reached", __FILE__, __LINE__))

bool _assert_(int exp, const char *exp_text, const char *file, int line);
