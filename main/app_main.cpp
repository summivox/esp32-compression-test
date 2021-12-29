#include <memory>

#include "driver/gpio.h"
#include "esp_err.h"
#include "fmt/core.h"
#include "scope_guard/scope_guard.hpp"

#include "common/macros.hpp"
#include "io/sd_card_daemon.hpp"

using fmt::print;

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

void Test() {
  static constexpr char TAG[] = "miniz";
  // TODO
}

extern "C" void app_main(void) {
  print(
      "\n\n\n\n"
      "FS playground\n\n\n\n");

  CHECK_OK(SetupSdCard());
  CHECK_OK(g_sd_card->Start(nullptr));
  while (!g_sd_card->CheckIsCardWorking()) {
    vTaskDelay(pdMS_TO_TICKS(1));
  }
  ESP_LOGI(TAG, "SD card mounted; starting test");
  Test();
  ESP_LOGI(TAG, "done");
  while (1) {
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}
