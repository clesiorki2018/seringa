#ifndef ROUTES_H
#define ROUTES_H

#include "esp_http_server.h"

/*
 * ============================================================================
 * 🌐 REGISTRO GLOBAL DE ROTAS
 * ============================================================================
 */
esp_err_t register_routes(
    httpd_handle_t server
);

#endif