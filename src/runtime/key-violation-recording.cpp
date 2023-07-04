#include "lib.h"


void single_key_bin_table_aux_record_key_violation_1(SINGLE_KEY_BIN_TABLE *table, SINGLE_KEY_BIN_TABLE_AUX *table_aux, char *err_msg, const char *table_name) {
  snprintf(err_msg, 256, "The key on the first column of %s was violated", table_name);
}

void double_key_bin_table_aux_record_key_violation_1(DOUBLE_KEY_BIN_TABLE *table, DOUBLE_KEY_BIN_TABLE_AUX *table_aux, char *err_msg, const char *table_name) {
  snprintf(err_msg, 256, "The key on the first column of %s was violated", table_name);
}

void double_key_bin_table_aux_record_key_violation_2(DOUBLE_KEY_BIN_TABLE *table, DOUBLE_KEY_BIN_TABLE_AUX *table_aux, char *err_msg, const char *table_name) {
  snprintf(err_msg, 256, "The key on the second column of %s was violated", table_name);
}

void double_key_bin_table_aux_record_key_violation(DOUBLE_KEY_BIN_TABLE *table, DOUBLE_KEY_BIN_TABLE_AUX *table_aux, char *err_msg, const char *table_name) {
  snprintf(err_msg, 256, "One of the keys of %s was violated", table_name);
}

void obj_col_aux_record_key_violation_1(OBJ_COL *table, OBJ_COL_AUX *table_aux, char *err_msg, const char *table_name) {
  snprintf(err_msg, 256, "The key on the first column of %s was violated", table_name);
}

void int_col_aux_record_key_violation_1(INT_COL *table, INT_COL_AUX *table_aux, char *err_msg, const char *table_name) {
  snprintf(err_msg, 256, "The key on the first column of %s was violated", table_name);
}

void float_col_aux_record_key_violation_1(FLOAT_COL *table, FLOAT_COL_AUX *table_aux, char *err_msg, const char *table_name) {
  snprintf(err_msg, 256, "The key on the first column of %s was violated", table_name);
}

void obj_col_aux_record_key_violation_12(OBJ_COL *table, OBJ_COL_AUX *table_aux, char *err_msg, const char *table_name) {
  snprintf(err_msg, 256, "The key on the first two columns of %s was violated", table_name);
}

void int_col_aux_record_key_violation_12(INT_COL *table, INT_COL_AUX *table_aux, char *err_msg, const char *table_name) {
  snprintf(err_msg, 256, "The key on the first two columns of %s was violated", table_name);
}

void float_col_aux_record_key_violation_12(FLOAT_COL *table, FLOAT_COL_AUX *table_aux, char *err_msg, const char *table_name) {
  snprintf(err_msg, 256, "The key on the first two columns of %s was violated", table_name);
}

void raw_obj_col_aux_record_key_violation_1(RAW_OBJ_COL *table, OBJ_COL_AUX *table_aux, char *err_msg, const char *table_name) {
  snprintf(err_msg, 256, "The key on the first column of %s was violated", table_name);
}

void raw_int_col_aux_record_key_violation_1(RAW_INT_COL *table, INT_COL_AUX *table_aux, char *err_msg, const char *table_name) {
  snprintf(err_msg, 256, "The key on the first column of %s was violated", table_name);
}

void raw_float_col_aux_record_key_violation_1(RAW_FLOAT_COL *table, FLOAT_COL_AUX *table_aux, char *err_msg, const char *table_name) {
  snprintf(err_msg, 256, "The key on the first column of %s was violated", table_name);
}

void slave_tern_table_aux_record_key_violation_12(BIN_TABLE *table, SLAVE_TERN_TABLE_AUX *table_aux, char *err_msg, const char *table_name) {
  snprintf(err_msg, 256, "The key on the first two columns of %s was violated", table_name);
}

void slave_tern_table_aux_record_key_violation_3(BIN_TABLE *table, SLAVE_TERN_TABLE_AUX *table_aux, char *err_msg, const char *table_name) {
  snprintf(err_msg, 256, "The key on the third column of %s was violated", table_name);
}

void tern_table_aux_record_key_violation_12(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, char *err_msg, const char *table_name) {
  snprintf(err_msg, 256, "The key on the first two columns of %s was violated", table_name);
}

void tern_table_aux_record_key_violation_13(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, char *err_msg, const char *table_name) {
  snprintf(err_msg, 256, "The key on the first and third columns of %s was violated", table_name);
}

void tern_table_aux_record_key_violation_23(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, char *err_msg, const char *table_name) {
  snprintf(err_msg, 256, "The key on the second and third columns of %s was violated", table_name);
}

void tern_table_aux_record_key_violation_3(TERN_TABLE *table, TERN_TABLE_AUX *table_aux, char *err_msg, const char *table_name) {
  snprintf(err_msg, 256, "The key on the third column of %s was violated", table_name);
}

void semisym_slave_tern_table_aux_record_key_violation_12(BIN_TABLE *table, SLAVE_TERN_TABLE_AUX *table_aux, char *err_msg, const char *table_name) {
  snprintf(err_msg, 256, "The key on the first two columns of %s was violated", table_name);
}

void semisym_slave_tern_table_aux_record_key_violation_3(BIN_TABLE *table, SLAVE_TERN_TABLE_AUX *table_aux, char *err_msg, const char *table_name) {
  snprintf(err_msg, 256, "The key on the third column of %s was violated", table_name);
}

void semisym_tern_table_aux_record_key_violation_12(TERN_TABLE *table, SEMISYM_TERN_TABLE_AUX *table_aux, char *err_msg, const char *table_name) {
  snprintf(err_msg, 256, "The key on the first two columns of %s was violated", table_name);
}

void semisym_tern_table_aux_record_key_violation_3(TERN_TABLE *table, SEMISYM_TERN_TABLE_AUX *table_aux, char *err_msg, const char *table_name) {
  snprintf(err_msg, 256, "The key on the third column of %s was violated", table_name);
}
