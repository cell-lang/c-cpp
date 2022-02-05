#include "lib.h"


void sort_u32(uint32 *array, uint32 len) {
  std::sort(array, array + len);
}

void sort_u64(uint64 *array, uint32 len) {
  std::sort(array, array + len);
}

void sort_3u32(uint32 *array, uint32 len) {
  internal_fail(); //## IMPLEMENT IMPLEMENT IMPLEMENT
}

////////////////////////////////////////////////////////////////////////////////

uint32 *stable_sort_obj_index(uint32 *ys, uint32 *zs, uint32 size, OBJ *objs) {
  int sort_len = 1;
  while (sort_len < size) {
    // Merging subarrays in ys into zs
    uint32 offset = 0;
    while (offset < size) {
      uint32 end1 = offset + sort_len;

      if (end1 < size) {
        uint32 end2 = end1 + sort_len;
        if (end2 > size)
          end2 = size;

        uint32 i1 = offset;
        uint32 i2 = end1;
        uint32 j = offset;

        uint32 y1 = ys[i1];
        uint32 y2 = ys[i2];

        while (j < end2) {
          int rc = comp_objs(objs[y1], objs[y2]);

          if (rc >= 0) { // y1 <= y2
            zs[j] = y1;
            j = j + 1;
            i1 = i1 + 1;
            if (i1 == end1) {
              while (i2 < end2) {
                zs[j] = ys[i2];
                j = j + 1;
                i2 = i2 + 1;
              }
            }
            else
              y1 = ys[i1];
          }
          else { // y1 > y2
            zs[j] = y2;
            j = j + 1;
            i2 = i2 + 1;
            if (i2 == end2) {
              while (i1 < end1) {
                zs[j] = ys[i1];
                j = j + 1;
                i1 = i1 + 1;
              }
            }
            else
              y2 = ys[i2];
          }
        }
      }
      else
        for (uint32 i = offset ; i < size ; i++)
          zs[i] = ys[i];

      offset = offset + 2 * sort_len;
    }

    uint32 *tmp = ys;
    ys = zs;
    zs = tmp;

    sort_len = 2 * sort_len;
  }

  return ys;
}

////////////////////////////////////////////////////////////////////////////////

uint32 *stable_sort_obj_index_2(uint32 *ys, uint32 *zs, uint32 size, OBJ *major, OBJ *minor) {
  int sort_len = 1;
  while (sort_len < size) {
    // Merging subarrays in ys into zs
    uint32 offset = 0;
    while (offset < size) {
      uint32 end1 = offset + sort_len;

      if (end1 < size) {
        uint32 end2 = end1 + sort_len;
        if (end2 > size)
          end2 = size;

        uint32 i1 = offset;
        uint32 i2 = end1;
        uint32 j = offset;

        uint32 y1 = ys[i1];
        uint32 y2 = ys[i2];

        while (j < end2) {
          int rc = comp_objs(major[y1], major[y2]);
          if (rc == 0)
            rc = comp_objs(minor[y1], minor[y2]);

          if (rc >= 0) { // y1 <= y2
            zs[j] = y1;
            j = j + 1;
            i1 = i1 + 1;
            if (i1 == end1) {
              while (i2 < end2) {
                zs[j] = ys[i2];
                j = j + 1;
                i2 = i2 + 1;
              }
            }
            else
              y1 = ys[i1];
          }
          else { // y1 > y2
            zs[j] = y2;
            j = j + 1;
            i2 = i2 + 1;
            if (i2 == end2) {
              while (i1 < end1) {
                zs[j] = ys[i1];
                j = j + 1;
                i1 = i1 + 1;
              }
            }
            else
              y2 = ys[i2];
          }
        }
      }
      else
        for (uint32 i = offset ; i < size ; i++)
          zs[i] = ys[i];

      offset = offset + 2 * sort_len;
    }

    uint32 *tmp = ys;
    ys = zs;
    zs = tmp;

    sort_len = 2 * sort_len;
  }

  return ys;
}

////////////////////////////////////////////////////////////////////////////////

