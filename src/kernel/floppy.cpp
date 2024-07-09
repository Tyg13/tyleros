#include "floppy.h"

#include "cmos.h"
#include "debug.h"
#include "dma.h"
#include "low_memory_allocator.h"
#include "memory.h"
#include "pic.h"
#include "util.h"
#include "util/io.h"

#include <string.h>
#include <assert.h>

constexpr static auto MT = 1 << 7;
constexpr static auto MFM = 1 << 6;

constexpr static auto COMMAND_SPECIFY = 0x03;
constexpr static auto COMMAND_UNLOCK = 0x04;
constexpr static auto COMMAND_WRITE = 0x05;
constexpr static auto COMMAND_READ = 0x06;
constexpr static auto COMMAND_RECALIBRATE = 0x07;
constexpr static auto COMMAND_SENSE_INTERRUPT = 0x08;
constexpr static auto COMMAND_VERSION = 0x10;
constexpr static auto COMMAND_CONFIGURE = 0x13;

constexpr static uint16_t DIGITAL_OUTPUT_REGISTER = 0x3F2;
constexpr static uint16_t MAIN_STATUS_REGISTER = 0x3F4;
constexpr static uint16_t DATA_FIFO = 0x3F5;
constexpr static uint16_t CONFIGURATION_CONTROL_REGISTER = 0x3F7;

constexpr static auto STATUS_RQM = 1 << 7;
constexpr static auto STATUS_DIO = 1 << 6;
constexpr static auto STATUS_CMD_BSY = 1 << 4;
constexpr static auto STATUS_ACTA = 1 << 0;

uint8_t g_drive_number = 0;

volatile bool disk_interrupt_handled = false;
static void wait_for_disk_interrupt() {
  assert(!irq_is_masked(irq::FLOPPY) &&
         "floppy interrupt was not enabled, can't wait for disk interrupt!");
  SPIN_UNTIL(disk_interrupt_handled);
  disk_interrupt_handled = false;
}

[[noreturn]] static void fail(const char *message) {
  kstd::panic("Error initializing floppy: %s", message);
}

static void fail_if(bool val, const char *message = "") {
  if (val) {
    fail(message);
  }
}

static void wait_til_fifo_ready() {
  SPIN_UNTIL((io::inb(MAIN_STATUS_REGISTER)&STATUS_RQM) != 0);
}

const bool issue_parameter_command(uint8_t param) {
  wait_til_fifo_ready();

  if ((io::inb(MAIN_STATUS_REGISTER)&STATUS_DIO) == 0) {
    io::outb(DATA_FIFO, param);
    return true;
  }
  return false;
};

template <unsigned N, typename... Args>
auto issue_command_with_result(uint16_t command, uint8_t (*result)[N],
                               Args &&...args) {
  for (int retries = 5; retries > 0; --retries) {
    uint8_t status = io::inb(MAIN_STATUS_REGISTER);
    if ((status & STATUS_RQM) == 0 || (status & STATUS_DIO) != 0) {
      continue;
    }

    io::outb(DATA_FIFO, command);
    const bool commands_succeeded = (issue_parameter_command(args) && ...);
    if (!commands_succeeded) {
      continue;
    }

    if (result != nullptr) {
      const uint16_t command_lo = command & 0xF;
      if (command_lo == COMMAND_READ || command_lo == COMMAND_WRITE) {
        wait_for_disk_interrupt();
      }
      auto status = io::inb(MAIN_STATUS_REGISTER);

      wait_til_fifo_ready();

      auto result_index = size_t{0};
      while ((status = io::inb(MAIN_STATUS_REGISTER)&STATUS_DIO)) {
        fail_if(result_index >= N, "Result buffer not large enough!");
        (*result)[result_index++] = io::inb(DATA_FIFO);
      }
    }

    wait_til_fifo_ready();

    status = io::inb(MAIN_STATUS_REGISTER);
    if ((status & STATUS_RQM) != 0 && (status & STATUS_DIO) == 0 &&
        (status & STATUS_CMD_BSY) == 0) {
      // Command succeeded, we can stop retrying
      return;
    }
  }
  debug::printf("command=%x\n", (int32_t)command);
  fail("ran out of tries issuing floppy command");
}

auto issue_command(uint16_t command, const auto... args) {
  return issue_command_with_result<sizeof...(args)>(command, nullptr, args...);
};

