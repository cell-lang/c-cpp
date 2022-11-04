void handler_wrapper_begin();
void handler_wrapper_finish();
void handler_wrapper_abort();

void method_wrapper_begin();
void method_wrapper_finish();
void method_wrapper_abort();

uint32 istream_read(void *ptr, uint8 *buffer, uint32 capacity);
bool   ostream_write(void *ptr, const uint8 *data, uint32 size);
