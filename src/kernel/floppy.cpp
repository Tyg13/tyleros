#include "floppy.h"

#include "cmos.h"
#include "pic.h"
#include "util.h"
#include "util/io.h"

constexpr static auto MT  = 1 << 7;
constexpr static auto MFM = 1 << 6;

constexpr static auto COMMAND_SPECIFY         = 0x03;
constexpr static auto COMMAND_UNLOCK          = 0x04;
constexpr static auto COMMAND_WRITE           = 0x05;
constexpr static auto COMMAND_READ            = 0x06;
constexpr static auto COMMAND_RECALIBRATE     = 0x07;
constexpr static auto COMMAND_SENSE_INTERRUPT = 0x08;
constexpr static auto COMMAND_VERSION         = 0x10;
constexpr static auto COMMAND_CONFIGURE       = 0x13;

constexpr static uint16_t DIGITAL_OUTPUT_REGISTER        = 0x3F2;
constexpr static uint16_t MAIN_STATUS_REGISTER           = 0x3F4;
constexpr static uint16_t DATA_FIFO                      = 0x3F5;
constexpr static uint16_t CONFIGURATION_CONTROL_REGISTER = 0x3F7;

constexpr static auto STATUS_RQM     = 1 << 7;
constexpr static auto STATUS_DIO     = 1 << 6;
constexpr static auto STATUS_CMD_BSY = 1 << 4;
constexpr static auto STATUS_ACTA    = 1 << 0;

volatile bool disk_interrupt_handled = false;
static void wait_for_disk_interrupt() {
   while (!disk_interrupt_handled) {
      asm volatile ("pause");
   }
   disk_interrupt_handled = false;
}

[[noreturn]] static void fail(const char * message) {
   panic("Error initializing floppy: %s", message);
}
const auto fail_if = [](const bool val, const char * message = "") {
   if (val) {
      fail(message);
   }
};

const auto issue_parameter_command = [](const auto param) {
   auto status = io::in(MAIN_STATUS_REGISTER);
   while (((status = io::in(MAIN_STATUS_REGISTER)) & STATUS_RQM) == 0) {
      asm volatile("pause");
   }
   if ((status & STATUS_DIO) == 0) {
      io::out(DATA_FIFO, param);
      return true;
   }
   return false;
};

template <int N = 1, typename ... Args>
auto issue_command_with_result(uint16_t command, uint8_t (*result)[N], Args&& ... args) {
   auto retries = 5;
   while (retries-- > 0) {
      // Check RQM = 1 and DIO = 0
      auto status = io::in(MAIN_STATUS_REGISTER);
      if (status & STATUS_RQM && (status & STATUS_DIO) == 0) {
         io::out(DATA_FIFO, command);
         const bool commands_succeeded = (issue_parameter_command(args) && ...);
         if (!commands_succeeded) {
            continue;
         }
         if (result) {
            const auto command_lo = command & 0xF;
            if (command_lo == COMMAND_READ || command_lo == COMMAND_WRITE) {
               wait_for_disk_interrupt();
            }
            auto status = io::in(MAIN_STATUS_REGISTER);
            while (((status = io::in(MAIN_STATUS_REGISTER)) & STATUS_RQM) == 0) {
               asm volatile("pause");
            }
            int result_index = 0;
            while ((status = io::in(MAIN_STATUS_REGISTER) & STATUS_DIO)) {
               fail_if(result_index >= N, "Result buffer not large enough!");
               (*result)[result_index] = io::in(DATA_FIFO) << result_index;
               ++result_index;
            }
         }
         while (((status = io::in(MAIN_STATUS_REGISTER)) & STATUS_RQM) == 0) {
            asm volatile("pause");
         }
         if (status & STATUS_RQM 
               && (status & STATUS_DIO) == 0
               && (status & STATUS_CMD_BSY) == 0) {
            // Command succeeded, we can stop retrying
            return;
         }
      }
   }
   fail("issuing floppy command");
}
const auto issue_command = [](const auto command, const auto ... args) {
   return issue_command_with_result<1>(command, nullptr, args...);
};

void init_floppy_driver() {
   // Send version command, verify result is 0x90
   {
      uint8_t result[1];
      issue_command_with_result(COMMAND_VERSION, &result);
      uint8_t version = result[0];
      fail_if(version != 0x90);
   }

   // Configure controller to polling off, FIFO on,
   // threshold = 8, implied seek on, precompensation 0 
   const auto implied_seek    = 1 << 6;
   const auto disable_polling = 1 << 4;
   const auto threshold_val   = 8 - 1;
   issue_command(COMMAND_CONFIGURE, 0, implied_seek | disable_polling | threshold_val, 0);

   // Lock controller configuration
   {
      uint8_t result[1];
      issue_command_with_result(COMMAND_UNLOCK | MT, &result);
   }

   // Reset controller
   const auto orig_dor_value = io::in(DIGITAL_OUTPUT_REGISTER);
   io::out(DIGITAL_OUTPUT_REGISTER, 0);
   sleep(4);
   io::out(DIGITAL_OUTPUT_REGISTER, orig_dor_value);
   
   unmask_irq(6);
   wait_for_disk_interrupt();

   // Recalibrate the drive
   io::out(CONFIGURATION_CONTROL_REGISTER, 0);
   const auto srt = 8;
   const auto hlt = 5;
   const auto hut = 0;
   issue_command(COMMAND_SPECIFY, srt << 4 | hut, hlt << 1);
   io::out(DIGITAL_OUTPUT_REGISTER, 1 << 4 | 1 << 3 | 1 << 2 | 0);

   issue_command(COMMAND_RECALIBRATE, 0);
   {
      uint8_t result[2];
      issue_command_with_result(COMMAND_SENSE_INTERRUPT, &result);
      uint8_t st0 = result[0] & 0xFF;
      fail_if(st0 != 0x20);
   }
}

constexpr static auto SECTORS_PER_HEAD = 18;
constexpr static auto HEADS = 2;
constexpr static auto SECTORS_PER_CYLINDER = SECTORS_PER_HEAD * HEADS;

struct CHS {
   int cylinder;
   int head;
   int sector;
};

static auto sector_to_chs(int sector) {
   return CHS {
      .cylinder = sector / SECTORS_PER_CYLINDER,
      .head     = (sector % SECTORS_PER_CYLINDER) / SECTORS_PER_HEAD,
      .sector   = (sector % SECTORS_PER_CYLINDER) % SECTORS_PER_HEAD + 1,
   };
}

const void * floppy_dma_buffer = reinterpret_cast<void *>(0x2000);

void read_floppy_sector(int lba) {
   const auto [cylinder, head, sector] = sector_to_chs(lba);
   const auto end_of_track = (lba / SECTORS_PER_HEAD + 1) * SECTORS_PER_HEAD;
   uint8_t result[8];
   issue_command_with_result(COMMAND_READ | MFM | MT, &result,
      head << 2,
      cylinder,
      head,
      sector,
      2,
      end_of_track,
      0x1b,
      0xff
   );
      
}