void init_floppy_driver(uint8_t drive_number) {
  g_drive_number = drive_number;

  // Send version command, verify result is 0x90
  {
    uint8_t result[1];
    issue_command_with_result(COMMAND_VERSION, &result);
    uint8_t version = result[0];
    fail_if(version != 0x90);
  }

  // Configure controller to polling off, FIFO on,
  // threshold = 8, implied seek on, precompensation 0
  const auto implied_seek = 1 << 6;
  const auto disable_polling = 1 << 4;
  const auto threshold_val = 8 - 1;
  issue_command(COMMAND_CONFIGURE, 0,
                implied_seek | disable_polling | threshold_val, 0);

  // Lock controller configuration
  {
    uint8_t result[1];
    issue_command_with_result(COMMAND_UNLOCK | MT, &result);
  }

  // Reset controller
  const auto orig_dor_value = io::inb(DIGITAL_OUTPUT_REGISTER);
  io::outb(DIGITAL_OUTPUT_REGISTER, 0);
  sleep(4);
  io::outb(DIGITAL_OUTPUT_REGISTER, orig_dor_value);

  unmask_irq(irq::FLOPPY);
  wait_for_disk_interrupt();

  // Recalibrate the drive
  io::outb(CONFIGURATION_CONTROL_REGISTER, 0);
  const auto srt = 8;
  const auto hlt = 5;
  const auto hut = 0;
  issue_command(COMMAND_SPECIFY, srt << 4 | hut, hlt << 1);
  io::outb(DIGITAL_OUTPUT_REGISTER, 1 << 4 | 1 << 3 | 1 << 2 | 0);

  issue_command(COMMAND_RECALIBRATE, 0);
  {
    uint8_t result[2];
    issue_command_with_result(COMMAND_SENSE_INTERRUPT, &result);
    uint8_t st0 = result[0] & 0xFF;
    fail_if((st0 & 0x20) == 0);
  }

  void *floppy_dma_buffer = low_memory::allocate(memory::PAGE_SIZE);
  dma::set_buffer_for_channel(dma::FLOPPY_CHANNEL, floppy_dma_buffer,
                              memory::PAGE_SIZE);
}

constexpr static auto SECTORS_PER_HEAD = 18;
constexpr static auto HEADS = 2;
constexpr static auto SECTORS_PER_CYLINDER = SECTORS_PER_HEAD * HEADS;

struct CHS {
  int cylinder;
  int head;
  int sector;
};

static auto lba_to_chs(int sector) {
  return CHS{
      .cylinder = sector / SECTORS_PER_CYLINDER,
      .head = (sector % SECTORS_PER_CYLINDER) / SECTORS_PER_HEAD,
      .sector = (sector % SECTORS_PER_CYLINDER) % SECTORS_PER_HEAD + 1,
  };
}

constexpr static auto BYTES_PER_SECTOR = 0x100;

floppy_status read_floppy(void *buffer, int lba, int sector_count) {
  const auto transfer_size = BYTES_PER_SECTOR * sector_count;
  const void *dma_buffer = dma::prepare_transfer(
      dma::FLOPPY_CHANNEL, transfer_size, dma::mode::read);

  const auto [cylinder, head, sector] = lba_to_chs(lba);
  const auto end_of_track = (lba / SECTORS_PER_HEAD + 1) * SECTORS_PER_HEAD;

  uint8_t result[7] = {0};
  issue_command_with_result(COMMAND_READ | MFM | MT, &result,
                            head << 2 | g_drive_number, cylinder, head, sector,
                            2, end_of_track, 0x1b, 0xff);

  uint8_t st0 = result[0];
  if ((st0 & 0xC0) != 0) {
    uint8_t st1 = result[1];
    if ((st1 & 0x80) != 0)
      return floppy_status::too_few_sectors;
    if ((st1 & 0x10) != 0)
      return floppy_status::driver_too_slow;
    if ((st1 & 0x2) != 0)
      return floppy_status::media_write_protected;
    return floppy_status::error_unknown;
  }

  memcpy(buffer, dma_buffer, transfer_size);
  return floppy_status::ok;
}

void read_floppy_or_fail(void *buffer, int lba, int sector_count) {
  switch (read_floppy(buffer, lba, sector_count)) {
  case floppy_status::ok:
    return;
  case floppy_status::too_few_sectors:
    kstd::panic("too few sectors for floppy read!");
  case floppy_status::driver_too_slow:
    kstd::panic("floppy driver too slow!");
  case floppy_status::media_write_protected:
    kstd::panic("floppy media is write protected!");
  case floppy_status::error_unknown:
    kstd::panic("unknown error trying to read floppy!");
  }
}
