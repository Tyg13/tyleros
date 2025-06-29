#include "pit.h"
#include "pic.h"
#include "util/io.h"

constexpr static io::port<io::readwrite> PIT_0_DATA{0x40};
constexpr static io::port<io::readwrite> PIT_COMMAND{0x43};

constexpr static uint8_t PIT_BINARY = 0x0;

constexpr static uint8_t PIT_LO_BYTE_HI_BYTE = 0x3 << 4;

constexpr static uint8_t PIT_MODE_2 = 0x2 << 1;
constexpr static uint8_t PIT_MODE_3 = 0x3 << 1;

constexpr static uint8_t PIT_SELECT_CHANNEL_0 = 0x0 << 6;

void init_pit() {
  io::outb(PIT_COMMAND, PIT_SELECT_CHANNEL_0 | PIT_MODE_2 |
                            PIT_LO_BYTE_HI_BYTE | PIT_BINARY);
  // The base reload frequency is divided by the reload count to obtain the
  // final PIT frequency. Since the reload count is 16 bit, max is technically
  // 0xFFFF, but a value of 0 is interpreted as meaning a reload count of
  // 0x10000
  //
  // Ignore above for now
  constexpr uint8_t reload_count_lo = 0x00;
  constexpr uint8_t reload_count_hi = 0x01;
  io::outb(PIT_0_DATA, reload_count_lo);
  io::outb(PIT_0_DATA, reload_count_hi);

  // Unmask the PIT interrupt
  pic::unmask_irq(irq::PIT);
}
