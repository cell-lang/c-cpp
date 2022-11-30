#!/usr/bin/env python

################################################################################

src_files = [
  'algs.cpp',
  'array-mem-pool.cpp',
  'arrays.cpp',
  'basic-ops.cpp',
  'bin-rel-obj.cpp',
  'bin-table-aux.cpp',
  'bin-table.cpp',
  'bit-map.cpp',
  'cmp.cpp',
  'concat.cpp',
  'conversion.cpp',
  'counter.cpp',
  'datetime.cpp',
  'debug.cpp',
  'double-key-bin-table-aux.cpp',
  'double-key-bin-table.cpp',
  'float-col-aux.cpp',
  'float-col.cpp',
  'hashing.cpp',
  'hashtables.cpp',
  'init.cpp',
  'instrs.cpp',
  'int-col-aux.cpp',
  'int-col.cpp',
  'inter-utils.cpp',
  'int-store-aux.cpp',
  'int-store.cpp',
  'iolib.cpp',
  'key-checking-utils.cpp',
  'loaded-one-way-bin-table.cpp',
  'loaded-overflow-table.cpp',
  'master-bin-table-aux.cpp',
  'master-bin-table.cpp',
  'mem-alloc.cpp',
  'mem-copying.cpp',
  'mem-core.cpp',
  'mem.cpp',
  'mem-utils.cpp',
  'obj-col-aux.cpp',
  'obj-col.cpp',
  'obj-store-aux.cpp',
  'obj-store.cpp',
  'one-way-bin-table.cpp',
  'os-interface-linux.cpp',
  'overflow-table.cpp',
  'parsing.cpp',
  'printing.cpp',
  'queues.cpp',
  'raw-float-col-aux.cpp',
  'raw-float-col.cpp',
  'raw-int-col-aux.cpp',
  'raw-int-col.cpp',
  'raw-obj-col-aux.cpp',
  'raw-obj-col.cpp',
  'semisym-slave-tern-table-aux.cpp',
  'semisym-slave-tern-table.cpp',
  'semisym-tern-table-aux.cpp',
  'semisym-tern-table.cpp',
  'seqs.cpp',
  'single-key-bin-table-aux.cpp',
  'single-key-bin-table.cpp',
  'slave-tern-table-aux.cpp',
  'slave-tern-table.cpp',
  'sorting.cpp',
  'stack-mem-alloc.cpp',
  'state-mem-alloc.cpp',
  'surr-set.cpp',
  'sym-bin-table-aux.cpp',
  'sym-bin-table.cpp',
  'symb-table.cpp',
  'sym-master-bin-table-aux.cpp',
  'sym-master-bin-table.cpp',
  'tern-rel-obj.cpp',
  'tern-table-aux.cpp',
  'tern-table.cpp',
  'tree-map-obj.cpp',
  'tree-set-obj.cpp',
  'unary-table-aux.cpp',
  'unary-table.cpp',
  'utils.cpp',
  'wrappers.cpp',
  'writing.cpp',
]

hdr_files = [
  'utils.h',
  'lib.h',
  'iolib.h',
  'extern.h',
  'basic-ops.h',
  'conversion.h',
  'instrs.h',
  'mem-utils.h',
  'one-way-bin-table.h',
  'os-interface.h',
  'seqs.h',
  'wrappers.h',
]

################################################################################

return_types = [
  'void',
  'bool',
  'char',
  'int',
  'int8',
  'int16',
  'int32',
  'int64',
  'uint8',
  'uint16',
  'uint32',
  'uint64',
  'double',

  'std::vector',
  'std::string',

  'BIN_REL_OBJ',
  'BOXED_OBJ',
  'INT_INFO',
  'INT_RANGE',
  'MIXED_REPR_MAP_OBJ',
  'MIXED_REPR_SET_OBJ',
  'OBJ',
  'SEQ_OBJ',
  'SET_OBJ',
  'STATE_MEM_POOL',
  'TERN_REL_OBJ',
  'TEXT_FRAG',
  'TREE_MAP_NODE',
  'TREE_SET_NODE',
  'UINT32_ARRAY',
]

def needs_static_qualifier(line):
  if not line or not line[0].isalpha():
    return False;
  idx = line.find('//')
  if idx != -1:
    line = line[:idx].rstrip()
  for rt in return_types:
    if line.startswith(rt) and (line.endswith(' {') or line.endswith(');')):
      return True;
  return False

################################################################################

num_of_tabs = 0

def escape(ch):
  if ch == ord('\\'):
    return '\\\\'
  elif ch == ord('"'):
    return '\\"'
  elif ch >= ord(' ') or ch <= ord('~'):
    return chr(ch)
  elif ch == ord('\t'):
    global num_of_tabs
    num_of_tabs += 1
    return '\\t'
  else:
    print 'Invalid character: ' + ch
    exit(1);


