#include "floppy.h"

#include "dma.h"
#include "low_memory_allocator.h"
#include "panic.h"
#include "pic.h"
#include "timing.h"
#include "util.h"
#include "util/io.h"

#include <string.h>
#include <assert.h>

constexpr static auto MT = 1 << 7;
constexpr static auto MFM = 1 << 6;

constexpr static auto COMMAND_SPECIFY = 0x03;
constexpr static auto COMMAND_WRITE = 0x05;
constexpr static auto COMMAND_READ = 0x06;
constexpr static auto COMMAND_RECALIBRATE = 0x07;
constexpr static auto COMMAND_SENSE_INTERRUPT = 0x08;
constexpr static auto COMMAND_SEEK = 0x0F;
constexpr static auto COMMAND_VERSION = 0x10;
constexpr static auto COMMAND_CONFIGURE = 0x13;
constexpr static auto COMMAND_UNLOCK = 0x14;
constexpr static auto COMMAND_LOCK = COMMAND_UNLOCK | MT;

constexpr static io::port<io::readwrite> DIGITAL_OUTPUT_REGISTER{0x3F2};
constexpr static io::port<io::read> MAIN_STATUS_REGISTER{0x3F4};
constexpr static io::port<io::write> DATARATE_SELECT_REGISTER{0x3F4};
constexpr static io::port<io::readwrite> DATA_FIFO{0x3F5};
constexpr static io::port<io::read> DIGITAL_INPUT_REGISTER{0x3F7};
constexpr static io::port<io::write> CONFIGURATION_CONTROL_REGISTER{0x3F7};

constexpr static auto MSR_RQM = 1 << 7;
constexpr static auto MSR_DIO = 1 << 6;
constexpr static auto MSR_CMD_BSY = 1 << 4;

// If set, controller is in reset mode.
constexpr static auto DOR_RESET = 1 << 2;
// If set, IRQs and DMA are enabled.
constexpr static auto DOR_IRQ = 1 << 3;
// When set, drive 0's motor is enabled.
constexpr static auto DOR_MOTA = 1 << 4;
// When set, drive 1's motor is enabled.
constexpr static auto DOR_MOTB = 1 << 5;
// When set, drive 2's motor is enabled.
constexpr static auto DOR_MOTC = 1 << 6;
// When set, drive 3's motor is enabled.
constexpr static auto DOR_MOTD = 1 << 7;

uint8_t g_drive_number = 0;

volatile bool disk_interrupt_handled = false;
void handle_floppy_interrupt() {
  disk_interrupt_handled = true;
}

static void wait_for_disk_interrupt() {
  assert(!pic::irq_is_masked(irq::FLOPPY) &&
         "floppy interrupt was not enabled, can't wait for disk interrupt!");
  SPIN_UNTIL(disk_interrupt_handled);
  disk_interrupt_handled = false;
}

static void wait_til_fifo_ready() {
  SPIN_UNTIL((io::inb(MAIN_STATUS_REGISTER)&MSR_RQM) != 0);
}

const bool issue_parameter_command(uint8_t param) {
  wait_til_fifo_ready();

  if ((io::inb(MAIN_STATUS_REGISTER) & MSR_DIO) == 0) {
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
    if ((status & MSR_RQM) == 0 || (status & MSR_DIO) != 0) {
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
      while ((status = io::inb(MAIN_STATUS_REGISTER) & MSR_DIO)) {
        assert(result_index < N && "Result buffer not large enough!");
        (*result)[result_index++] = io::inb(DATA_FIFO);
      }
    }

    wait_til_fifo_ready();

    status = io::inb(MAIN_STATUS_REGISTER);
    if ((status & MSR_RQM) != 0 && (status & MSR_DIO) == 0 &&
        (status & MSR_CMD_BSY) == 0) {
      // Command succeeded, we can stop retrying
      return;
    }
  }
  printf("command=%x\n", (int32_t)command);
  assert(false && "ran out of tries issuing floppy command");
}

template <typename... Args>
auto issue_command_with_result(uint16_t command, uint8_t *result,
                               const Args ...args) {
  return issue_command_with_result(command, (uint8_t(*)[1])result, args...);
}

template <typename... Args>
auto issue_command(uint16_t command, const Args... args) {
  return issue_command_with_result<1>(command, nullptr, args...);
};

