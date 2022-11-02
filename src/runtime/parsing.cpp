#include "lib.h"


inline uint64 inline_uint8s_as_uint16s_pack(uint16 *array, uint32 size) {
  assert(size <= 8);

  uint64 packed_elts = 0;
  for (int i=0 ; i < size ; i++)
    packed_elts |= ((uint64) array[i]) << (8 * i);
  for (int i=0 ; i < size ; i++)
    assert(inline_uint8_at(packed_elts, i) == array[i]);
  for (int i=size ; i < 8 ; i++)
    assert(inline_uint8_at(packed_elts, i) == 0);
  return packed_elts;
}

uint16 *resize_uint16_array(uint16* array, uint32 size, uint32 new_size) {
  uint16 *new_array = new_uint16_array(new_size);
  memcpy(new_array, array, size * sizeof(uint16));
  return new_array;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

enum TOKEN_TYPE {
  TOKEN_TYPE_COMMA,
  TOKEN_TYPE_COLON,
  TOKEN_TYPE_SEMICOLON,
  TOKEN_TYPE_ARROW,
  TOKEN_TYPE_OPEN_PAR,
  TOKEN_TYPE_CLOSE_PAR,
  TOKEN_TYPE_OPEN_BRACKET,
  TOKEN_TYPE_CLOSE_BRACKET,
  TOKEN_TYPE_NUMBER,
  TOKEN_TYPE_SYMBOL,
  TOKEN_TYPE_STRING,
  TOKEN_TYPE_LITERAL,
  TOKEN_TYPE_EOF,
  TOKEN_TYPE_ERROR
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

static bool read_byte(PARSER *parser, uint8 *value) {
  uint32 count = parser->count;
  if (count == 0) {
    if (parser->eof) {
      return false;
    }
    count = parser->read(parser->read_state, parser->buffer, PARSER_BUFF_SIZE);
    if (count == 0) {
      parser->eof = true;
      return false;
    }
    // parser->count = count; // Not necessary, parser->count is set a few lines below
  }
  uint32 offset = parser->offset;
  uint8 byte = parser->buffer[offset % PARSER_BUFF_SIZE];
  offset++;
  parser->offset = offset;
  parser->count = count - 1;
  *value = byte;
  return true;
}

static bool peek_byte(PARSER *parser, uint8 *value) {
  uint32 count = parser->count;
  if (count == 0) {
    if (parser->eof) {
      return false;
    }
    count = parser->read(parser->read_state, parser->buffer, PARSER_BUFF_SIZE);
    if (count == 0) {
      parser->eof = true;
      return false;
    }
    parser->count = count;
  }
  *value = parser->buffer[parser->offset % PARSER_BUFF_SIZE];
  return true;
}

static void skip_byte(PARSER *parser) {
  assert(parser->count > 0);
  parser->offset++;
  parser->count--;
}

////////////////////////////////////////////////////////////////////////////////

// Decodes the next character in the stream and saves it in parser->next_char
// If any error (read error, invalid UTF-8) occurs parser->next_char is set to -1
static void decode_next_char(PARSER *parser) {
  uint8 byte;

  if (!read_byte(parser, &byte)) {
    assert(parser->eof || parser->read_error);
    parser->next_char = -1;
    return;
  }

  if (byte < ' ') {
    if (byte == '\n' || byte == '\t') {
      parser->next_char = byte;
    }
    else if (byte == '\r') {
      if (peek_byte(parser, &byte) && byte == '\n')
        skip_byte(parser);
      parser->next_char = '\n';
    }
    else {
      //## SAVE ERROR INFORMATION (INVALID CHARACTER IN THE STREAM)
    }
    return;
  }

  if (byte < 128) { // 0xxxxxxx
    parser->next_char = byte;
    return;
  }

  int value, shift;

  if (byte >> 5 == 6) { // 110xxxxx  10xxxxxx
    value = (byte & 0x1F) << 6;
    shift = 0;
  }
  else if (byte >> 4 == 14) { // 1110xxxx  10xxxxxx  10xxxxxx
    value = (byte & 0xF) << 12;
    shift = 6;
  }
  else if (byte >> 3 == 30) { // 11110xxx  10xxxxxx  10xxxxxx  10xxxxxx
    value = (byte & 7) << 18;
    shift = 12;
  }
  else {
    //## SAVE ERROR INFORMATION (MALFORMED UTF-8)
    parser->next_char = -1;
    return;
  }

  while (shift >= 0) {
    if (!read_byte(parser, &byte)) {
      parser->next_char = -1;
      //## SAVE ERROR INFORMATION (MALFORMED UTF-8/PREMATURE EOF)
      return;
    }
    if (byte >> 6 != 2) { // Byte must be of the form 10xxxxxx
      parser->next_char = -1;
      //## SAVE ERROR INFORMATION (MALFORMED UTF-8)
      return;
    }
    value |= (byte & 0x3F) << shift;
    shift -= 6;
  }

  parser->next_char = value;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void init_parser(PARSER *parser, uint32 (*read)(void *, uint8 *, uint32), void *read_state) {
  parser->read = read;
  parser->read_state = read_state;

  parser->offset = 0;
  parser->count = 0;
  parser->eof = false;
  parser->read_error = false;

  parser->line = 0;
  parser->column = 0;

  decode_next_char(parser);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

static bool read_char(PARSER *parser, int32 *value) {
  int32 next_char = parser->next_char;
  if (next_char >= 0) {
    *value = next_char;
    if (next_char == '\n') {
      parser->line++;
      parser->column = 0;
    }
    else
      parser->column++;
    decode_next_char(parser);
    return true;
  }
  else {
    *value = -1;
    return false;
  }
}

static bool peek_char(PARSER *parser, int32 *value) {
  int32 next_char = parser->next_char;
  *value = next_char;
  return value >= 0;
}

static bool next_char_is(PARSER *parser, char exp_char) {
  return parser->next_char == exp_char;
}

static bool peek_byte_after_next_char(PARSER *parser, uint8 *byte) {
  return peek_byte(parser, byte);
}

static void skip_char(PARSER *parser) {
  int32 unused;
  bool ok = read_char(parser, &unused);
  assert(ok);
}

static void skip_chars(PARSER *parser, uint32 count) {
  for (int i=0 ; i < count ; i++)
    skip_char(parser);
}

static bool read_char_no_eof(PARSER *parser, int32 *value) {
  bool ok = read_char(parser, value);
  if (!ok && parser->eof) {
      //## SAVE ERROR INFORMATION (PREMATURE EOF)
  }
  return ok;
}

static bool consume_char(PARSER *parser, char exp_char) {
  int32 next_char;
  if (peek_char(parser, &next_char) && next_char == exp_char) {
    skip_char(parser);
    return true;
  }
  else
    return false;
}

static bool read_digit(PARSER *parser, int32 *digit) {
  int32 next_char = parser->next_char;
  if (is_digit(next_char)) {
    skip_char(parser);
    *digit = next_char - '0';
    return true;
  }
  else
    return false;
}

static bool read_2_digits(PARSER *parser, int32 min, int32 max, int32 *hours) {

}

////////////////////////////////////////////////////////////////////////////////

static uint32 line(PARSER *parser) {
  return parser->line;
}

static uint32 column(PARSER *parser) {
  return parser->column;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

static void consume_ws(PARSER *parser) {
  int32 maybe_char;
  while (peek_char(parser, &maybe_char) && is_white_space(maybe_char))
    skip_char(parser);
}

bool consume_non_ws_char(PARSER *parser, char ch) {
  consume_ws(parser);
  return consume_char(parser, ch);
}

bool consume_non_ws_chars(PARSER *parser, char ch1, char ch2) {
  consume_ws(parser);
  int32 maybe_char;
  if (peek_char(parser, &maybe_char) && maybe_char == ch1) {
    uint8 byte;
    if (peek_byte_after_next_char(parser, &byte) && byte == ch2) {
      bool success = consume_char(parser, ch1);
      assert(success);
      success = consume_char(parser, ch2);
      assert(success);
      return true;
    }
  }
  return false;
}

bool next_non_ws_char_is(PARSER *parser, char ch) {
  consume_ws(parser);
  return next_char_is(parser, ch);
}

// Can be called only after read_char() or peek_char() return false
bool eof(PARSER *parser) {
  assert(parser->next_char == -1);
  return parser->eof;
}

////////////////////////////////////////////////////////////////////////////////

static TOKEN_TYPE peek_token_type(PARSER *parser) {
  consume_ws(parser);

  int32 maybe_char;
  if (peek_char(parser, &maybe_char)) {
    if (is_digit(maybe_char))
      return TOKEN_TYPE_NUMBER;

    if (is_lower(maybe_char))
      return TOKEN_TYPE_SYMBOL;

    if (maybe_char == '-') {
      uint8 byte;
      if (peek_byte_after_next_char(parser, &byte)) {
        if (byte == '>')
          return TOKEN_TYPE_ARROW;
        if (is_digit(byte))
          return TOKEN_TYPE_NUMBER;
      }
      return TOKEN_TYPE_ERROR;
    }

    switch (maybe_char) {
        case '"':     return TOKEN_TYPE_STRING;
        case ',':     return TOKEN_TYPE_COMMA;
        case ':':     return TOKEN_TYPE_COLON;
        case ';':     return TOKEN_TYPE_SEMICOLON;
        case '(':     return TOKEN_TYPE_OPEN_PAR;
        case ')':     return TOKEN_TYPE_CLOSE_PAR;
        case '[':     return TOKEN_TYPE_OPEN_BRACKET;
        case ']':     return TOKEN_TYPE_CLOSE_BRACKET;
        case '`':     return TOKEN_TYPE_LITERAL;
        default:      return TOKEN_TYPE_ERROR;
    }
  }
  else if (eof(parser)) {
    return TOKEN_TYPE_EOF;
  }
  else {
    return TOKEN_TYPE_ERROR;
  }
}

////////////////////////////////////////////////////////////////////////////////

static bool read_int64(PARSER *parser, int64 *value) {
  bool negative = consume_non_ws_char(parser, '-');

  int64 partial_value = 0;
  for (int i=0 ; ; i++) {
    int32 maybe_char;
    if (!peek_char(parser, &maybe_char)) {
      if (eof(parser) && i > 0)
        break;
      return false;
    }

    if (!is_digit(maybe_char)) {
      if (i > 0)
        break;
      //## SAVE ERROR INFORMATION
      return false;
    }

    skip_char(parser);
    int digit = maybe_char - '0';

    // Checking for overflows
    if (partial_value >= 922337203685477580L) {
      if (partial_value == 922337203685477580L) {
        // No more digits possible after this one
        if (peek_char(parser, &maybe_char) && is_digit(maybe_char)) {
          //## SAVE ERROR INFORMATION
          return false;
        }

        if (negative) {
          if (digit <= 8) {
            *value = -10 * partial_value - digit;
            return true;
          }
        }
        else {
          if (digit <= 7) {
            *value = 10 * partial_value + digit;
            return true;
          }
        }
      }

      //## SAVE ERROR INFORMATION
      return false;
    }

    partial_value = 10 * partial_value + digit;
  }

  *value = negative ? -partial_value : partial_value;
  return true;
}

////////////////////////////////////////////////////////////////////////////////

// Returns 0 on error, 2 on success
static int32 read_exp_double(PARSER *parser, double unexp_value, double *value) {
  int64 exp;
  if (read_int64(parser, &exp)) {
    *value = unexp_value * pow(10, exp);
    return 2;
  }
  else {
    return 0;
  }
}


// Returns 0 on error, 2 on success
static int32 read_double(PARSER *parser, bool negative, double int_part, double *value) {
  double partial_value = int_part;
  double weight = 0.1;
  for ( ; ; ) {
    int32 maybe_char;
    if (!peek_char(parser, &maybe_char)) {
      if (eof(parser))
        break;
      return 0;
    }

    if (maybe_char == 'e') {
      skip_char(parser);
      return read_exp_double(parser, partial_value, value);
    }

    if (!is_digit(maybe_char))
      break;

    skip_char(parser);
    int digit = maybe_char - '0';
    partial_value += digit * weight;
    weight *= 0.1;
  }

  *value = negative ? -partial_value : partial_value;
  return 2;
}

// Returns 0 on error, 2 on success
static int32 read_very_long_double(PARSER *parser, bool negative, double partial_value, double *value) {
  for ( ; ; ) {
    int32 maybe_char;
    if (!peek_char(parser, &maybe_char)) {
      if (eof(parser)) {
        //## SAVE ERROR INFORMATION (TOO LARGE INTEGER)
      }
      return 0;
    }

    if (maybe_char == '.') {
      skip_char(parser);
      return read_double(parser, negative, partial_value, value);
    }

    if (maybe_char == 'e') {
      skip_char(parser);
      return read_exp_double(parser, negative ? -partial_value : partial_value, value);
    }

    if (!is_digit(maybe_char)) {
      //## SAVE ERROR INFORMATION (TOO LARGE INTEGER)
      return 0;
    }

    int32 digit = maybe_char - '0';
    partial_value = 10 * partial_value + digit;
  }
}

// Returns 0 on error, 1 on reading an integer and 2 on reading a double
static int32 read_number(PARSER *parser, int64 *int64_value, double *double_value) {
  bool negative = consume_non_ws_char(parser, '-');

  int64 partial_value = 0;
  for (int i=0 ; ; i++) {
    int32 maybe_char;
    if (!peek_char(parser, &maybe_char)) {
      if (eof(parser) && i > 0)
        break;
      return 0;
    }

    if (!is_digit(maybe_char)) {
      if (maybe_char == '.') {
        skip_char(parser);
        return read_double(parser, negative, partial_value, double_value);
      }

      if (i == 0) {
        //## SAVE ERROR INFORMATION
        return 0;
      }

      if (maybe_char == 'e') {
        skip_char(parser);
        return read_exp_double(parser, negative ? -partial_value : partial_value, double_value);
      }

      break;
    }

    skip_char(parser);
    int digit = maybe_char - '0';

    // Checking for overflows
    if (partial_value >= 922337203685477580L) {
      if (partial_value == 922337203685477580L) {
        if (!peek_char(parser, &maybe_char) || !is_digit(maybe_char)) {
          if (negative) {
            if (digit <= 8) {
              *int64_value = -10 * partial_value - digit;
              return 1;
            }
          }
          else {
            if (digit <= 7) {
              *int64_value = 10 * partial_value + digit;
              return 1;
            }
          }
        }
      }

      return read_very_long_double(parser, negative, 10.0 * partial_value + digit, double_value);
    }

    partial_value = 10 * partial_value + digit;
  }

  *int64_value = negative ? -partial_value : partial_value;
  return 1;
}

////////////////////////////////////////////////////////////////////////////////

//  0         Empty
//  1 - 26    Letter
// 27 - 36    Digit
// 37         Underscore (followed by a digit)
// 38 - 63    Underscore + letter


static uint64 encoded_letter(uint8 ch) {
  return ch - 'a' + 1;
}

static uint64 encoded_underscored_letter(uint8 ch) {
  return ch - 'a' + 38;
}

static uint64 encoded_digit(uint8 ch) {
  return ch - '0' + 27;
}

const uint64 encoded_underscore = 37;


// Returns the number of encoded words read, or -1 on error
static int32 read_enc_symb_words(PARSER *parser, uint64 *enc_word, uint32 size) {
  int32 next_char;

  assert(peek_char(parser, &next_char) && is_lower(next_char));

  for (int i=0 ; i < size ; i++) {
    uint64 part_enc_word = 0;

    for (int j=0 ; j < 10 ; j++) {
      if (!peek_char(parser, &next_char)) {
        if (eof(parser)) {
          enc_word[i] = part_enc_word;
          return i + 1;
        }
        return -1;
      }

      if (is_lower(next_char)) {
        skip_char(parser);
        uint64 code = encoded_letter(next_char);
        part_enc_word = (part_enc_word << 6) + code;
      }
      else if (next_char == '_') {
        skip_char(parser);
        if (!peek_char(parser, &next_char)) {
          if (eof(parser)) {
            //## SAVE ERROR INFORMATION
          }
          return -1;
        }
        if (is_lower(next_char)) {
          skip_char(parser);
          uint64 code = encoded_underscored_letter(next_char);
          part_enc_word = (part_enc_word << 6) + code;
        }
        else if (is_digit(next_char)) {
          part_enc_word = (part_enc_word << 6) + encoded_underscore;
        }
        else {
          //## SAVE ERROR INFORMATION
          return -1;
        }
      }
      else if (is_digit(next_char)) {
        skip_char(parser);
        uint64 code = encoded_digit(next_char);
        part_enc_word = (part_enc_word << 6) + code;
      }
      else {
        if (j > 0)
          enc_word[i++] = part_enc_word;
        return i;
      }
    }

    enc_word[i] = part_enc_word;
  }

  //## SAVE ERROR INFORMATION
  return -1;
}

static bool read_symbol(PARSER *parser, uint16 *value) {
  int32 next_char;

  assert(peek_char(parser, &next_char) && is_lower(next_char));

  uint64 enc_words[64];
  int32 count = read_enc_symb_words(parser, enc_words, 64);

  if (count > 0) {
    *value = lookup_enc_symb_id(enc_words, count);
    return true;
  }
  else
    return false;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

static bool read_string(PARSER *parser, OBJ *value) {
  assert(next_char_is(parser, '"'));

  const int INIT_CAPACITY = 4096;
  uint32 length = 0;
  uint16 buffer[INIT_CAPACITY];
  uint32 capacity = INIT_CAPACITY;
  uint16 *chars = buffer;
  uint16 max_char = 0;

  skip_char(parser);

  for ( ; ; ) {
    int32 next_char;
    if (!read_char_no_eof(parser, &next_char))
      return false;

    // Standard ASCII character
    if (next_char == '\\') {
      if (!read_char_no_eof(parser, &next_char))
        return false;

      if (next_char == '\\' || next_char == '"') {
        // Nothing to do here
      }
      else if (next_char == 'n') {
        next_char = '\n';
      }
      else if (next_char == 't') {
        next_char = '\t';
      }
      else if (next_char == 'r') {
        next_char = '\r';
      }
      else if (is_hex(next_char)) {
        next_char = hex_digit_value(next_char) << 12;
        for (int shift=8 ; shift >= 0 ; shift -= 4) {
          int32 ch;
          if (!read_char_no_eof(parser, &ch))
            return false;
          if (!is_hex(ch)) {
            //## SAVE ERROR INFORMATION
            return false;
          }
          next_char += hex_digit_value(ch) << shift;
        }
      }
      else {
        //## SAVE ERROR INFORMATION
        return false;
      }
    }
    else if (next_char == '"') {
      break;
    }

    if (next_char > max_char)
      max_char = next_char;

    if (length > capacity) {
      //## IMPLEMENT IMPLEMENT IMPLEMENT
      internal_fail();
    }

    chars[length++] = next_char;
  }

  OBJ seq;

  if (length == 0) {
    seq = make_empty_seq();
  }
  else if (is_uint8(max_char)) {
    if (length > 8) {
      SEQ_OBJ *seq_ptr = new_uint8_seq(length);
      uint8 *array = seq_ptr->buffer.uint8_;
      for (uint32 i=0 ; i < length ; i++)
        array[i] = chars[i];
      seq = make_seq_uint8(seq_ptr, length);
    }
    else
      seq = make_seq_uint8_inline(inline_uint8s_as_uint16s_pack(chars, length), length);
  }
  else if (is_int16(max_char)) {
    if (length > INIT_CAPACITY) {
      seq = make_slice_int16((int16 *) chars, length);
    }
    else if (length > 4) {
      SEQ_OBJ *seq_ptr = new_int16_seq(length);
      int16 *array = seq_ptr->buffer.int16_;
      memcpy(seq_ptr, chars, length * sizeof(uint16));
    }
    else
      seq = make_seq_int16_inline(inline_int16_pack((int16 *) chars, length), length);
  }
  else {
    if (length > 2) {
      SEQ_OBJ *seq_ptr = new_int32_seq(length);
      int32 *array = seq_ptr->buffer.int32_;
      for (uint32 i=0 ; i < length ; i++)
        array[i] = chars[i];
      seq = make_seq_int32(seq_ptr, length);
    }
    else {
      uint64 data = inline_int32_init_at(0, 0, chars[0]);
      if (length == 2)
        data = inline_int32_init_at(data, 1, chars[1]);
      seq = make_seq_int32_inline(data, length);
    }
  }

  *value = make_tag_obj(symb_id_string, seq);
  return true;
}

static bool read_literal(PARSER *parser, OBJ *value) {
  assert(next_char_is(parser, '`'));

  skip_char(parser);

  int32 char_1, char_2;
  if (!read_char_no_eof(parser, &char_1) || !read_char_no_eof(parser, &char_2))
    return false;

  if (char_1 == '\\') {
    if (!consume_char(parser, '`'))
      return false;

    if (char_2 == 'n') {
      *value = make_int('\n');
      return true;
    }
    else if (char_2 == '`') {
      *value = make_int('`');
      return true;
    }
    else if (char_2 == 't') {
      *value = make_int('\t');
      return true;
    }
    else if (char_2 == '\\') {
      *value = make_int('\\');
      return true;
    }
    else {
      //## SAVE ERROR INFORMATION
      return false;
    }
  }
  else if (char_2 == '`') {
    *value = make_int(char_1);
    return true;
  }
  else {
    if (!is_digit(char_1) || !is_digit(char_2)) {
      //## SAVE ERROR INFORMATION
      return true;
    }

    int32 d1 = char_1 - '0';
    int32 d2 = char_2 - '0';
    int32 d3, d4;
    if (!read_digit(parser, &d3) || !read_digit(parser, &d4))
      return false;

    int32 year = 1000 * d1 + 100 * d2 + 10 * d3 + d4;

    if (!consume_char(parser, '-') || !read_digit(parser, &d1) || !read_digit(parser, &d2))
      return false;

    int32 month = 10 * d1 + d2;

    if (!consume_char(parser, '-') || !read_digit(parser, &d1) || !read_digit(parser, &d2))
      return false;

    int32 day = 10 * d1 + d2;

    if (!is_valid_date(year, month, day)) {
      //## SAVE ERROR INFORMATION (REMEMBER TO SET THE LOCATION AT THE BEGINNING)
      return false;
    }

    int32 epoc_days = days_since_epoc(year, month, day);

    if (consume_char(parser, '`')) {
      *value = make_tag_obj(symb_id_date, make_int(epoc_days));
      return true;
    }

    int32 hours, minutes, seconds;
    bool ok = consume_char(parser, ' ') &&
              read_2_digits(parser, 0, 23, &hours) &&
              consume_char(parser, ':') &&
              read_2_digits(parser, 0, 59, &minutes) &&
              consume_char(parser, ':') &&
              read_2_digits(parser, 0, 59, &seconds);
    if (!ok)
      return false;

    int32 nanosecs = 0;
    if (consume_char(parser, '.')) {
      int pow10 = 100000000;
      for (int i=0 ; i < 9 ; i++) {
        if (!read_digit(parser, &d1))
          break;
        nanosecs += pow10 * d1;
        pow10 /= 10;
      }
    }

    if (!consume_char(parser, '`'))
      return false;

    uint64 day_time_ns = 1000000000L * (60L * (60L * hours + minutes) + seconds) + nanosecs;
    if (!date_time_is_within_range(epoc_days, day_time_ns)) {
      //## SAVE ERROR INFORMATION
      return false;
    }

    *value = make_tag_obj(symb_id_time, make_int(epoc_time_ns(epoc_days, day_time_ns)));
    return true;
  }
}

bool read_label(PARSER *parser, uint16 *value) {
  consume_ws(parser);
  return read_symbol(parser, value) && consume_non_ws_char(parser, ':'); //## SHOULD IT BE consume_char(..)?
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

const uint32 INIT_CAPACITY = 64;


static bool finish_parsing_record(PARSER *parser, uint16 first_label_id, OBJ *result) {
  OBJ obj;
  uint16 labels_array[INIT_CAPACITY];
  OBJ objs_array[INIT_CAPACITY];

  if (!parse_obj(parser, &obj))
    return false;

  labels_array[0] = first_label_id;
  objs_array[0] = obj;

  uint16 *labels = labels_array;
  OBJ *objs = objs_array;
  uint32 count = 1;
  uint32 capacity = INIT_CAPACITY;

  for ( ; ; ) {
    if (consume_non_ws_char(parser, ')')) {
      *result = build_record(labels, objs, count);
      return true;
    }

    if (!consume_non_ws_char(parser, ',')) {
      //## SAVE ERROR INFORMATION?
      return false;
    }

    uint16 label_id;
    if (!read_label(parser, &label_id)) {
      //## SAVE ERROR INFORMATION?
      return false;
    }

    if (!parse_obj(parser, &obj))
      return false;

    if (count == capacity) {
      labels = resize_uint16_array(labels, capacity, 2 * capacity);
      objs = resize_obj_array(objs, capacity, 2 * capacity);
      capacity *= 2;
    }

    labels[count] = label_id;
    objs[count++] = obj;
  }
}

static bool finish_parsing_sequence(PARSER *parser, OBJ first_elt, OBJ *result) {
  OBJ array[INIT_CAPACITY];
  OBJ *elts = array;
  uint32 count = 1;
  uint32 capacity = INIT_CAPACITY;

  elts[0] = first_elt;

  for ( ; ; ) {
    if (consume_non_ws_char(parser, ')')) {
      if (elts == array) {
        elts = new_obj_array(count);
        memcpy(elts, array, count * sizeof(OBJ));
      }
      *result = build_seq(elts, count);
      return true;
    }

    if (count > 1 && !consume_non_ws_char(parser, ','))
      return false;

    OBJ obj;
    if (!parse_obj(parser, &obj))
      return false;

    if (count == capacity) {
      elts = resize_obj_array(elts, capacity, 2 * capacity);
      capacity *= 2;
    }

    elts[count++] = obj;
  }
}

static bool parse_seq_after_open_par(PARSER *parser, OBJ *result) {
  OBJ array[INIT_CAPACITY];
  OBJ *elts = array;
  int32 capacity = INIT_CAPACITY;
  int32 size = 0;

  for ( ; ; ) {
    consume_ws(parser);

    if (consume_non_ws_char(parser, ')')) {
      if (elts == array) {
        elts = new_obj_array(size);
        memcpy(elts, array, size * sizeof(OBJ));
      }
      *result = build_seq(elts, size);
      return true;
    }

    if (size > 0 && !consume_non_ws_char(parser, ','))
      return false;

    consume_ws(parser);

    OBJ elt;
    if (!parse_obj(parser, &elt))
      return false;

    if (size == capacity) {
      OBJ *elts = resize_obj_array(elts, capacity, 2 * capacity);
      capacity *= 2;
    }

    elts[size++] = elt;
  }
}

static bool parse_seq_or_record(PARSER *, OBJ *);

static bool parse_tagged_obj(PARSER *parser, uint16 tag_id, OBJ *result) {
  OBJ obj;

  assert(next_char_is(parser, '('));

  if (!parse_seq_or_record(parser, &obj))
    return false;
  if (is_seq(obj) && get_size(obj) == 1)
    obj = get_obj_at(obj, 0);
  *result = opt_repr_build(make_symb(tag_id), obj);
  return true;
}

static bool parse_seq_or_record(PARSER *parser, OBJ *result) {
  assert(next_char_is(parser, '('));

  skip_char(parser);

  TOKEN_TYPE next_token_type = peek_token_type(parser);
  if (next_token_type == TOKEN_TYPE_SYMBOL) {
    uint16 symb_id;
    if (!read_symbol(parser, &symb_id))
      return false;

    if (consume_non_ws_char(parser, ':'))
      return finish_parsing_record(parser, symb_id, result);

    if (consume_non_ws_char(parser, ','))
      return finish_parsing_sequence(parser, make_symb(symb_id), result);

    if (consume_non_ws_char(parser, ')')) {
      OBJ only_elt = make_symb(symb_id);
      OBJ *elts = new_obj_array(1);
      elts[0] = only_elt;
      *result = build_seq(elts, 1);
      return true;
    }

    if (next_char_is(parser, '(')) {
      OBJ first_elt;
      if (!parse_tagged_obj(parser, symb_id, &first_elt))
        return false;
      if (!consume_non_ws_char(parser, ','))
        return false;
      return finish_parsing_sequence(parser, first_elt, result);
    }

    //## SAVE ERROR INFORMATION
    return false;
  }

  return parse_seq_after_open_par(parser, result);
}

static bool finish_parsing_bin_rel(PARSER *parser, OBJ arg0, OBJ arg1, OBJ *result) {
  if (consume_non_ws_char(parser, ']')) {
    *result = build_bin_rel(&arg0, &arg1, 1);
    return true;
  }

  OBJ array0[INIT_CAPACITY];
  OBJ array1[INIT_CAPACITY];
  OBJ *col0 = array0;
  OBJ *col1 = array1;
  col0[0] = arg0;
  col1[0] = arg1;
  uint32 count = 1;
  uint32 capacity = INIT_CAPACITY;

  for ( ; ; ) {
    if (!parse_obj(parser, &arg0) || !consume_non_ws_char(parser, ',') || !parse_obj(parser, &arg1))
      return false;

    if (count == capacity) {
      col0 = resize_obj_array(col0, capacity, 2 * capacity);
      col1 = resize_obj_array(col1, capacity, 2 * capacity);
      capacity *= 2;
    }

    col0[count] = arg0;
    col1[count++] = arg1;

    if (consume_non_ws_char(parser, ']')) {
      *result = build_bin_rel(col0, col1, count);
      return true;
    }

    if (!consume_non_ws_char(parser, ';'))
      return false;
  }
}

static bool finish_parsing_tern_rel(PARSER *parser, OBJ arg0, OBJ arg1, OBJ arg2, OBJ *result) {
  if (consume_non_ws_char(parser, ']')) {
    *result = build_tern_rel(&arg0, &arg1, &arg2, 1);
    return true;
  }

  OBJ array0[INIT_CAPACITY];
  OBJ array1[INIT_CAPACITY];
  OBJ array2[INIT_CAPACITY];
  OBJ *col0 = array0;
  OBJ *col1 = array1;
  OBJ *col2 = array2;
  col0[0] = arg0;
  col1[0] = arg1;
  col2[0] = arg2;
  uint32 count = 1;
  uint32 capacity = INIT_CAPACITY;

  for ( ; ; ) {
    if (!parse_obj(parser, &arg0) || !consume_non_ws_char(parser, ',') || !parse_obj(parser, &arg1) || !consume_non_ws_char(parser, ',') || !parse_obj(parser, &arg2))
      return false;

    if (count == capacity) {
      col0 = resize_obj_array(col0, capacity, 2 * capacity);
      col1 = resize_obj_array(col1, capacity, 2 * capacity);
      col2 = resize_obj_array(col2, capacity, 2 * capacity);
      capacity *= 2;
    }

    col0[count] = arg0;
    col1[count] = arg1;
    col2[count++] = arg2;

    if (consume_non_ws_char(parser, ']')) {
      *result = build_tern_rel(col0, col1, col2, count);
      return true;
    }

    if (!consume_non_ws_char(parser, ';'))
      return false;
  }
}

static bool finish_parsing_map(PARSER *parser, OBJ first_key, OBJ *result) {
  OBJ first_value;
  if (!parse_obj(parser, &first_value))
    return false;

  OBJ keys_array[INIT_CAPACITY];
  OBJ values_array[INIT_CAPACITY];
  OBJ *keys = keys_array;
  OBJ *values = values_array;
  keys[0] = first_key;
  values[0] = first_value;
  uint32 count = 1;
  uint32 capacity = INIT_CAPACITY;

  while (consume_non_ws_char(parser, ',')) {
    OBJ key, value;
    if (!parse_obj(parser, &key) || !consume_non_ws_char(parser, '-') || !consume_char(parser, '>') || !parse_obj(parser, &value))
      return false;

    if (count == capacity) {
      keys = resize_obj_array(keys, capacity, 2 * capacity);
      values = resize_obj_array(values, capacity, 2 * capacity);
      capacity *= 2;
    }

    keys[count] = key;
    values[count++] = value;
  }

  if (!consume_non_ws_char(parser, ']'))
    return false;

  *result = build_bin_rel(keys, values, count);
  return true;
}

static bool parse_set_or_relation(PARSER *parser, OBJ *result) {
  assert(next_char_is(parser, '['));

  skip_char(parser);
  if (consume_non_ws_char(parser, ']')) {
    *result = make_empty_rel();
    return true;
  }

  OBJ array[INIT_CAPACITY];
  OBJ *elts = array;
  uint32 count = 0;
  uint32 capacity = INIT_CAPACITY;

  for ( ; ; ) {
    OBJ obj;
    if (!parse_obj(parser, &obj))
      return false;

    if (count == capacity) {
      elts = resize_obj_array(elts, capacity, 2 * capacity);
      capacity *= 2;
    }

    elts[count++] = obj;

    if (consume_non_ws_char(parser, ']')) {
      if (count <= capacity) {
        OBJ *dynamic_array = new_obj_array(count);
        memcpy(dynamic_array, elts, count * sizeof(OBJ));
        elts = dynamic_array;
      }
      *result = build_set_in_place(elts, count);
      return true;
    }

    if (consume_non_ws_char(parser, ';')) {
      if (count == 2)
        return finish_parsing_bin_rel(parser, elts[0], elts[1], result);

      if (count == 3)
        return finish_parsing_tern_rel(parser, elts[0], elts[1], elts[2], result);

      return false;
    }

    if (consume_non_ws_char(parser, '-')) {
      if (count != 1 || !consume_char(parser, '>'))
        return false;
      return finish_parsing_map(parser, elts[0], result);
    }

    if (!consume_non_ws_char(parser, ','))
      return false;
  }
}

bool parse_obj(PARSER *parser, OBJ *result) {
  switch (peek_token_type(parser)) {
    case TOKEN_TYPE_COMMA:
    case TOKEN_TYPE_COLON:
    case TOKEN_TYPE_SEMICOLON:
    case TOKEN_TYPE_ARROW:
    case TOKEN_TYPE_CLOSE_PAR:
    case TOKEN_TYPE_CLOSE_BRACKET:
      //## SAVE ERROR INFORMATION

    case TOKEN_TYPE_EOF:
    case TOKEN_TYPE_ERROR:
      return false;

    case TOKEN_TYPE_NUMBER: {
      int64 int64_value;
      double double_value;
      int32 res = read_number(parser, &int64_value, &double_value);
      if (res == 0)
        return false;
      assert(res == 1 || res == 2);
      *result = res == 1 ? make_int(int64_value) : make_float(double_value);
      return true;
    }

    case TOKEN_TYPE_SYMBOL: {
      uint16 symb_id;
      if (!read_symbol(parser, &symb_id))
        return false;
      consume_ws(parser); //## NOT SURE HERE
      if (next_char_is(parser, '('))
        return parse_tagged_obj(parser, symb_id, result);
      *result = make_symb(symb_id);
      return true;
    }

    case TOKEN_TYPE_LITERAL:
      return read_literal(parser, result);

    case TOKEN_TYPE_STRING:
      return read_string(parser, result);

    case TOKEN_TYPE_OPEN_PAR:
      return parse_seq_or_record(parser, result);

    case TOKEN_TYPE_OPEN_BRACKET:
      return parse_set_or_relation(parser, result);

    default:
      internal_fail();
  }
}

bool skip_value(PARSER *parser) {
  OBJ obj = make_blank_obj();
  return parse_obj(parser, &obj);
}

////////////////////////////////////////////////////////////////////////////////

typedef struct {
  const char *text;
  uint32 length;
  uint32 offset;
} READ_STR_STATE;

static uint32 read_str(void *read_state, uint8 *buffer, uint32 capacity) {
  READ_STR_STATE *reader = (READ_STR_STATE *) read_state;
  assert(reader->offset >= 0 && reader->offset <= reader->length);
  uint32 offset = reader->offset;
  uint32 remaining = reader->length - offset;
  uint32 read_size = remaining < capacity ? remaining : capacity;
  memcpy(buffer, reader->text + offset, read_size);
  reader->offset = offset + read_size;
  return read_size;
}

bool parse(const char *text, uint32 size, OBJ *var, uint32 *error_offset) {
  READ_STR_STATE read_str_state;
  PARSER parser;

  read_str_state.text = text;
  read_str_state.length = size;
  read_str_state.offset = 0;
  init_parser(&parser, read_str, &read_str_state);

  if (!parse_obj(&parser, var)) {
    *error_offset = parser.offset;
    return false;
  }

  for (int i=read_str_state.offset ; i < size ; i++)
    if (!is_white_space(text[i])) {
      *error_offset = i;
      return false;
    }

  return true;
}

////////////////////////////////////////////////////////////////////////////////

uint32 read_file(void *read_state, uint8 *buffer, uint32 capacity) {
  READ_FILE_STATE *state = (READ_FILE_STATE *) read_state;
  return fread(buffer, 1, capacity, state->fp);
}
