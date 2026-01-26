#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

// --- Definiciones de Bits de Estado ---
// Estos bits son la interfaz de comunicación asíncrona con el resto del sistema.

/**
 * @brief Bit 0: Conexión Exitosa.
 * Se establece cuando el ESP32 ha obtenido una IP del router en modo Estación.
 * Es la señal verde para iniciar tareas de alto consumo (Cámara, MQTT, Bridge).
 */
#define WIFI_CONNECTED_BIT BIT0

/**
 * @brief Bit 1: Fallo de Conexión / Modo AP activo.
 * Se establece si fallaron los reintentos de conexión o no había credenciales.
 * Indica que el sistema está esperando configuración del usuario.
 */
#define WIFI_FAIL_BIT BIT1

// --- Variables Globales Expuestas ---

/**
 * @brief Handle del grupo de eventos.
 * Declarado 'extern' para que main.c pueda hacer xEventGroupWaitBits() sobre él.
 * Se define (asigna memoria) en wifi_manager.c.
 */
extern EventGroupHandle_t wifi_event_group;

// --- API Pública ---

/**
 * @brief Inicia el orquestador de conectividad.
 * * Esta función inicializa la pila TCP/IP (Netif), el bucle de eventos (Event Loop)
 * y la lógica de decisión de arranque:
 * 1. Verifica si existen credenciales en NVS.
 * 2. Si existen: Inicia en modo STA e intenta conectar.
 * 3. Si no existen o falla la conexión: Inicia en modo AP + DNS Server + HTTP Portal.
 * * La función es no-bloqueante (retorna inmediatamente), el resultado se
 * comunicará asíncronamente a través del wifi_event_group.
 */
void wifi_provisioning_init(void);

#endif // WIFI_MANAGER_H