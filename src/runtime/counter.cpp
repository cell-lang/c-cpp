#include "lib.h"


void counter_init(COUNTER *counter, STATE_MEM_POOL *mem_pool) {
  const uint32 INIT_SIZE = 256;

  counter->capacity = INIT_SIZE;

  uint8 *uint8_array = alloc_state_mem_uint8_array(mem_pool, INIT_SIZE);
  memset(uint8_array, 0, INIT_SIZE * sizeof(uint8));
  counter->counters = uint8_array;
}

void counter_clear(COUNTER *counter, STATE_MEM_POOL *mem_pool) {
  release_state_mem_uint8_array(mem_pool, counter->counters, counter->capacity);
  counter->capacity = 0;
  counter->counters = NULL;
  counter->overflows.clear();
}

void counter_resize(COUNTER *counter, uint32 min_capacity, STATE_MEM_POOL *mem_pool) {
  assert(counter->counters != NULL && counter->capacity != 0);
  assert(min_capacity > counter->capacity);

  uint32 capacity = counter->capacity;
  uint32 new_capacity = 2 * capacity;
  while (new_capacity < min_capacity)
    new_capacity *= 2;
  counter->capacity = new_capacity;

  assert(new_capacity > capacity && new_capacity >= min_capacity);

  //## BUG BUG BUG: counter->counters IS NEVER RELEASED
  uint8 *counters = extend_state_mem_zeroed_uint8_array(mem_pool, counter->counters, capacity, new_capacity);
  counter->counters = counters;

  for (uint32 i=capacity ; i < new_capacity ; i++)
    assert(counter->counters[i] == 0);
}

////////////////////////////////////////////////////////////////////////////////

uint32 counter_read(COUNTER *counter, uint32 index) {
  uint32 count = 0;
  if (index < counter->capacity) {
    count = counter->counters[index];
    if (count > 127)
      if (counter->overflows.count(index) > 0)
        count += counter->overflows[index];
  }
  return count;
}

bool counter_is_cleared(COUNTER *counter) { //## BAD BAD BAD: FIND BETTER NAME
  return counter->counters == NULL;
}

////////////////////////////////////////////////////////////////////////////////

void counter_incr(COUNTER *counter, uint32 index, STATE_MEM_POOL *mem_pool) {
  if (index >= counter->capacity)
    counter_resize(counter, index + 1, mem_pool);

  assert(index < counter->capacity);

  uint8 *counters = counter->counters;
  uint32 count = counters[index] + 1;
  if (count == 256) {
    counter->overflows[index]++;
    count -= 64;
  }
  counters[index] = count;
}

void counter_decr(COUNTER *counter, uint32 index, STATE_MEM_POOL *mem_pool) {
  assert(index < counter->capacity);

  uint8 *counters = counter->counters;
  assert(counters[index] > 0);
  uint32 count = counters[index] - 1;
  if (count == 127) {
    if (counter->overflows.count(index) > 0) {
      uint32 extra_count = counter->overflows[index] - 1;
      if (extra_count == 0)
        counter->overflows.erase(index);
      else
        counter->overflows[index] = extra_count;
      count += 64;
    }
  }
  counters[index] = count;
}

void counter_decr(COUNTER *counter, uint32 index, uint32 amount, STATE_MEM_POOL *mem_pool) {
  assert(index < counter->capacity);

  uint8 *counters = counter->counters;
  uint32 count = counters[index];
  assert(count > 0);

  if (count < 128 || counter->overflows.count(index) == 0) {
    count -= amount;
  }
  else {
    count += 64 * counter->overflows[index];
    count -= amount;
    if (count >= 256) {
      uint32 new_extra_count = (count / 64) - 2;
      count -= 64 * new_extra_count;
      counter->overflows[index] = new_extra_count;
    }
    else
      counter->overflows.erase(index);
  }

  assert(count < 256);

  counters[index] = count;
}
