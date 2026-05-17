/*
 * Copyright 2026 clesiorki
 *
 * Licensed under the Apache License, Version 2.0.
 * See the LICENSE file in the project root for full license information.
 */

#include "routes_static.h"

#include "esp_log.h"

#include <stdio.h>

/*
 * ============================================================================
 * 🧾 TAG
 * ============================================================================
 */
static const char *TAG = "ROUTES_STATIC";

/*
 * ============================================================================
 * 📂 SERVE FILE
 * ============================================================================
 *
 * Responsável por:
 *
 *  - abrir arquivo SPIFFS
 *  - stream em chunks
 *  - evitar uso excessivo de RAM
 *
 * IMPORTANTE:
 *  - funciona bem para ESP32
 *  - não carrega arquivo inteiro na memória
 * ============================================================================
 */
static esp_err_t serve_file(
    httpd_req_t *req,
    const char *filepath,
    const char *content_type
)
{
    /*
     * ================================================================
     * 📂 ABRE ARQUIVO
     * ================================================================
     */
    FILE *file =
        fopen(filepath, "r");

    if (!file) {

        ESP_LOGE(
            TAG,
            "Arquivo não encontrado: %s",
            filepath
        );

        return httpd_resp_send_err(
            req,
            HTTPD_404_NOT_FOUND,
            "Arquivo não encontrado"
        );
    }

    /*
     * ================================================================
     * 📦 CONTENT TYPE
     * ================================================================
     */
    httpd_resp_set_type(
        req,
        content_type
    );

    /*
     * ================================================================
     * 🚀 STREAM CHUNK
     * ================================================================
     */
    char buffer[512];

    size_t read_bytes;

    while (
        (read_bytes = fread(
            buffer,
            1,
            sizeof(buffer),
            file
        )) > 0
    ) {
        if (
            httpd_resp_send_chunk(
                req,
                buffer,
                read_bytes
            ) != ESP_OK
        ) {
            fclose(file);

            ESP_LOGE(
                TAG,
                "Erro stream arquivo"
            );

            return ESP_FAIL;
        }
    }

    fclose(file);

    /*
     * ================================================================
     * 🧹 FINALIZA CHUNKED RESPONSE
     * ================================================================
     */
    httpd_resp_send_chunk(
        req,
        NULL,
        0
    );

    return ESP_OK;
}

/*
 * ============================================================================
 * 🌐 HTML INDEX
 * ============================================================================
 */
static esp_err_t index_handler(
    httpd_req_t *req
)
{
    return serve_file(
        req,
        "/spiffs/index.html",
        "text/html"
    );
}

/*
 * ============================================================================
 * 🌐 HTML CALIBRAÇÃO
 * ============================================================================
 */
static esp_err_t calibragem_handler(
    httpd_req_t *req
)
{
    return serve_file(
        req,
        "/spiffs/calibragem.html",
        "text/html"
    );
}

/*
 * ============================================================================
 * 📜 JAVASCRIPT
 * ============================================================================
 */
static esp_err_t js_handler(
    httpd_req_t *req
)
{
    return serve_file(
        req,
        "/spiffs/app.js",
        "application/javascript"
    );
}

/*
 * ============================================================================
 * 🎨 CSS
 * ============================================================================
 */
static esp_err_t css_handler(
    httpd_req_t *req
)
{
    return serve_file(
        req,
        "/spiffs/style.css",
        "text/css"
    );
}

/*
 * ============================================================================
 * 🧠 REGISTRO DE ROTAS
 * ============================================================================
 */
esp_err_t register_static_routes(
    httpd_handle_t server
)
{
    httpd_uri_t routes[] = {

        /*
         * ============================================================
         * 🌐 INDEX
         * ============================================================
         */
        {
            "/",
            HTTP_GET,
            index_handler,
            NULL
        },

        /*
         * ============================================================
         * 🌐 CALIBRAÇÃO
         * ============================================================
         */
        {
            "/calibragem",
            HTTP_GET,
            calibragem_handler,
            NULL
        },

        /*
         * ============================================================
         * 📜 JS
         * ============================================================
         */
        {
            "/app.js",
            HTTP_GET,
            js_handler,
            NULL
        },

        /*
         * ============================================================
         * 🎨 CSS
         * ============================================================
         */
        {
            "/style.css",
            HTTP_GET,
            css_handler,
            NULL
        }
    };

    /*
     * ================================================================
     * 🌐 REGISTRA TODAS
     * ================================================================
     */
    for (
        int i = 0;
        i < sizeof(routes) / sizeof(routes[0]);
        i++
    ) {
        httpd_register_uri_handler(
            server,
            &routes[i]
        );
    }

    ESP_LOGI(
        TAG,
        "Rotas estáticas registradas"
    );

    return ESP_OK;
}