#include <memory>

#include "driver/gpio.h"
#include "esp32/rom/miniz.h"
#include "esp_err.h"
#include "scope_guard/scope_guard.hpp"

#include "common/macros.hpp"
#include "io/sd_card_daemon.hpp"

namespace {
constexpr char TAG[] = "main";

constexpr gpio_num_t kCardDetectPin = GPIO_NUM_34;

constexpr char kInputPath[] = "/s/input.log";
constexpr char kOutputPath[] = "/s/output.log.gz";
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

  FILE* fin = fopen(kInputPath, "rb");
  DEFER { fclose(fin); };
  FILE* fout = fopen(kOutputPath, "wb");
  DEFER { fclose(fout); };

#if 0  // TODO: insufficient RAM for compressor state
  auto compressor_owned = std::make_unique<tdefl_compressor>();
  tdefl_compressor* compressor = compressor_owned.get();
  ESP_LOGI(TAG, "compressor size: %d", sizeof(tdefl_compressor));
  tdefl_init(
      compressor,
      [](const void* pBuf, int len, void* pUser) {
        // TODO
        return 1;
      },
      nullptr,
      TDEFL_WRITE_ZLIB_HEADER);
#endif

  fseek(fin, 0, SEEK_END);
  const int input_size = ftell(fin);
  fseek(fin, 0, SEEK_SET);
  ESP_LOGI(TAG, "input size: %d", input_size);
  // TODO
}

extern "C" void app_main(void) {
  puts("\n\n\n\n");
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
