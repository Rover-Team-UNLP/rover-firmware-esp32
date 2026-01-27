#ifndef WEB_SOCKET
#define WEB_SOCKET

#include "app_globals.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "json_parser.h"
#include <stdint.h> 


void web_socket_task(void *);


#endif