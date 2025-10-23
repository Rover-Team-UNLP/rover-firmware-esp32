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
#define UART_NUM UART_NUM_1
#define UART_PARITY UART_PARITY_DISABLE
#define UART_STOP UART_STOP_BITS_1
#define UART_FLOW UART_HW_FLOWCTRL_DISABLE
#define UART_TX GPIO_NUM_1
#define  UART_RX GPIO_NUM_3
#define RX_SIZE 128
#define TX_SIZE 128
#define QUEUE_SIZE 10


/**
 * @brief This function initialize the UART to a series of parameters define in the macros.
 */
void init_uart(void);

void task_uart(void);

#endif