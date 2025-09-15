# Style Guide

La finalidad de este archivo es definir la reglas para el desarrollo del proyecto, manteniendo coherencia entre las diferentes soluciones de cada desarrollador.

## Organización de código

### Carpetas

Los nombres de las carpetas deben ir en minúsculas y en caso de ser compuestos deben de usarse el - como separador de palabras. Por ejemplo: `firmware/` o `web-app/`.

### Archivos

El nombre del archivo debe reflejar el contenido del mismo.

- *C*: `rover_speed.c`
- *JavaScript*: `camera-viewer.js`
- *Python*:  `rover_controller.py`

Para el caso de archivos tipo drivers tanto .h como .c deben especificarse `placa_periférico_driver`. Por ejemplo: `esp32_uart_driver.c`

Cada archivo debe estar en su carpeta correspondiente. Un ejemplo que mantiene aproximadamente la forma actual es:

```plaintext
rover-proyecto/
│
├─ README.md
├─ .gitignore
├─ docs/                  # Documentación, diagramas, guías, style guide
│   ├─ style-guide.md
│   ├─ architecture.md
│   └─ api-reference.md
│
├─ firmware/              # Código embebido
│   ├─ drivers/           # Drivers por placa/periférico (UART, PWM, etc.)
│   │   ├─ ciaa_uart_driver.c
│   │   ├─ ciaa_pwm_driver.c
│   │   └─ ...
│   ├─ rover/             # Lógica del rover (control, navegación, sensores)
│   ├─ brazo/             # (Opcional) Código para brazo robótico si se agrega
│   └─ common/            # Librerías/utilidades compartidas
│
├─ ia/                    # Inteligencia Artificial
│   ├─ nlp/               # Procesamiento de lenguaje natural (comandos → JSON)
│   ├─ models/            # Modelos entrenados
│   └─ utils/             # Pre/post procesamiento, helpers
│
├─ web/                   # Aplicación web
│   ├─ frontend/          # React/Vue u otro framework de UI
│   ├─ backend/           # API, endpoints, WebSocket
│   ├─ scripts/           # Automatización (ej: build, deploy)
│   └─ public/            # Archivos estáticos (imágenes, íconos, etc.)
│
├─ sim/                   # Simulación (ROS, Gazebo, etc.)
│   ├─ worlds/
│   ├─ models/
│   └─ launch/
│
├─ tests/                 # Pruebas unitarias/integración
│   ├─ firmware/
│   ├─ ia/
│   └─ web/
│
└─ scripts/               # Scripts generales (builds, uploads, CI/CD, utils)

```

## Reglas de nombres

Los nombres deben ser en ingles siempre que sea posible.

### Variables

- Python y C: usar `snake_case`
- JavaScript: usar `camelCase`

### Funciones

Misma convención que para las variables.

### Clases

Tanto en python como en js usar `PascalCase`

### Constantes/defines/Macros

Utilizar `UPPER_CASE`

## Documentación de código

Toda la documentación mencionada debe estar en ingles.

Todos los archivos deben comenzar con un header de la forma

```C
/* ======================================

- File: motor_control.c
- Description: Control de motores del rover
- Author/s: [Nombre/s]
- Last-update: YYYY-MM-DD
- ====================================== */
```

Adaptado al comentario del lenguaje que se este usando.

Cada función debe tener un comentario que explique propósito, parámetros y retorno:

- **C**:

```C
/**
 * @brief Mueve el rover hacia adelante
 * @param distance Distancia en cm
 * @return 0 si OK, -1 si error
 */
int rover_move_forward(int distance);
```

- **Python**:

```python
def process_command(cmd: str) -> dict:
    """
    Procesa un comando de control del rover.
    
    Args:
        cmd (str): Instrucción en lenguaje natural.
    Returns:
        dict: JSON con parámetros de movimiento.
    """
```

- **JS**:

```javascript
/**
 * Envía datos de la cámara al servidor.
 * @param {ImageData} frame - Frame de la cámara
 * @returns {Promise<Response>}
 */
async function sendCameraFrame(frame) { ... }
```

## Reglas de trabajo en GIT

- Las ramas main y develop solo reciben cambios por Pull Request.
- Toda PR debe estar aprobada por 2 miembros.
- Los nombres de las otras ramas y su forma de trabajo quedan a decisión de cada desarrollador. Se puede usar una rama por cada desarrollador o seguir un escama de trabajo estilo `tipo_de_rama\nombre` por ejemplo `feature\nombre_del_feature`. Es indiferente.

Se recomienda siempre que se vaya a trabajar por las dudas hacer un fetch y pull a develop.

### Commits

Los commits deben ser claros, concisos y en ingles. Con la forma

```php-template
<type> : <descripción corta>
```

- Ejemplos:

  - `feat: add motor driver for CIAA`

  - `fix: correct UART baudrate config`

  - `docs: update README with installation steps`

  - `test: add unit tests for rover controller`

- Tipos

  - `feat`: nueva funcionalidad

  - `fix`: corrección de bug
  
  - `docs`: documentación
  
  - `style`: cambios de formato (no funcionales)
  
  - `refactor`: reestructuración de código (sin cambiar funcionalidad)
  
  - `test`: agregar o modificar tests
  
  - `chore`: mantenimiento, dependencias, configs

Pueden existir otros.

En caso del que el commit sea muy grande se utiliza la forma

```csharp
feat: add rover forward movement command

- implemented rover_move_forward() in control.c
- updated navigation.h with new prototypes
- added basic test in tests/rover_control.c

```

### Pull Request

Las PR deben tener titulo y descripción. Se debe explicar una lista de cambios principales. Si se definieron test para funcionalidades de dicha PR, los test deben pasar antes de que se haga el merge.

Estructura típica

Titulo:

```vbnet
feat: implement basic camera streaming from ESP32 to backend
```

Cuerpo:

```markdown
### Changes
- Added camera module in firmware/esp32/
- Implemented UART communication with CIAA
- Updated backend API for image reception

### Notes
Pending: integrate with web frontend

```

### Changelog.md

Este lo dejo como opcional, la idea es un archivo que registra los cambios para las diferentes versiones del proyecto. Por ejemplo:

```markdown
# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

### Added
- Rover basic movement commands (`forward`, `backward`, `turn`).
- ESP32 camera streaming to backend.
- Initial web interface with joystick control.

### Fixed
- UART baudrate mismatch between CIAA and ESP32.

---

## [0.1.0] - 2025-09-03

### Added
- Project setup with repo structure.
- Basic motor driver (CIAA).
- UART driver implementation.
```
