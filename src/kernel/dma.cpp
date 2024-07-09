#include "dma.h"

#include "floppy.h"
#include "low_memory_allocator.h"
#include "memory.h"
#include "paging.h"
#include "util.h"
#include "util/io.h"

#include <assert.h>

namespace dma {

constexpr auto START_ADDRESS_CHANNEL_2 = 0x04;
constexpr auto COUNT_CHANNEL_2 = 0x05;
constexpr auto SINGLE_CHANNEL_MASK = 0x0A;
constexpr auto MODE_REGISTER = 0x0B;
constexpr auto FLIP_FLOP_RESET = 0x0C;
constexpr auto PAGE_ADDRESS_CHANNEL_2 = 0x81;
constexpr auto MASK_ON = 1 << 2;

constexpr auto SINGLE_TRANSFER = 1 << 6;
constexpr auto AUTOINCREMENT = 1 << 4;
constexpr auto MODE_READ = 2 << 2;
constexpr auto MODE_WRITE = 1 << 2;

static buffer floppy_dma_buffer{};

void set_buffer_for_channel(int channel, void *data, size_t size) {
  assert(data && "Can't set null buffer!");
  assert((uintptr_t)data < (1 << 24) &&
         "Buffer address must be able to fit in 24-bits!");
  assert(paging::kernel_page_tables.get_physical_address(data) < (1 << 24) &&
         "Buffer address must be mapped in the bottom 24 bits of physical "
         "memory (DMA doesn't understand fancy virtual addresses)");
  switch (channel) {
  case FLOPPY_CHANNEL:
    floppy_dma_buffer.data = data;
    floppy_dma_buffer.size = size;
    break;
  default:
    kstd::panic("Invalid DMA channel %d", channel);
  }
}

buffer *buffer_for_channel(int channel) {
  switch (channel) {
  case 2:
    return &floppy_dma_buffer;
  default:
    return nullptr;
  }
}

void *prepare_transfer(int channel, uint16_t transfer_size, mode mode) {
  const auto buffer = buffer_for_channel(channel);
  assert(buffer && buffer->data && "floppy DMA buffer is not allocated?");
  assert(buffer->size >= transfer_size &&
         "dma buffer not large enough for transfer!");

  // if a device is reading, the DMA controller is writing, and vice versa
  const auto read_or_write = (mode == mode::read) ? MODE_WRITE : MODE_READ;
  const auto transfer_count = transfer_size - 1;
  io::outb(SINGLE_CHANNEL_MASK, MASK_ON | channel);
  io::outb(FLIP_FLOP_RESET, 0xFF);
  io::outb(START_ADDRESS_CHANNEL_2,
           (reinterpret_cast<uintptr_t>(buffer->data)) & 0xFF);
  io::outb(START_ADDRESS_CHANNEL_2,
           (reinterpret_cast<uintptr_t>(buffer->data) >> 8) & 0xFF);
  io::outb(FLIP_FLOP_RESET, 0xFF);
  io::outb(COUNT_CHANNEL_2, (transfer_count) & 0xFF);
  io::outb(COUNT_CHANNEL_2, (transfer_count >> 4) & 0xFF);
  io::outb(PAGE_ADDRESS_CHANNEL_2,
           (reinterpret_cast<uintptr_t>(buffer->data) >> 16) & 0xFF);
  io::outb(MODE_REGISTER,
           SINGLE_TRANSFER | AUTOINCREMENT | read_or_write | channel);
  io::outb(SINGLE_CHANNEL_MASK, channel);

  return buffer->data;
}

} // namespace dma
