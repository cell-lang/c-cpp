// uint64 get_tick_count(); // Moved to lib.h

uint8 *file_read(const char *fname, int &size);
bool   file_write(const char *fname, const char *buffer, int size, bool append);
