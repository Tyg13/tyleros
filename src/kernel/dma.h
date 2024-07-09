#ifndef DMA_H
#define DMA_H

#include <stddef.h>
#include <stdint.h>

namespace dma {
struct buffer {
  void *data = nullptr;
  size_t size = 0;
};

enum class mode { read, write };
constexpr static auto FLOPPY_CHANNEL = 2;
void *prepare_transfer(int channel, uint16_t transfer_size, mode mode);

void set_buffer_for_channel(int channel, void *data, size_t size);
buffer *buffer_for_channel(int channel);

} // namespace dma

#endif
