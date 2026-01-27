#ifndef DNS_SERVER_H
#define DNS_SERVER_H

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/sys.h"



/**
 * @brief Inicia el servidor DNS "Hijacker" para el Portal Cautivo.
 * * Esta función crea una tarea de FreeRTOS dedicada y abre un socket UDP
 * en el puerto 53.
 * * Su lógica es responder a TODAS las consultas DNS de tipo A (IPv4) con
 * la dirección IP del SoftAP del ESP32 (por defecto 192.168.4.1).
 * Esto fuerza a los dispositivos móviles a abrir el navegador automáticamente.
 * * @return
 * - ESP_OK: Si el socket se creó y la tarea se lanzó correctamente.
 * - ESP_FAIL: Si hubo un error de memoria o de red.
 */
esp_err_t dns_server_start(void);

/**
 * @brief Detiene el servidor DNS y libera recursos.
 * * Cierra el socket UDP y elimina la tarea de FreeRTOS asociada.
 * Es CRÍTICO llamar a esta función una vez que se obtienen las credenciales
 * WiFi para liberar memoria RAM (Stack) que será necesaria para el
 * procesamiento de video de la ESP32-CAM.
 */
void dns_server_stop(void);

#endif // DNS_SERVER_H