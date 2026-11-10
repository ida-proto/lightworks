#include "app.hpp"

#include "board.h"

#include "status_led.hpp"
#include "ws2815_spi_dma.hpp"

#include "lightworks/control.hpp"
#include "lightworks/frame_buffer.hpp"
#include "lightworks/rgb.hpp"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include <cstddef>

namespace lightworks::app {
namespace {

constexpr configSTACK_DEPTH_TYPE kStatusWords{256};
constexpr configSTACK_DEPTH_TYPE kEffectWords{384};
constexpr configSTACK_DEPTH_TYPE kOutputWords{512};
constexpr UBaseType_t kStatusPriority = 1;
constexpr UBaseType_t kEffectPriority = 2;
constexpr UBaseType_t kOutputPriority = 3;
constexpr TickType_t kFramePeriodMs = 33;
constexpr Rgb kIdleColor{0, 40, 0};

FrameBuffer<BOARD_PIXEL_COUNT> frames;
h743::Ws2815SpiDma strip;
Control control;

SemaphoreHandle_t frame_ready = nullptr;
StaticSemaphore_t frame_ready_storage;

StackType_t status_stack[kStatusWords];
StackType_t effect_stack[kEffectWords];
StackType_t output_stack[kOutputWords];
StaticTask_t status_tcb;
StaticTask_t effect_tcb;
StaticTask_t output_tcb;

[[noreturn]] void halt() {
  for (;;) {
  }
}

void fill(Span<Rgb> pixels, Rgb color) {
  for (Rgb& pixel : pixels) {
    pixel = color;
  }
}

Rgb color_for_frame() {
  Rgb color = kIdleColor;
  Rgb requested{};
  if (control.poll(requested)) {
    color = requested;
  }
  return color;
}

extern "C" void status_task(void* arg) {
  (void)arg;
  TickType_t wake = xTaskGetTickCount();
  bool on = false;
  for (;;) {
    on = !on;
    h743::status_led::set(on);
    (void)xTaskDelayUntil(&wake, pdMS_TO_TICKS(500));
  }
}

extern "C" void effect_task(void* arg) {
  (void)arg;
  TickType_t wake = xTaskGetTickCount();
  for (;;) {
    fill(frames.back(), color_for_frame());
    while (strip.busy()) {
      vTaskDelay(pdMS_TO_TICKS(1));
    }
    frames.publish();
    if (xSemaphoreGive(frame_ready) != pdPASS) {
      h743::status_led::fault_on();
    }
    (void)xTaskDelayUntil(&wake, pdMS_TO_TICKS(kFramePeriodMs));
  }
}

extern "C" void output_task(void* arg) {
  (void)arg;
  for (;;) {
    (void)xSemaphoreTake(frame_ready, portMAX_DELAY);
    (void)strip.submit(frames.front());
  }
}

void create_task(TaskFunction_t fn, const char* name, configSTACK_DEPTH_TYPE words, UBaseType_t priority,
                 StackType_t* stack, StaticTask_t* tcb) {
  const TaskHandle_t handle = xTaskCreateStatic(fn, name, words, nullptr, priority, stack, tcb);
  if (handle == nullptr) {
    halt();
  }
}

}  // namespace

[[noreturn]] void run() {
  strip.init();
  frame_ready = xSemaphoreCreateBinaryStatic(&frame_ready_storage);
  if (frame_ready == nullptr) {
    halt();
  }

  // Output is the highest of the three, so it reaches the frame semaphore
  // and blocks before the effect publishes the first frame.
  create_task(output_task, "output", kOutputWords, kOutputPriority, output_stack, &output_tcb);
  create_task(effect_task, "effect", kEffectWords, kEffectPriority, effect_stack, &effect_tcb);
  create_task(status_task, "status", kStatusWords, kStatusPriority, status_stack, &status_tcb);

  vTaskStartScheduler();
  halt();
}

}  // namespace lightworks::app
