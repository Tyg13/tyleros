#include "dma.h"

#include "floppy.h"
#include "low_memory_allocator.h"
#include "memory.h"
#include "util.h"
#include "util/io.h"

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

static void *floppy_dma_buffer = nullptr;

void set_buffer_for_channel(int channel, void *buffer) {
  switch (channel) {
  case FLOPPY_CHANNEL:
    floppy_dma_buffer = buffer;
    break;
  default:
    panic("Invalid DMA channel %d", channel);
  }
}

void *buffer_for_channel(int channel) {
  switch (channel) {
  case 2:
    return floppy_dma_buffer;
  default:
    return nullptr;
  }
}

void prepare_transfer(int channel, void *buffer, uint16_t transfer_size,
                      mode mode) {
  const auto dma_buffer = buffer_for_channel(channel);
  if (dma_buffer == nullptr)
    return;

  // if a device is reading, the DMA controller is writing, and vice versa
  const auto read_or_write = (mode == mode::read) ? MODE_WRITE : MODE_READ;
  const auto transfer_count = transfer_size - 1;
  io::out(SINGLE_CHANNEL_MASK, MASK_ON | channel);
  io::out(FLIP_FLOP_RESET, 0xFF);
  io::out(START_ADDRESS_CHANNEL_2,
          (reinterpret_cast<uintptr_t>(dma_buffer)) & 0xFF);
  io::out(START_ADDRESS_CHANNEL_2,
          (reinterpret_cast<uintptr_t>(dma_buffer) >> 8) & 0xFF);
  io::out(FLIP_FLOP_RESET, 0xFF);
  io::out(COUNT_CHANNEL_2, (transfer_count)&0xFF);
  io::out(COUNT_CHANNEL_2, (transfer_count >> 4) & 0xFF);
  io::out(PAGE_ADDRESS_CHANNEL_2,
          (reinterpret_cast<uintptr_t>(dma_buffer) >> 16) & 0xFF);
  io::out(MODE_REGISTER,
          SINGLE_TRANSFER | AUTOINCREMENT | read_or_write | channel);
  io::out(SINGLE_CHANNEL_MASK, channel);
}

} // namespace dma
