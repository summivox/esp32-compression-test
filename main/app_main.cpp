#include <map>
#include <memory>

#include "argtable3/argtable3.h"
#include "cmd_system.h"
#include "driver/gpio.h"
#include "esp_console.h"
#include "esp_err.h"
#include "scope_guard/scope_guard.hpp"

#include "common/command.hpp"
#include "common/command_registry.hpp"
#include "common/macros.hpp"
#include "io/fs_utils.hpp"
#include "io/sd_card_daemon.hpp"

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

DEFINE_COMMAND(
    ls,
    "list all directories and files under given path",
    /*hint*/ nullptr,
    {
      arg_lit* time = arg_lit0("t", "time", "show time");
      arg_str* path = arg_str1(nullptr, nullptr, "<path>", "path to list");
    },
    /*num_end*/ 1) {
  CHECK(path->count == 1);

  std::string dir = "/s";
  dir += path->sval[0];

  for (dirent* p : io::DirIter(dir.c_str())) {
    printf("name='%s' type=%d (%s)", p->d_name, p->d_type, kDirentTypeName.at(p->d_type).c_str());
    if (p->d_type == DT_REG) {
      struct stat s {};
      stat((dir + "/" + p->d_name).c_str(), &s);
      TimeParts mt = ToParts(s.st_mtim);
      printf(" size=%d", (int)s.st_size);
      if (time->count) {
        // st_ctim is not populated, and we don't have st_birthtim / st_birthtimespec
        printf(
            " mtim=%04d-%02d-%02d_%02d:%02d:%02d",
            mt.tm_year + 1900,
            mt.tm_mon + 1,
            mt.tm_mday,
            mt.tm_hour,
            mt.tm_min,
            mt.tm_sec);
      }
    }
    printf("\n");
  }

  return 0;
}

esp_console_repl_t* InitializeConsole() {
  esp_console_repl_t* repl = nullptr;
  esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
  repl_config.prompt = "rice>";
  esp_console_dev_uart_config_t repl_uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
  OK_OR_RETURN(esp_console_new_repl_uart(&repl_uart_config, &repl_config, &repl), nullptr);
  register_system_common();
  OK_OR_RETURN(CommandRegistry::GetInstance()->Register(), nullptr);
  return repl;
}

extern "C" void app_main(void) {
  esp_console_repl_t* repl = InitializeConsole();
  printf(
      "\n\n\n\n"
      "FS playground\n\n\n\n"
      "Awaiting SD card mount...\n");

  CHECK_OK(SetupSdCard());
  CHECK_OK(g_sd_card->Start(nullptr));
  while (!g_sd_card->CheckIsCardWorking()) {
    vTaskDelay(pdMS_TO_TICKS(1));
  }
  printf("done.\n");
  esp_console_start_repl(repl);

  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}
