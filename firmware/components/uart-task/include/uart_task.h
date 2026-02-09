/* ======================================
- File: uart_task.h
- Description: Header of the uart task.
- Author/s: @JuanCruzFerreiraM
- Last-update: 2025-10-20
- ====================================== */

#ifndef UART_TASK
#define UART_TASK

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "stdint.h"
#include "string.h"
#include "stdio.h"
#include "communication.h"
#include "app_globals.h"

#define UART_BAUD_RATE 115200
#define UART_NUM UART_NUM_2
#define UART_PARITY UART_PARITY_DISABLE
#define UART_STOP UART_STOP_BITS_1
#define UART_FLOW UART_HW_FLOWCTRL_DISABLE
/* ESP32-CAM: GPIO 12 = UART2 TX, GPIO 13 = UART2 RX.
 * WARNING: GPIO 12 is a strapping pin (must be LOW at boot for 3.3V flash).
 * GPIO 12/13 are also used for SD card on some boards; do not use SD if using UART2 here. */
#define UART_TX GPIO_NUM_12
#define UART_RX GPIO_NUM_13
#define RX_SIZE 256
#define TX_SIZE 256
#define QUEUE_SIZE 10

/**
 * @brief This function initialize the UART to a series of parameters define in the macros.
 */
void init_uart(void);

void task_uart(void *);

#endif