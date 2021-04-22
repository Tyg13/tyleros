#include "elf.h"

#include "memory.h"

namespace elf {

static const auto KERNEL_ELF_HEADER = *reinterpret_cast<elf_header*>(&__KERNEL_VMA_START__);

const elf_header & kernel_header() {
    return KERNEL_ELF_HEADER;
}

}
