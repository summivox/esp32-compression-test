// Copyright 2021 summivox. All rights reserved.
// Authors: summivox@gmail.com

#pragma once

#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "driver/sdmmc_host.h"
#include "esp_err.h"
#include "ff.h"

#include "common/iter.hpp"
#include "common/times.hpp"

namespace io {

// We won't be dealing with permissions
constexpr mode_t kFsMode = 0777;

// Fixed value for SDHC/SDXC
constexpr int kSdSectorSize = 512;

// Root directories (without trailing slashes)
#define SD_FATFS_ROOT "0:"
#define SD_VFS_ROOT CONFIG_MOUNT_ROOT
constexpr char kFatfsRoot[] = "0:";
constexpr char kVfsRoot[] = "/s";

class DirIterImpl {
 public:
  using Item = FILINFO*;
  DirIterImpl() = default;
  DirIterImpl(const char* path);
  DirIterImpl(const std::string& path) : DirIterImpl(path.c_str()) {}
  ~DirIterImpl();

  std::optional<FILINFO*> Next();

 private:
  std::optional<FF_DIR> dir_;
  FILINFO filinfo_;
};
using DirIter = RustIter<DirIterImpl>;

using OwnedFile = std::unique_ptr<FILE, decltype(&fclose)>;

OwnedFile OpenFile(const std::string& path, const char* modestr);
esp_err_t ReadBinaryFileToString(const std::string& path, std::string* out_file_content);
esp_err_t FlushAndSync(FILE* f);

esp_err_t Mkdir(const std::string& dir);
esp_err_t MkdirParts(std::initializer_list<std::string_view> parts, std::string* out_path);

esp_err_t UpdateFileTime(const std::string& path, TimeUnix t_unix);

int32_t GetFreeSpaceSectors(const char* fatfs_root = kFatfsRoot);
int64_t GetFreeSpaceBytes(const char* fatfs_root = kFatfsRoot);

}  // namespace io
