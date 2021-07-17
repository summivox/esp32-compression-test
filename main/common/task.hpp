// Authors: summivox@gmail.com

#pragma once

#include <atomic>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "common/macros.hpp"

/// Bare-minimum wrapper base class for a FreeRTOS task. This allows encapsulation of a task
/// alongside with its context.
///
/// \example
/// \code{.cpp}
/// class MyTask : public Task {
///  public:
///   virtual ~MyTask() = default;
///   void Start() { Task::SpawnSame(/* ... */); }
///   void Stop() { Task::Kill(); }
///
///  protected:
///   void Run() override {
///     // the task entry point is here
///     while (true) {
///       counter_++;
///       vTaskDelay(1);
///     }
///   }
///
///  private:
///   int counter_ = 0;
///   /* ... */
/// };
/// \endcode
class Task {
 public:
  virtual ~Task() { Kill(); }

  /// \returns FreeRTOS task handle if the task has been started;
  ///          `nullptr` otherwise.
  TaskHandle_t handle() const { return handle_.load(); }

 protected:
  explicit Task() {}

  /// Entry point for the task (must be overridden by the subclass and never return)
  [[noreturn]] virtual void Run() = 0;

  /// Starts the task without specifying CPU core affinity.
  /// If the task has already been started, it will be killed then restarted.
  /// \param name           FreeRTOS task name.
  /// \param stack_depth    Size of the FreeRTOS stack allocated for this task, in Bytes.
  /// \param priority       FreeRTOS priority (higher number is more important).
  esp_err_t Spawn(const char* name, uint32_t stack_depth, uint32_t priority) {
    Kill();
    TaskHandle_t handle = nullptr;
    if (xTaskCreate(RunAdapter, name, stack_depth, this, priority, &handle) != pdTRUE) {
      return ESP_ERR_NO_MEM;
    }
    handle_.store(handle);
    return ESP_OK;
  }

  /// Starts the task on the specified CPU core.
  /// If the task has already been started, it will be killed then restarted.
  /// \param name           FreeRTOS task name.
  /// \param stack_depth    Size of the FreeRTOS stack allocated for this task, in Bytes.
  /// \param priority       FreeRTOS priority (higher number is more important).
  /// \param cpu            Index of the core (e.g. `PRO_CPU_NUM`, `APP_CPU_NUM`).
  esp_err_t SpawnPinned(const char* name, uint32_t stack_depth, uint32_t priority, BaseType_t cpu) {
    Kill();
    TaskHandle_t handle = nullptr;
    if (xTaskCreatePinnedToCore(RunAdapter, name, stack_depth, this, priority, &handle, cpu) !=
        pdTRUE) {
      return ESP_ERR_NO_MEM;
    }
    handle_.store(handle);
    return ESP_OK;
  }

  /// Starts the task on the same CPU as the caller.
  /// If the task has already been started, it will be killed then restarted.
  /// \param name           FreeRTOS task name.
  /// \param stack_depth    Size of the FreeRTOS stack allocated for this task, in Bytes.
  /// \param priority       FreeRTOS priority (higher number is more important).
  esp_err_t SpawnSame(const char* name, uint32_t stack_depth, uint32_t priority) {
    return SpawnPinned(name, stack_depth, priority, xPortGetCoreID());
  }

  /// Stops the task if not already stopped.
  void Kill() {
    const TaskHandle_t handle = handle_.exchange(nullptr);
    if (handle) {
      vTaskDelete(handle);
    }
  }

 private:
  std::atomic<TaskHandle_t> handle_ = nullptr;
  static_assert(
      std::atomic<TaskHandle_t>::is_always_lock_free,
      "Task wrapper class is not usable on this platform.");

  static void RunAdapter(void* self) { reinterpret_cast<Task*>(self)->Run(); }

  NON_COPYABLE_NOR_MOVABLE(Task)
};
