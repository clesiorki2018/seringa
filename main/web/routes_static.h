/*
 * Copyright 2026 clesiorki
 *
 * Licensed under the Apache License, Version 2.0.
 * See the LICENSE file in the project root for full license information.
 */

#ifndef ROUTES_STATIC_H
#define ROUTES_STATIC_H

#include "esp_http_server.h"

/*
 * ============================================================================
 * 🌐 ROTAS ESTÁTICAS
 * ============================================================================
 *
 * Responsável por:
 *
 *  - páginas HTML
 *  - CSS
 *  - JavaScript
 *  - arquivos estáticos SPIFFS
 *
 * IMPORTANTE:
 *  - não contém lógica REST
 *  - não contém autenticação
 *  - apenas entrega arquivos
 * ============================================================================
 */

/*
 * ============================================================================
 * 🧠 REGISTRO DE ROTAS ESTÁTICAS
 * ============================================================================
 */
esp_err_t register_static_routes(
    httpd_handle_t server
);

#endif