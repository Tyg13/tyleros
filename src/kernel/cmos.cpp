#include "cmos.h"

#include "pic.h"

#include "util.h"
#include "util/io.h"

uint64_t ticks_since_boot = 0;
uint64_t get_ticks_since_boot() { return ticks_since_boot; }

void init_real_time_clock() {
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
  unmask_irq(irq::CMOS_RTC);
}

void sleep(uint64_t ticks) {
  const auto start_ticks = get_ticks_since_boot();
  SPIN_WHILE(get_ticks_since_boot() - start_ticks > ticks);
}
