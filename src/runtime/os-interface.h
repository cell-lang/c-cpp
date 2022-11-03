uint8 *file_read(const char *fname, int &size);
bool   file_write(const char *fname, const char *buffer, int size, bool append);

void *file_io_open_writable(const char *fname);
bool  file_io_write(void *fp, const uint8 *data, uint32 size);
void  file_io_close(void *fp);

uint64 phys_mem_byte_size();

void *os_interface_reserve(uint64 size);
void os_interface_release(void *, uint64 size);

void os_interface_alloc(void *ptr, uint64 start, uint64 size);
void os_interface_dealloc(void *ptr, uint64 start, uint64 size);

int64 get_epoc_time_nsec();
// uint64 get_tick_count(); // Moved to lib.h
