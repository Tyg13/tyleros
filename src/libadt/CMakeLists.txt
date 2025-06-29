add_library(adt INTERFACE
  array.h
  buffer.h
  hash_map.h
  intrusive_bitmap.h
  optional.h
  range.h
  ring_buffer.h
  small_string.h
  stack_string.h
)
target_include_directories(adt INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
