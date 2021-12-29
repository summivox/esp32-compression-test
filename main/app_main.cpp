#include <map>
#include <memory>

#include "driver/gpio.h"
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

////////////////////////////////////////////////////////

extern "C" {
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
}
#include <optional>

class VfsDirIterImpl {
 public:
  using Item = dirent*;
  VfsDirIterImpl() = default;
  VfsDirIterImpl(const char* path) : dir_(opendir(path)) {}
  ~VfsDirIterImpl() {
    if (dir_) {
      (void)closedir(dir_);
    }
  }

  std::optional<Item> Next() {
    if (!dir_) {
      return std::nullopt;
    }
    dirent* p = readdir(dir_);
    if (p) {
      return p;
    }
    return std::nullopt;
  }

 private:
  DIR* dir_;
};
class VfsDirIter : public RustIter<VfsDirIterImpl> {
 public:
  explicit VfsDirIter(const char* path) : RustIter<VfsDirIterImpl>(path) {}
};

////////////////////////////////////////////////////////

const std::map<uint8_t, std::string> kDirentTypeName{
    {DT_REG, "DT_REG"},
    {DT_DIR, "DT_DIR"},
    {DT_UNKNOWN, "DT_UNKNOWN"},
};

void Test() {
  std::string dir = "/s/BD2AA7C0/355/0";
  // std::string dir = "/s/maps";
  for (dirent* p : VfsDirIter(dir.c_str())) {
    print("name=\'{}\' type={} ({})", p->d_name, p->d_type, kDirentTypeName.at(p->d_type));
    if (p->d_type == DT_REG) {
      struct stat s {};
      stat(format("{}/{}", dir, p->d_name).c_str(), &s);
      TimeZulu mt = ToZulu(s.st_mtim);
      // st_ctim is not populated, and we don't have st_birthtim / st_birthtimespec
      print(" size={} mtim={:%F_%T}", s.st_size, mt);
    }
    print("\n");
  }
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