uint32 *stable_sort_obj_index_3(uint32 *ys, uint32 *zs, uint32 size, OBJ *major, OBJ *middle, OBJ *minor) {
  int sort_len = 1;
  while (sort_len < size) {
    // Merging subarrays in ys into zs
    uint32 offset = 0;
    while (offset < size) {
      uint32 end1 = offset + sort_len;

      if (end1 < size) {
        uint32 end2 = end1 + sort_len;
        if (end2 > size)
          end2 = size;

        uint32 i1 = offset;
        uint32 i2 = end1;
        uint32 j = offset;

        uint32 y1 = ys[i1];
        uint32 y2 = ys[i2];

        while (j < end2) {
          int rc = comp_objs(major[y1], major[y2]);
          if (rc == 0) {
            rc = comp_objs(middle[y1], middle[y2]);
            if (rc == 0)
              rc = comp_objs(minor[y1], minor[y2]);
          }

          if (rc >= 0) { // y1 <= y2
            zs[j] = y1;
            j = j + 1;
            i1 = i1 + 1;
            if (i1 == end1) {
              while (i2 < end2) {
                zs[j] = ys[i2];
                j = j + 1;
                i2 = i2 + 1;
              }
            }
            else
              y1 = ys[i1];
          }
          else { // y1 > y2
            zs[j] = y2;
            j = j + 1;
            i2 = i2 + 1;
            if (i2 == end2) {
              while (i1 < end1) {
                zs[j] = ys[i1];
                j = j + 1;
                i1 = i1 + 1;
              }
            }
            else
              y2 = ys[i2];
          }
        }
      }
      else
        for (uint32 i = offset ; i < size ; i++)
          zs[i] = ys[i];

      offset = offset + 2 * sort_len;
    }

    uint32 *tmp = ys;
    ys = zs;
    zs = tmp;

    sort_len = 2 * sort_len;
  }

  return ys;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

static uint32 custom_sort_unique_setup_step(OBJ *ys, OBJ *zs, uint32 *sizes, uint32 size) {
  uint32 iy = 0;
  uint32 iz = 0;
  uint32 ib = 0;

  while (iy + 3 <= size) {
    uint32 block_size = -1;

    OBJ elt0 = ys[iy++];
    OBJ elt1 = ys[iy++];
    OBJ elt2 = ys[iy++];

    int cr01 = comp_objs(elt0, elt1);

    if (cr01 == 0) {
      int cr12 = comp_objs(elt1, elt2);

      if (cr12 == 0) {
        // elt0 == elt1 == elt2
        zs[iz++] = elt0;
        block_size = 1;
      }
      else if (cr12 > 0) {
        // elt0 == elt1, elt1 < elt2
        zs[iz++] = elt1;
        zs[iz++] = elt2;
        block_size = 2;
      }
      else {
        // elt0 == elt1, elt1 > elt2
        zs[iz++] = elt2;
        zs[iz++] = elt1;
        block_size = 2;
      }
    }
    else {
      if (cr01 < 0) {
        OBJ tmp = elt0;
        elt0 = elt1;
        elt1 = tmp;
      }

      // elt0 < elt1
      int cr12 = comp_objs(elt1, elt2);

      if (cr12 > 0) {
        // elt0 < elt1 < elt2
        zs[iz++] = elt0;
        zs[iz++] = elt1;
        zs[iz++] = elt2;
        block_size = 3;
      }
      else if (cr12 < 0) {
        int cr02 = comp_objs(elt0, elt2);

        if (cr02 > 0) {
          // elt0 < elt2 < elt1
          zs[iz++] = elt0;
          zs[iz++] = elt2;
          zs[iz++] = elt1;
          block_size = 3;
        }
        else if (cr02 < 0) {
          // elt2 < elt0 < elt1
          zs[iz++] = elt2;
          zs[iz++] = elt0;
          zs[iz++] = elt1;
          block_size = 3;
        }
        else {
          // elt0 < elt1, elt0 == elt2
          zs[iz++] = elt0;
          zs[iz++] = elt1;
          block_size = 2;
        }
      }
      else {
        // elt0 < elt1, elts[1] == elt2
        zs[iz++] = elt0;
        zs[iz++] = elt1;
        block_size = 2;
      }
    }

    assert(block_size >= 0 | block_size <= 1);
    sizes[ib++] = block_size;
  }

  // Taking care of a possible undersized block at the end

  uint32 left = size - iy;
  assert(left >= 0 & left <= 2);

  if (left == 2) {
    uint32 block_size = -1;

    OBJ elt0 = ys[iy++];
    OBJ elt1 = ys[iy++];

    int cr = comp_objs(elt0, elt1);

    if (cr == 0) {
      zs[iz++] = elt0;
      block_size = 1;
    }
    else if (cr > 0) {
      // elt0 < elt1
      zs[iz++] = elt0;
      zs[iz++] = elt1;
      block_size = 2;
    }
    else {
      // elts[0] > elts[1]
      zs[iz++] = elt1;
      zs[iz++] = elt0;
      block_size = 2;
    }

    assert(block_size == 1 | block_size == 2);
    sizes[ib++] = block_size;
  }
  else if (left == 1) {
    zs[iz] = ys[iy];
    sizes[ib++] = 1;
  }

  return ib;
}

////////////////////////////////////////////////////////////////////////////////

OBJ *custom_sort_unique(OBJ *zs, OBJ *ys, uint32 *sizes, uint32 size, uint32 &final_size) {
  assert(size > 0);

  uint32 blocks_count = custom_sort_unique_setup_step(zs, ys, sizes, size);

  while (blocks_count != 1) {
    // Merging subarrays in ys into zs

    uint32 iy = 0;
    uint32 iz = 0;

    uint32 merged_block_idx = 0;

    for (uint32 ib=0 ; ib + 1 < blocks_count ; ib += 2) {
      uint32 size1 = sizes[ib];
      uint32 size2 = sizes[ib + 1];

      uint32 i1 = iy;
      uint32 i2 = iy + size1;

      iy = i2 + size2;

      uint32 start_iz = iz;

      uint32 end1 = i2;
      uint32 end2 = end1 + size2;

      OBJ y1 = ys[i1];
      OBJ y2 = ys[i2];

      while (i1 < end1) {
        assert(i2 < end2);
        assert(y1.core_data.int_ == ys[i1].core_data.int_ & y1.extra_data == ys[i1].extra_data);
        assert(y2.core_data.int_ == ys[i2].core_data.int_ & y2.extra_data == ys[i2].extra_data);

        int rc = comp_objs(y1, y2);

        if (rc > 0) { // y1 < y2
          zs[iz++] = y1;
          if (++i1 == end1) {
            zs[iz++] = y2;
            while (++i2 < end2)
              zs[iz++] = ys[i2];
          }
          else
            y1 = ys[i1];
        }
        else if (rc < 0) { // y1 > y2
          zs[iz++] = y2;
          if (++i2 == end2) {
            zs[iz++] = y1;
            while (++i1 < end1)
              zs[iz++] = ys[i1];
          }
          else
            y2 = ys[i2];
        }
        else { // y1 == y2
          zs[iz++] = y1;
          i1++;
          i2++;

          if (i1 == end1) {
            while (i2 < end2)
              zs[iz++] = ys[i2++];
          }
          else if (i2 == end2) {
            while (i1 < end1)
              zs[iz++] = ys[i1++];
          }
          else {
            y1 = ys[i1];
            y2 = ys[i2];
          }
        }
      }

      sizes[merged_block_idx++] = iz - start_iz;
    }

    // Accounting for a possible spare block at the end
    if ((blocks_count & 1) != 0) {
      uint32 size = sizes[blocks_count - 1];
      sizes[merged_block_idx++] = size;
      uint32 end = iy + size;
      while (iy < end)
        zs[iz++] = ys[iy++];
    }

    OBJ *tmp = ys;
    ys = zs;
    zs = tmp;

    blocks_count = merged_block_idx;
  }

  final_size = sizes[0];
  return ys;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

static uint32 custom_inline_sort_unique_setup_step(OBJ *ys, OBJ *zs, uint32 *sizes, uint32 size) {
  uint32 iy = 0;
  uint32 iz = 0;
  uint32 ib = 0;

  while (iy + 3 <= size) {
    uint32 block_size = -1;

    OBJ elt0 = ys[iy++];
    OBJ elt1 = ys[iy++];
    OBJ elt2 = ys[iy++];

    int cr01 = shallow_cmp(elt0, elt1);

    if (cr01 == 0) {
      int cr12 = shallow_cmp(elt1, elt2);

      if (cr12 == 0) {
        // elt0 == elt1 == elt2
        zs[iz++] = elt0;
        block_size = 1;
      }
      else if (cr12 > 0) {
        // elt0 == elt1, elt1 < elt2
        zs[iz++] = elt1;
        zs[iz++] = elt2;
        block_size = 2;
      }
      else {
        // elt0 == elt1, elt1 > elt2
        zs[iz++] = elt2;
        zs[iz++] = elt1;
        block_size = 2;
      }
    }
    else {
      if (cr01 < 0) {
        OBJ tmp = elt0;
        elt0 = elt1;
        elt1 = tmp;
      }

      // elt0 < elt1
      int cr12 = shallow_cmp(elt1, elt2);

      if (cr12 > 0) {
        // elt0 < elt1 < elt2
        zs[iz++] = elt0;
        zs[iz++] = elt1;
        zs[iz++] = elt2;
        block_size = 3;
      }
      else if (cr12 < 0) {
        int cr02 = shallow_cmp(elt0, elt2);

        if (cr02 > 0) {
          // elt0 < elt2 < elt1
          zs[iz++] = elt0;
          zs[iz++] = elt2;
          zs[iz++] = elt1;
          block_size = 3;
        }
        else if (cr02 < 0) {
          // elt2 < elt0 < elt1
          zs[iz++] = elt2;
          zs[iz++] = elt0;
          zs[iz++] = elt1;
          block_size = 3;
        }
        else {
          // elt0 < elt1, elt0 == elt2
          zs[iz++] = elt0;
          zs[iz++] = elt1;
          block_size = 2;
        }
      }
      else {
        // elt0 < elt1, elts[1] == elt2
        zs[iz++] = elt0;
        zs[iz++] = elt1;
        block_size = 2;
      }
    }

    assert(block_size >= 0 | block_size <= 1);
    sizes[ib++] = block_size;
  }

  // Taking care of a possible undersized block at the end

  uint32 left = size - iy;
  assert(left >= 0 & left <= 2);

  if (left == 2) {
    uint32 block_size = -1;

    OBJ elt0 = ys[iy++];
    OBJ elt1 = ys[iy++];

    int cr = shallow_cmp(elt0, elt1);

    if (cr == 0) {
      zs[iz++] = elt0;
      block_size = 1;
    }
    else if (cr > 0) {
      // elt0 < elt1
      zs[iz++] = elt0;
      zs[iz++] = elt1;
      block_size = 2;
    }
    else {
      // elts[0] > elts[1]
      zs[iz++] = elt1;
      zs[iz++] = elt0;
      block_size = 2;
    }

    assert(block_size == 1 | block_size == 2);
    sizes[ib++] = block_size;
  }
  else if (left == 1) {
    zs[iz] = ys[iy];
    sizes[ib++] = 1;
  }

  return ib;
}

////////////////////////////////////////////////////////////////////////////////

OBJ *custom_inline_sort_unique(OBJ *zs, OBJ *ys, uint32 *sizes, uint32 size, uint32 &final_size) {
  assert(size > 0);

  uint32 blocks_count = custom_inline_sort_unique_setup_step(zs, ys, sizes, size);

  while (blocks_count != 1) {
    // Merging subarrays in ys into zs

    uint32 iy = 0;
    uint32 iz = 0;

    uint32 merged_block_idx = 0;

    for (uint32 ib=0 ; ib + 1 < blocks_count ; ib += 2) {
      uint32 size1 = sizes[ib];
      uint32 size2 = sizes[ib + 1];

      uint32 i1 = iy;
      uint32 i2 = iy + size1;

      iy = i2 + size2;

      uint32 start_iz = iz;

      uint32 end1 = i2;
      uint32 end2 = end1 + size2;

      OBJ y1 = ys[i1];
      OBJ y2 = ys[i2];

      while (i1 < end1) {
        assert(i2 < end2);
        assert(y1.core_data.int_ == ys[i1].core_data.int_ & y1.extra_data == ys[i1].extra_data);
        assert(y2.core_data.int_ == ys[i2].core_data.int_ & y2.extra_data == ys[i2].extra_data);

        int rc = shallow_cmp(y1, y2);

        if (rc > 0) { // y1 < y2
          zs[iz++] = y1;
          if (++i1 == end1) {
            zs[iz++] = y2;
            while (++i2 < end2)
              zs[iz++] = ys[i2];
          }
          else
            y1 = ys[i1];
        }
        else if (rc < 0) { // y1 > y2
          zs[iz++] = y2;
          if (++i2 == end2) {
            zs[iz++] = y1;
            while (++i1 < end1)
              zs[iz++] = ys[i1];
          }
          else
            y2 = ys[i2];
        }
        else { // y1 == y2
          zs[iz++] = y1;
          i1++;
          i2++;

          if (i1 == end1) {
            while (i2 < end2)
              zs[iz++] = ys[i2++];
          }
          else if (i2 == end2) {
            while (i1 < end1)
              zs[iz++] = ys[i1++];
          }
          else {
            y1 = ys[i1];
            y2 = ys[i2];
          }
        }
      }

      sizes[merged_block_idx++] = iz - start_iz;
    }

    // Accounting for a possible spare block at the end
    if ((blocks_count & 1) != 0) {
      uint32 size = sizes[blocks_count - 1];
      sizes[merged_block_idx++] = size;
      uint32 end = iy + size;
      while (iy < end)
        zs[iz++] = ys[iy++];
    }

    OBJ *tmp = ys;
    ys = zs;
    zs = tmp;

    blocks_count = merged_block_idx;
  }

  final_size = sizes[0];
  return ys;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/* __attribute__ ((noinline)) */ static bool is_sorted(OBJ *objs, uint32 size) {
  OBJ last = objs[0];
  for (int i=1 ; i < size ; i++) {
    OBJ curr = objs[i];
    if (comp_objs(last, curr) <= 0)
      return false;
    last = curr;
  }
  return true;
}


uint32 sort_unique_(OBJ *objs, uint32 size) {
  OBJ obj = objs[0];
  if (is_inline_obj(obj)) {
    bool already_sorted = true;
    for (int i=1 ; i < size ; i++) {
      OBJ obj_i = objs[i];
      if (!is_inline_obj(obj_i)) {
        goto not_all_inline;
        //## SOMEHOW THIS MADE IT SLOWER. TRY AGAIN SOMETIMES
        // if (already_sorted && is_sorted(objs + i, size - i)) {
        //   assert(is_sorted(objs, size));
        //   return size;
        // }
        // else
        //   goto not_all_inline_not_sorted;
      }
      if (shallow_cmp(obj, obj_i) <= 0)
        already_sorted = false;
      obj = obj_i;
    }

    if (already_sorted)
      return size;

    if (size <= 1024) {
      OBJ buffer[1024];
      uint32 uint32_buff[1024];

      uint32 unique = -1;
      OBJ *final_buff = custom_inline_sort_unique(objs, buffer, uint32_buff, size, unique);

      if (final_buff == buffer)
        memcpy(objs, buffer, unique * sizeof(OBJ));

      return unique;
    }
    else {
      //## HERE I OUGHT TO RELEASE THE BUFFERS RIGHT AWAY
      OBJ *buffer = new_obj_array(size);
      uint32 *uint32_buff = new_uint32_array(size);

      uint32 unique = -1;
      OBJ *final_buff = custom_inline_sort_unique(objs, buffer, uint32_buff, size, unique);

      if (final_buff == buffer)
        memcpy(objs, buffer, unique * sizeof(OBJ));

      return unique;
    }
  }

not_all_inline:
  if (is_sorted(objs, size))
    return size;

not_all_inline_not_sorted:
  if (size <= 1024) {
    OBJ buffer[1024];
    uint32 uint32_buff[1024];

    uint32 unique = -1;
    OBJ *final_buff = custom_sort_unique(objs, buffer, uint32_buff, size, unique);

    if (final_buff == buffer)
      memcpy(objs, buffer, unique * sizeof(OBJ));

    return unique;
  }
  else {
    //## HERE I OUGHT TO RELEASE THE BUFFERS RIGHT AWAY
    OBJ *buffer = new_obj_array(size);
    uint32 *uint32_buff = new_uint32_array(size);

    uint32 unique = -1;
    OBJ *final_buff = custom_sort_unique(objs, buffer, uint32_buff, size, unique);

    if (final_buff == buffer)
      memcpy(objs, buffer, unique * sizeof(OBJ));

    return unique;
  }
}

uint32 sort_unique(OBJ *objs, uint32 size) {
  uint32 final_size = sort_unique_(objs, size);

  // for (int i=0 ; i < final_size ; i++)
  //   for (int j=i+1 ; j < final_size ; j++)
  //     if (comp_objs(objs[i], objs[j]) != 1) {
  //       printf("\nSorting error, size = %d, final_size = %d\n", size, final_size);
  //       if (size == 3 & final_size == 3) {
  //         for (int k=0 ; k < 3 ; k++)
  //           print(objs[k]);
  //         printf("\n");
  //         for (int k=0 ; k < 3 ; k++)
  //           for (int l=k+1 ; l < 3 ; l++)
  //             printf("comp_objs(%d, %d) -> %d\n", k, l, comp_objs(objs[k], objs[l]));
  //         exit(1);
  //       }
  //       return final_size;
  //     }

  return final_size;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void stable_index_sort(uint32 *indexes, uint32 count, OBJ *values) {
  for (uint32 i=0 ; i < count ; i++)
    indexes[i] = i;
  uint32 *buffer = new_uint32_array(count); //## REMEMBER TO RELEASE THE MEMORY ONCE THE ALLOCATOR IS DONE
  uint32 *final = stable_sort_obj_index(indexes, buffer, count, values);
  if (final != indexes)
    memcpy(indexes, final, count * sizeof(uint32));

#ifndef NDEBUG
  for (uint32 i=1 ; i < count ; i++) {
    uint32 pix = indexes[i - 1];
    uint32 cix = indexes[i];
    int cr = comp_objs(values[pix], values[cix]);
    assert(cr > 0 | (cr == 0 & pix < cix));
  }
#endif
}

void stable_index_sort(uint32 *indexes, uint32 count, OBJ *major, OBJ *minor) {
  for (uint32 i=0 ; i < count ; i++)
    indexes[i] = i;
  uint32 *buffer = new_uint32_array(count); //## REMEMBER TO RELEASE THE MEMORY ONCE THE ALLOCATOR IS DONE
  uint32 *final = stable_sort_obj_index_2(indexes, buffer, count, major, minor);
  if (final != indexes)
    memcpy(indexes, final, count * sizeof(uint32));

#ifndef NDEBUG
  for (uint32 i=1 ; i < count ; i++) {
    uint32 pix = indexes[i - 1];
    uint32 cix = indexes[i];
    int cr = comp_objs(major[pix], major[cix]);
    assert(cr >= 0);
    if (cr == 0) {
      cr = comp_objs(minor[pix], minor[cix]);
      assert(cr > 0 | (cr == 0 & pix < cix));
    }
  }
#endif
}

void stable_index_sort(uint32 *indexes, uint32 count, OBJ *major, OBJ *middle, OBJ *minor) {
  for (uint32 i=0 ; i < count ; i++)
    indexes[i] = i;
  uint32 *buffer = new_uint32_array(count);
  uint32 *final = stable_sort_obj_index_3(indexes, buffer, count, major, middle, minor);
  if (final != indexes)
    memcpy(indexes, final, count * sizeof(uint32));

#ifndef NDEBUG
  for (uint32 i=1 ; i < count ; i++) {
    uint32 pix = indexes[i - 1];
    uint32 cix = indexes[i];
    int cr = comp_objs(major[pix], major[cix]);
    assert(cr >= 0);
    if (cr == 0) {
      cr = comp_objs(middle[pix], middle[cix]);
      assert(cr >= 0);
      if (cr == 0) {
        cr = comp_objs(minor[pix], minor[cix]);
        assert(cr > 0 | (cr == 0 & pix < cix));
      }
    }
  }
#endif
}

////////////////////////////////////////////////////////////////////////////////

void index_sort(uint32 *indexes, uint32 count, OBJ *values) {
  stable_index_sort(indexes, count, values);
}

void index_sort(uint32 *indexes, uint32 count, OBJ *major_sort, OBJ *minor_sort) {
  stable_index_sort(indexes, count, major_sort, minor_sort);
}

void index_sort(uint32 *indexes, uint32 count, OBJ *major_sort, OBJ *middle_sort, OBJ *minor_sort) {
  stable_index_sort(indexes, count, major_sort, middle_sort, minor_sort);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// OBJ *custom_sort(OBJ *ys, OBJ *zs, int size) {
//   int sort_len = 1;
//   while (sort_len < size) {
//     // Merging subarrays in ys into zs
//     int offset = 0;
//     while (offset < size) {
//       int end1 = offset + sort_len;

//       if (end1 < size) {
//         int end2 = end1 + sort_len;
//         if (end2 > size)
//           end2 = size;

//         int i1 = offset;
//         int i2 = end1;
//         int j = offset;

//         OBJ y1 = ys[i1];
//         OBJ y2 = ys[i2];

//         while (j < end2) {

//           int rc = comp_objs(y1, y2);

//           if (rc > 0) { // y1 < y2
//             zs[j] = y1;
//             j = j + 1;
//             i1 = i1 + 1;
//             if (i1 == end1) {
//               while (i2 < end2) {
//                 zs[j] = ys[i2];
//                 j = j + 1;
//                 i2 = i2 + 1;
//               }
//             }
//             else
//               y1 = ys[i1];
//           }
//           else { //if (rc < 0) { // y1 >= y2
//             zs[j] = y2;
//             j = j + 1;
//             i2 = i2 + 1;
//             if (i2 == end2) {
//               while (i1 < end1) {
//                 zs[j] = ys[i1];
//                 j = j + 1;
//                 i1 = i1 + 1;
//               }
//             }
//             else
//               y2 = ys[i2];
//           }
//         }
//       }
//       else
//         for (int i = offset ; i < size ; i++)
//           zs[i] = ys[i];

//       offset = offset + 2 * sort_len;
//     }

//     OBJ *tmp = ys;
//     ys = zs;
//     zs = tmp;

//     sort_len = 2 * sort_len;
//   }

//   return ys;
// }

// uint32 sort_unique(OBJ *objs, uint32 size) {
//   if (size < 2)
//     return size;

//   uint32 low_idx = 0;
//   uint32 high_idx = size - 1; // size is greater than 0 (actually 1) here, so this is always non-negative (actually positive)
//   for ( ; ; ) {
//     // Advancing the lower cursor to the next non-inline object
//     while (low_idx < high_idx & is_inline_obj(objs[low_idx]))
//       low_idx++;

//     // Advancing the upper cursor to the next inline object
//     while (high_idx > low_idx & not is_inline_obj(objs[high_idx]))
//       high_idx--;

//     if (low_idx == high_idx)
//       break;

//     OBJ tmp = objs[low_idx];
//     objs[low_idx] = objs[high_idx];
//     objs[high_idx] = tmp;
//   }

//   uint32 inline_count = is_inline_obj(objs[low_idx]) ? low_idx + 1 : low_idx;

//   uint32 idx = 0;
//   if (inline_count > 0) {
//     std::sort(objs, objs+inline_count, obj_inline_less());

//     OBJ last_obj = objs[0];
//     for (uint32 i=1 ; i < inline_count ; i++) {
//       OBJ next_obj = objs[i];
//       if (!inline_eq(last_obj, next_obj)) {
//         idx++;
//         last_obj = next_obj;
//         assert(idx <= i);
//         if (idx != i)
//           objs[idx] = next_obj;
//       }
//     }

//     idx++;
//     if (inline_count == size)
//       return idx;
//   }

//   std::sort(objs+inline_count, objs+size, obj_less());

//   if (idx != inline_count)
//     objs[idx] = objs[inline_count];

//   for (uint32 i=inline_count+1 ; i < size ; i++)
//     if (!are_eq(objs[idx], objs[i])) {
//       idx++;
//       assert(idx <= i);
//       if (idx != i)
//         objs[idx] = objs[i];
//     }

//   return idx + 1;
// }


// uint32 remove_duplicates(OBJ *objs, uint32 size) {
//   uint32 idx = 0;
//   for (uint32 i=1 ; i < size ; i++)
//     if (!are_eq(objs[idx], objs[i])) {
//       idx++;
//       assert(idx <= i);
//       if (idx != i)
//         objs[idx] = objs[i];
//     }

//   return idx + 1;
// }
