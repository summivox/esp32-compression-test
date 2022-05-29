#include <map>
#include <memory>

#include "argtable3/argtable3.h"
#include "cmd_system.h"
#include "driver/gpio.h"
#include "esp_console.h"
#include "esp_err.h"
#include "fmt/chrono.h"
#include "fmt/core.h"
#include "scope_guard/scope_guard.hpp"

#include "common/macros.hpp"
#include "io/fs_utils.hpp"
#include "io/sd_card_daemon.hpp"

using fmt::format;
using fmt::format_to;
using fmt::print;

using FmtBuffer = fmt::basic_memory_buffer<char, 256>;

namespace {
constexpr char TAG[] = "main";

constexpr gpio_num_t kCardDetectPin = GPIO_NUM_34;
}  // namespace

std::unique_ptr<io::SdCardDaemon> g_sd_card;

esp_err_t SetupSdCard() {
  g_sd_card = io::SdCardDaemon::Create({
      .mount_config =
          {
              .format_if_mount_failed = false,
              .max_files = 4,
              .allocation_unit_size = 0,  // only needed if format
          },
      .card_detect_pin = kCardDetectPin,
      .priority = 3,
  });
  return g_sd_card ? ESP_OK : ESP_FAIL;
}

const std::map<uint8_t, std::string> kDirentTypeName{
    {DT_REG, "DT_REG"},
    {DT_DIR, "DT_DIR"},
    {DT_UNKNOWN, "DT_UNKNOWN"},
};

struct ListCommandArgs {
  arg_str* path = arg_str1(nullptr, nullptr, "<path>", "path to list");
  struct arg_end* end = arg_end(1);
} FLAGS_list;
int ListCommand(int argc, char** argv) {
  const int num_errors = arg_parse(argc, argv, (void**)&FLAGS_list);
  if (num_errors > 0) {
    arg_print_errors(stderr, FLAGS_list.end, argv[0]);
    return 0;
  }
  CHECK(FLAGS_list.path->count == 1);
  std::string dir = "/s";
  dir += FLAGS_list.path->sval[0];
  for (dirent* p : io::DirIter(dir.c_str())) {
    print("name=\'{}\' type={} ({})", p->d_name, p->d_type, kDirentTypeName.at(p->d_type));
    if (p->d_type == DT_REG) {
      struct stat s {};
      stat(format("{}/{}", dir, p->d_name).c_str(), &s);
      TimeParts mt = ToParts(s.st_mtim);
      // st_ctim is not populated, and we don't have st_birthtim / st_birthtimespec
      print(" size={} mtim={:%F_%T}", s.st_size, mt);
    }
    print("\n");
  }

  return 0;
}
void RegisterListCommand() {
  constexpr esp_console_cmd_t cmd = {
      .command = "ls",
      .help = "list everything under given path",
      .hint = nullptr,
      .func = ListCommand,
      .argtable = &FLAGS_list,
  };
  CHECK_OK(esp_console_cmd_register(&cmd));
}

extern "C" void app_main(void) {
  esp_console_repl_t* repl = nullptr;
  esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
  repl_config.prompt = "rice>";
  esp_console_dev_uart_config_t repl_uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
  repl_uart_config.baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE;
  CHECK_OK(esp_console_new_repl_uart(&repl_uart_config, &repl_config, &repl));
  register_system_common();
  RegisterListCommand();

  print(
      "\n\n\n\n"
      "FS playground\n\n\n\n"
      "Awaiting SD card mount...\n");

  CHECK_OK(SetupSdCard());
  CHECK_OK(g_sd_card->Start(nullptr));
  while (!g_sd_card->CheckIsCardWorking()) {
    vTaskDelay(pdMS_TO_TICKS(1));
  }
  print("done.\n");

  CHECK_OK(esp_console_start_repl(repl));

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}
