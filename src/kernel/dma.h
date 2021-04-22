#ifndef DMA_H
#define DMA_H

#include <stdint.h>

namespace dma {

enum class mode {
   read,
   write
};
constexpr static auto FLOPPY_CHANNEL = 2;
void prepare_transfer(int channel, void * buffer, uint16_t transfer_size, mode mode);

void * buffer_for_channel(int channel);

}

#endif
