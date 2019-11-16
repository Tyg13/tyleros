#ifndef CMOS_H
#define CMOS_H

#include <stdint.h>

void init_real_time_clock();

constexpr static uint8_t CMOS_COMMAND = 0x70;
constexpr static uint8_t CMOS_DATA    = 0x71;

constexpr static uint8_t CMOS_NMI_DISABLE  = 0x80;

constexpr static uint8_t RTC_SELECT_A = 0x0A;
constexpr static uint8_t RTC_SELECT_B = 0x0B;
constexpr static uint8_t RTC_SELECT_C = 0x0C;

constexpr static uint8_t RTC_ENABLE_INTERRUPT = 1 << 6;

uint64_t get_ticks_since_boot();

void sleep(uint64_t ticks);

#endif
