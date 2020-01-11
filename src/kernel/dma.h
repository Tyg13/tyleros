#ifndef DMA_H
#define DMA_H

#include <stdint.h>

enum class dma_mode {
   read,
   write
};
constexpr static auto FLOPPY_DMA_CHANNEL = 2;
void prepare_dma_transfer(int channel, void * buffer, uint16_t transfer_size, dma_mode mode);

void * dma_buffer_for_channel(int channel);

#endif
