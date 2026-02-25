/* ======================================
 * File: wifi_manager.h
 * Description: WiFi provisioning (STA/AP) and event bits
 * Author/s: @JuanCruzFerreiraM
 * Last-update: 2026-02-19
 * ====================================== */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

/** Bit 0: WiFi connected (STA mode, got IP). Signal to start camera/WebSocket. */
#define WIFI_CONNECTED_BIT BIT0

/** Bit 1: Connection failed or no credentials; AP mode active (provisioning). */
#define WIFI_FAIL_BIT BIT1

extern EventGroupHandle_t wifi_event_group;

/**
 * @brief Start WiFi provisioning (Netif, event loop, STA or AP + DNS + HTTP portal).
 * Non-blocking; result is signaled via wifi_event_group.
 */
void wifi_provisioning_init(void);

#endif