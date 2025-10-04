# Camera Task

## 📋 Descripción

Módulo para streaming de video MJPEG en tiempo real usando la ESP32-CAM. Implementa un servidor HTTP que sirve frames de la cámara como stream multipart para visualización en navegadores web o aplicaciones cliente.

## 🔧 Interfaz

### Configuración de hardware

```c
// Pines ESP32-CAM AI-Thinker
#define Y2_GPIO_NUM     5       // Datos Y2
#define Y3_GPIO_NUM     18      // Datos Y3
#define Y4_GPIO_NUM     19      // Datos Y4
#define Y5_GPIO_NUM     21      // Datos Y5
#define Y6_GPIO_NUM     36      // Datos Y6
#define Y7_GPIO_NUM     39      // Datos Y7
#define Y8_GPIO_NUM     34      // Datos Y8
#define Y9_GPIO_NUM     35      // Datos Y9
#define XCLK_GPIO_NUM   0       // Clock maestro
#define PCLK_GPIO_NUM   22      // Pixel clock
#define VSYNC_GPIO_NUM  25      // Sincronización vertical
#define HREF_GPIO_NUM   23      // Referencia horizontal
#define SIOD_GPIO_NUM   26      // I2C SDA (configuración)
#define SIOC_GPIO_NUM   27      // I2C SCL (configuración)
#define PWDN_GPIO_NUM   32      // Power down
```

### Funciones principales

```c
// Inicializar cámara con configuración JPEG
esp_err_t start_camera();

// Iniciar servidor HTTP para streaming
httpd_handle_t server_init();

// Handler interno para servir frames
esp_err_t uri_get_handler(httpd_req_t *req);
```

## 🚀 Funcionamiento

### 1. Configuración de cámara

- **Formato:** JPEG comprimido
- **Resolución:** QVGA (320x240 px)
- **Frame rate:** 20 FPS
- **Calidad JPEG:** 10 (alta compresión)
- **Buffers:** 2 frame buffers (doble buffering)

### 2. Servidor HTTP

- **Endpoint:** `GET /stream`
- **Content-Type:** `multipart/x-mixed-replace`
- **Protocolo:** HTTP chunked transfer
- **Prioridad:** Baja (task_priority = 1)

### 3. Stream MJPEG

Formato de respuesta HTTP:
```
--frame
Content-Type: image/jpeg
Content-Length: [tamaño]

[datos JPEG]
--frame
Content-Type: image/jpeg
Content-Length: [tamaño]

[datos JPEG]
...
```

## ⚡ Rendimiento ESP32-CAM (Estimaciones teóricas)

> ⚠️ **NOTA:** Los valores siguientes son cálculos teóricos. Este módulo **NO ha sido testeado** en hardware real.

| Métrica | Valor estimado |
|---------|---------------|
| **Frame rate** | 20 FPS |
| **Resolución** | 320x240 (76,800 px) |
| **Tamaño JPEG** | ~8-15 KB/frame |
| **Ancho de banda** | ~160-300 KB/s |
| **Latencia por frame** | ~50-80 ms |
| **Memoria RAM** | ~30-50 KB (buffers) |

### Cálculos de rendimiento

#### **Procesamiento por frame:**
```
- Captura sensor:        ~10-15 ms
- Compresión JPEG:       ~20-30 ms
- Transmisión HTTP:      ~15-25 ms
- Delay programado:      50 ms
─────────────────────────────────
Total por frame:         ~95-120 ms
Frame rate real:         ~8-10 FPS*
```
*\*Menor que los 20 FPS configurados debido a overhead*

#### **Uso de CPU:**

```
- Captura/compresión:    ~40-60% @ 240MHz
- Servidor HTTP:         ~15-25%
- Task switching:        ~5-10%
─────────────────────────────────
CPU total estimado:      ~60-95%
```

#### **Ancho de banda WiFi:**

```
- Frame promedio:        12 KB
- @ 20 FPS teórico:      240 KB/s
- @ 10 FPS real:         120 KB/s
- Overhead HTTP:         +20-30%
─────────────────────────────────
Ancho de banda:          ~150-170 KB/s
```

### Factores limitantes

- ❌ **CPU intensivo:** Compresión JPEG en tiempo real
- ❌ **Memoria:** Buffers de frames + stack HTTP
- ❌ **WiFi:** Latencia y ancho de banda variable
- ❌ **Calidad vs velocidad:** Trade-off jpeg_quality vs frame_rate

## Testing

> ⚠️ **IMPORTANTE:** Este módulo **NO ha sido testeado** en hardware real.

### Tests requeridos

- [ ] **Inicialización de cámara** - Verificar configuración GPIO
- [ ] **Captura de frames** - Validar formato JPEG
- [ ] **Servidor HTTP** - Probar endpoint `/stream`
- [ ] **Performance real** - Medir FPS y latencia reales
- [ ] **Estabilidad** - Stream continuo por horas
- [ ] **Manejo de errores** - Desconexiones, fallos de cámara

### Ejemplo de uso teórico

```c
#include "cam_task.h"

void app_main() {
    // Inicializar WiFi (no incluido)
    wifi_init();
    
    // Inicializar cámara
    if (start_camera() != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed");
        return;
    }
    
    // Iniciar servidor
    httpd_handle_t server = server_init();
    if (!server) {
        ESP_LOGE(TAG, "Server init failed");
        return;
    }
    
    ESP_LOGI(TAG, "Stream available at: http://[IP]/stream");
}
```

## 📊 Configuraciones optimizables

### Para mayor FPS
```c
.frame_size = FRAMESIZE_QQVGA,    // 160x120
.jpeg_quality = 15,               // Menor calidad
.fb_count = 3,                    // Más buffers
vTaskDelay(pdMS_TO_TICKS(25));    // 40 FPS teórico
```

### Para mayor calidad
```c
.frame_size = FRAMESIZE_VGA,      // 640x480
.jpeg_quality = 5,                // Mayor calidad
.fb_count = 1,                    // Menos memoria
vTaskDelay(pdMS_TO_TICKS(100));   // 10 FPS
```

### Para menor consumo
```c
.xclk_freq_hz = 10000000,         // 10 MHz clock
.frame_size = FRAMESIZE_QQVGA,    // Resolución mínima
server_config.task_priority = 0;   // Prioridad muy baja
```

## 🎯 Casos de uso

1. **Telemetría visual:** Stream en tiempo real para control remoto
2. **Vigilancia:** Monitoreo continuo del entorno
3. **Debug visual:** Verificar movimientos y posición del rover
4. **Navegación:** Input visual para algoritmos de pathfinding

## ⚠️ Limitaciones conocidas

- **Sin autenticación:** Stream HTTP público
- **Una conexión:** Un solo cliente simultaneo
- **Sin grabación:** No guarda frames en memoria/SD
- **Calidad fija:** Sin control dinámico de calidad
- **Sin zoom/pan:** Sin control de cámara avanzado

---
*Desarrollado para Rover Team UNLP - ESP32-CAM Firmware*  
⚠️ **Módulo no testeado - Requiere validación en hardware**
