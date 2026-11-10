#include "status_led.hpp"

#include "FreeRTOS.h"
#include "task.h"

namespace {

StaticTask_t idle_tcb;
StackType_t idle_stack[configMINIMAL_STACK_SIZE];

}  // namespace

extern "C" void vApplicationGetIdleTaskMemory(StaticTask_t** tcb, StackType_t** stack,
                                              configSTACK_DEPTH_TYPE* words) {
  *tcb = &idle_tcb;
  *stack = idle_stack;
  *words = configMINIMAL_STACK_SIZE;
}

extern "C" void vApplicationStackOverflowHook(TaskHandle_t task, char* name) {
  (void)task;
  (void)name;
  lightworks::h743::status_led::fault_on();
  for (;;) {
  }
}
