/*
 * Copyright 2026 clesiorki
 *
 * Licensed under the Apache License, Version 2.0.
 * See the LICENSE file in the project root for full license information.
 */

#ifndef ROUTES_API_H
#define ROUTES_API_H

#include "esp_http_server.h"

/*
 * ============================================================================
 * 🌐 API REST
 * ============================================================================
 *
 * Responsável por:
 *
 *  - status da seringa
 *  - comandos de movimento
 *  - controle remoto
 *  - endpoints REST
 *
 * IMPORTANTE:
 *  - não deve conter lógica de hardware
 *  - apenas integração HTTP <-> domínio
 * ============================================================================
 */

/*
 * ============================================================================
 * 🧠 REGISTRO DE ROTAS API
 * ============================================================================
 */
esp_err_t register_api_routes(
    httpd_handle_t server
);

#endif