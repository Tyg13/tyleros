#include "dma.h"

#include "util/io.h"
#include "floppy.h"

constexpr auto START_ADDRESS_CHANNEL_2 = 0x04;
constexpr auto COUNT_CHANNEL_2         = 0x05;
constexpr auto SINGLE_CHANNEL_MASK     = 0x0A;
constexpr auto MODE_REGISTER           = 0x0B;
constexpr auto FLIP_FLOP_RESET         = 0x0C;
constexpr auto PAGE_ADDRESS_CHANNEL_2  = 0x81;
constexpr auto MASK_ON                 = 1 << 2;

constexpr auto SINGLE_TRANSFER         = 1 << 6;
constexpr auto AUTOINCREMENT           = 1 << 4;
constexpr auto MODE_READ               = 2 << 2;
constexpr auto MODE_WRITE              = 1 << 2;

static void init_floppy_dma();

void init_dma() {
   init_floppy_dma();
}

void init_floppy_dma() {
   io::out(SINGLE_CHANNEL_MASK, MASK_ON | 2);
   io::out(FLIP_FLOP_RESET, 0xFF);
   io::out(START_ADDRESS_CHANNEL_2, (reinterpret_cast<uintptr_t>(floppy_dma_buffer)      ) & 0xFF);
   io::out(START_ADDRESS_CHANNEL_2, (reinterpret_cast<uintptr_t>(floppy_dma_buffer) >>  8) & 0xFF);
   io::out(FLIP_FLOP_RESET, 0xFF);
   const auto transfer_count = 0x100 - 1;
   io::out(COUNT_CHANNEL_2, (transfer_count     ) & 0xFF);
   io::out(COUNT_CHANNEL_2, (transfer_count >> 4) & 0xFF);
   io::out(PAGE_ADDRESS_CHANNEL_2,  (reinterpret_cast<uintptr_t>(floppy_dma_buffer) >> 16) & 0xFF);
   io::out(MODE_REGISTER, SINGLE_TRANSFER | AUTOINCREMENT | MODE_WRITE | 2);
   io::out(SINGLE_CHANNEL_MASK, 2);
}