void init_floppy_driver(uint8_t drive_number) {
  g_drive_number = drive_number;

  // Send version command, verify result is 0x90
  {
    uint8_t version;
    issue_command_with_result(COMMAND_VERSION, &version);
    assert(version == 0x90 && "invalid floppy version!");
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
    issue_command_with_result(COMMAND_LOCK, &result);
    const auto is_locked = (bool)(result[0] >> 4);
    assert(is_locked && "controller couldn't be locked!");
  }

  // Reset controller
  pic::unmask_irq(irq::FLOPPY);
  const auto orig_dor_value = io::inb(DIGITAL_OUTPUT_REGISTER);
  io::outb(DIGITAL_OUTPUT_REGISTER, 0);
  busy_sleep(4_us);
  io::outb(DIGITAL_OUTPUT_REGISTER, orig_dor_value);
  wait_for_disk_interrupt();

  // Recalibrate the drive
  io::outb(CONFIGURATION_CONTROL_REGISTER, 0);
  const auto srt = 8;
  const auto hlt = 5;
  const auto hut = 0;
  issue_command(COMMAND_SPECIFY, srt << 4 | hut, hlt << 1);

  assert(drive_number < 4 && "invalid floppy drive number!");
  constexpr static int enable_drive_motor[4] = {DOR_MOTA, DOR_MOTB, DOR_MOTC, DOR_MOTD};
  io::outb(DIGITAL_OUTPUT_REGISTER, enable_drive_motor[drive_number] | DOR_IRQ |
                                        DOR_RESET | drive_number);
  busy_sleep(4_us);

  issue_command(COMMAND_RECALIBRATE, drive_number);
  {
    uint8_t result[2];
    issue_command_with_result(COMMAND_SENSE_INTERRUPT, &result);
    uint8_t st0 = result[0];
    assert((st0 & 0x20) != 0);
  }

  void *floppy_dma_buffer = low_memory::allocate(FLOPPY_BUFFER_SIZE);
  dma::set_buffer_for_channel(dma::FLOPPY_CHANNEL, floppy_dma_buffer,
                              FLOPPY_BUFFER_SIZE);
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

floppy_status read_floppy(void *buffer, int lba, int sector_count) {
  const auto [cylinder, head, sector] = lba_to_chs(lba);
  fprintf(stderr,
          "reading %d sector(s) from lba %d (%d C, %d H, %d S) to %p... ",
          sector_count, lba, cylinder, head, sector, buffer);
  const auto transfer_size = BYTES_PER_SECTOR * sector_count;
  const void *dma_buffer = dma::prepare_transfer(
      dma::FLOPPY_CHANNEL, transfer_size, dma::mode::read);

  const auto end_of_track = 18;

  uint8_t result[7] = {0};
  issue_command_with_result(COMMAND_READ | MFM | MT, &result,
                            head << 2 | g_drive_number, cylinder, head, sector,
                            2, end_of_track, 0x1b, 0xff);

  uint8_t st0 = result[0];
  if ((st0 & 0xC0) != 0) {
    uint8_t st1 = result[1];
    if ((st1 & 0x80) != 0) {
      fprintf(stderr, " FAILED: end of cylinder (tried to access sector beyond "
                      "final sector of track)\n");
      return floppy_status::end_of_cylinder;
    }
    if ((st1 & 0x20) != 0) {
      fprintf(stderr, " FAILED: data failed crc check\n");
      return floppy_status::data_error;
    }
    if ((st1 & 0x10) != 0) {
      fprintf(stderr, " FAILED: did not receive DMA service within the "
                      "required time interval; data underrun/overrun\n");
      return floppy_status::overrun_underrun;
    }
    if ((st1 & 0x4) != 0) {
      fprintf(stderr, " FAILED: could not find the specified sector\n");
      return floppy_status::no_data;
    }
    if ((st1 & 0x2) != 0) {
      fprintf(stderr, " FAILED: media write-protected\n");
      return floppy_status::not_writable;
    }
    if ((st1 & 0x1) != 0) {
      fprintf(stderr, " FAILED: missing address mark\n");
      return floppy_status::missing_address_mark;
    }
    fprintf(stderr, " FAILED: unknown error\n");
    return floppy_status::error_unknown;
  }

  memcpy(buffer, dma_buffer, transfer_size);
  fprintf(stderr, " OK\n");
  return floppy_status::ok;
}

void read_floppy_or_fail(void *buffer, int lba, int sector_count) {
  switch (read_floppy(buffer, lba, sector_count)) {
  case floppy_status::ok:
    return;
  default:
    kstd::panic("error reading floppy!");
  }
}
