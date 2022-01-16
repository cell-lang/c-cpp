#include "lib.h"


void unary_table_init(UNARY_TABLE *, STATE_MEM_POOL *) {

}

bool unary_table_contains(UNARY_TABLE *, uint32) {

}

uint32 unary_table_insert(UNARY_TABLE *, STATE_MEM_POOL *, uint32) {

}

void unary_table_delete(UNARY_TABLE *, uint32) {

}

void unary_table_clear(UNARY_TABLE *) {

}

uint32 unary_table_queue_insert(UNARY_TABLE *, UNARY_TABLE_AUX *, uint32) {

}

void unary_table_queue_delete(UNARY_TABLE *, UNARY_TABLE_AUX *, uint32) {

}

void unary_table_queue_clear(UNARY_TABLE *, UNARY_TABLE_AUX *) {

}

void unary_table_apply(UNARY_TABLE *, UNARY_TABLE_AUX *, STATE_MEM_POOL *) {

}

void unary_table_reset(UNARY_TABLE_AUX *) {

}

OBJ unary_table_copy_to(UNARY_TABLE *table, OBJ_STORE *store, STREAM *stream) {

}

void unary_table_iter_init(UNARY_TABLE *, UNARY_TABLE_ITER *) {

}

void unary_table_iter_move_forward(UNARY_TABLE_ITER *) {

}

bool unary_table_iter_is_out_of_range(UNARY_TABLE_ITER *) {

}

uint32 unary_table_iter_get(UNARY_TABLE_ITER *) {

}
