#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

struct memory_map_entry {
    enum class type : uint32_t {
        usable      = 1,
        reserved    = 2,
        reclaimable = 3,
        nvs         = 4,
        badmemory   = 5,
    };
    char    * base;
    uint64_t  length;
    type      type;
    uint32_t  extended_attributes = 1;
}
__attribute((packed));

void init_memory();

void * get_physical_page();

#endif
