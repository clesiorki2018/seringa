#ifndef AUTH_H
#define AUTH_H

#include <stdbool.h>

#include "esp_http_server.h"

/*
 * ============================================================================
 * 🔐 AUTENTICAÇÃO WEB
 * ============================================================================
 *
 * Responsabilidades:
 *
 *  - login via PIN
 *  - geração de token
 *  - controle de sessão
 *  - timeout automático
 *  - autorização de rotas
 *
 * IMPORTANTE:
 *  - módulo isolado da lógica da seringa
 *  - reutilizável para futuras APIs
 * ============================================================================
 */

/*
 * ============================================================================
 * 🔐 HANDLER LOGIN
 * ============================================================================
 *
 * POST /api/login
 */
esp_err_t auth_login_handler(httpd_req_t *req);

/*
 * ============================================================================
 * 🔐 VALIDA AUTORIZAÇÃO
 * ============================================================================
 *
 * Verifica:
 *  - header Authorization
 *  - token válido
 *  - sessão expirada
 */
bool auth_is_authorized(httpd_req_t *req);

#endif