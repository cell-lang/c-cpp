#include <sys/mman.h>
#include <unistd.h>

#include "lib.h"
#include "os-interface.h"


uint64 get_tick_count() {
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
    // error
  }
  return 1000 * ts.tv_sec + ts.tv_nsec / 1000000;
}

uint8 *file_read(const char *fname, int &size) {
  FILE *fp = fopen(fname, "r");
  if (fp == NULL) {
    size = -1;
    return NULL;
  }
  int start = ftell(fp);
  assert(start == 0);
  fseek(fp, 0, SEEK_END);
  int end = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  size = end - start;
  if (size == 0) {
    fclose(fp);
    return NULL;
  }
  uint8 *data = new_uint8_array(size);
  int read = fread(data, 1, size, fp);
  fclose(fp);
  if (read != size) {
    size = -1;
    return NULL;
  }
  return data;
}

bool file_write(const char *fname, const char *buffer, int size, bool append) {
  FILE *fp = fopen(fname, append ? "a" : "w");
  if (fp == NULL)
    return false;
  size_t written = fwrite(buffer, 1, size, fp);
  fclose(fp);
  return written == size;
}

////////////////////////////////////////////////////////////////////////////////

uint64 phys_mem_byte_size() {
  uint64 num_pages = sysconf (_SC_PHYS_PAGES);
  uint32 page_size = sysconf (_SC_PAGESIZE);
  return num_pages * page_size;
}

////////////////////////////////////////////////////////////////////////////////

void *os_interface_reserve(uint64 size) {
  void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
  return ptr;
}

void os_interface_release(void *ptr, uint64 size) {
  munmap(ptr, size);
}

void os_interface_alloc(void *ptr, uint64 offset, uint64 size) {
  uint8 *subptr = (uint8 *) ptr + offset;
  mprotect(subptr, size, PROT_READ | PROT_WRITE);
}

void os_interface_dealloc(void *ptr, uint64 offset, uint64 size) {
  void *subptr = ((uint8 *) ptr) + offset;
  int result = madvise(subptr, size, MADV_DONTNEED);
  if (result != 0) {
    fputs("Unrecoverable error in os_interface_dealloc(void *, unsigned long long, unsigned long long)\n", stderr);
    exit(1);
  }
}
