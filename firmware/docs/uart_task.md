# UART_task

## Calculo de tamaño de buffers

Para determinar el tamaño de los buffers Tx y Rx debemos determinar que datos van a manejar cada cual.

### Buffer Rx

En este caso Rx va manejar como máximo strings de la forma S:Response:IDE donde S = 1 byte, E = 1 byte, Response = 2 bytes, ID = 4 bytes

$$

$$
