#include "cmos.h"

#include "pic.h"

#include "util/io.h"

uint64_t ticks_since_boot = 0;

namespace cmos {
constexpr static io::port<io::write> CMOS_COMMAND{0x70};
constexpr static io::port<io::readwrite> CMOS_DATA{0x71};

constexpr static uint8_t CMOS_NMI_DISABLE = 0x80;

constexpr static uint8_t RTC_SELECT_A = 0x0A;
constexpr static uint8_t RTC_SELECT_B = 0x0B;
constexpr static uint8_t RTC_SELECT_C = 0x0C;

constexpr static uint8_t RTC_ENABLE_INTERRUPT = 1 << 6;

static bool is_initialized = false;
bool initialized() { return is_initialized; }

void init() {
  // Read value of status register B
  io::outb(CMOS_COMMAND, CMOS_NMI_DISABLE | RTC_SELECT_B);
  const auto value = io::inb(CMOS_DATA);

  // Reselect B (reading resets the CMOS register)
  io::outb(CMOS_COMMAND, CMOS_NMI_DISABLE | RTC_SELECT_B);

  // Toggle the interrupt bit
  io::outb(CMOS_DATA, value | RTC_ENABLE_INTERRUPT);

  // Read status register C just in case there were any pending interrupts
  io::outb(CMOS_COMMAND, CMOS_NMI_DISABLE | RTC_SELECT_C);
  io::inb(CMOS_DATA);

  // Unmask the RTC interrupt
  pic::unmask_irq(irq::CMOS_RTC);

  is_initialized = true;
}

uint64_t get_ticks_since_boot() { return ticks_since_boot; }
} // namespace cmos
