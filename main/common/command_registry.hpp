// Copyright 2022 summivox. All rights reserved.
// Authors: summivox@gmail.com

#pragma once

#include <vector>

#include "esp_console.h"

class CommandRegistry {
 public:
  static CommandRegistry* GetInstance();

  void AddCommand(const esp_console_cmd_t& command);
  esp_err_t Register();

 private:
  std::vector<esp_console_cmd_t> commands_;

  CommandRegistry();
};
