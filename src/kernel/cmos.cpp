#include "cmos.h"

#include "pic.h"

#include "util/io.h"

void init_real_time_clock() {
   // Read value of status register B
   io::out(CMOS_COMMAND, CMOS_NMI_DISABLE | RTC_SELECT_B);
   const auto value = io::in(CMOS_DATA);

   // Reselect B (reading resets the CMOS register)
   io::out(CMOS_COMMAND, CMOS_NMI_DISABLE | RTC_SELECT_B);

   // Toggle the interrupt bit
   const auto value_with_interrupt_enabled = value | RTC_ENABLE_INTERRUPT;
   io::out(CMOS_DATA, value_with_interrupt_enabled);

   // Read status register C just in case there were any pending interrupts
   io::out(CMOS_COMMAND, CMOS_NMI_DISABLE | RTC_SELECT_C);
   io::in(CMOS_DATA);

   // Unmask the RTC interrupt
   unmask_irq(8);
}