def merge_lines(lines):
  merged_lines = []
  curr_line = ""
  for l in lines:
    if l:
      if len(curr_line) + len(l) > 2000:
        merged_lines.append(curr_line)
        curr_line = ""
      if curr_line:
        curr_line += "\\n"
      curr_line += l
  if curr_line:
    merged_lines.append(curr_line);
  return merged_lines


def read_and_strip_file(file_name):
  lines = []
  f = open(file_name)
  past_header = False;
  for l in f:
    # idx = l.find('//')
    # if idx != -1:
    #   l = l[:idx]
    l = l.rstrip();
    uil = l.strip()
    if uil == '':
      if past_header:
        lines.append(l)
    elif uil.startswith('#include'):
      pass
    elif uil == '#pragma once':
      pass
    else:
      past_header = True
      if needs_static_qualifier(l):
        l = 'static ' + l
      lines.append(l)
  return lines


def read_and_strip_files(directory, file_names):
  blocks = [];
  for i, fn in enumerate(file_names):
    lines = read_and_strip_file(directory + '/' + fn)
    blocks.append((fn, lines))
  return blocks

################################################################################

header = [
  '#include <ctype.h>',
  '#include <math.h>',
  '#include <stddef.h>',
  '#include <stdio.h>',
  '#include <stdlib.h>',
  '#include <string.h>',
  '#include <unistd.h>',
  '#include <time.h>',
  '',
  '#include <algorithm>',
  '#include <exception>',
  '#include <iostream>',
  '#include <string>',
  '#include <vector>',
  '',
  '#include <cstdint>',
  '#include <cstddef>',
  '#include <cmath>',
  '#include <functional>',
  '#include <algorithm>',
  '#include <iterator>',
  '#include <utility>',
  '#include <type_traits>',
  '',
  '#include <sys/mman.h>',
  '',
  ''
]

################################################################################

from sys import argv, exit

if len(argv) != 3:
  print 'Usage: ' + argv[0] + ' <input directory> <output file>'
  exit(0)

_, input_dir, out_fname = argv

blocks = read_and_strip_files(input_dir, ['../external/flat-hash-map.h'] + hdr_files + src_files)

out_file = open(out_fname, 'w')

for l in header:
  out_file.write(l + '\n')

for i, (f, ls) in enumerate(blocks):
  if i > 0:
    out_file.write('\n')

  out_file.write(80 * '/' + '\n')
  l = ' ' + f + ' '
  l = ((80 - len(l)) / 2) * '/' + l
  l = l + (80 - len(l)) * '/'
  out_file.write(l + '\n')
  out_file.write(80 * '/' + '\n\n')

  for l in ls:
    out_file.write(l + '\n');










# def convert_file(file_name, keep_all):
#   lines = []
#   f = open(file_name)
#   past_header = False
#   header = []
#   for l in f:
#     l = l.rstrip()
#     uil = l.strip()
#     if uil.startswith('Debug.Assert'):
#       l = (len(l) - len(uil)) * ' ' + 'System.Diagnostics.' + uil
#     past_header = past_header or (l != "" and not l.startswith('using '))
#     if keep_all or past_header:
#       el = ''.join([escape(ord(ch)) for ch in l])
#       if past_header:
#         lines.append(el)
#       else:
#         header.append(el)

#   return ['"' + l + '"' for l in header + merge_lines(lines)]


# def to_code(bytes):
#   count = len(bytes)
#   ls = []
#   l = ' '
#   for i, b in enumerate(bytes):
#     l += ' ' + str(b) + (',' if i < count-1 else '')
#     if len(l) > 80:
#       ls.append(l)
#       l = ' '
#   if l:
#     ls.append(l)
#   return ls


# def convert_files(directory, file_names, keep_all):
#   ls = []
#   for i, f in enumerate(file_names):
#     if i > 0:
#       ls.extend(['""', '""'])
#     ls.extend(convert_file(directory + '/' + f, keep_all))
#   return ['  ' + l for l in ls]


# def data_array_def(array_name, directory, file_names, keep_all):
#   lines = convert_files(directory, file_names, keep_all)
#   # code = to_code(lines)
#   if len(lines) <= 500:
#     lines = [l + (',' if i < len(lines) - 1 else '') for i, l in enumerate(lines)]
#     return ['String* ' + array_name + ' = ('] + lines + [');']
#   code = []
#   count = (len(lines) + 499) / 500;
#   for i in range(count):
#     code += ['String* ' + array_name + '_' + str(i) + ' = (']
#     chunk = lines[500 * i : 500 * (i + 1)]
#     code += [l + (',' if i < len(chunk) - 1 else '') for i, l in enumerate(chunk)]
#     code += [');', '', '']
#   pieces = [array_name + '_' + str(i) for i in range(count)]
#   code += ['String* ' + array_name + ' = ' + ' & '.join(pieces) + ';']
#   return code
