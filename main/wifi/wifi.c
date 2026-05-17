/*
 * Copyright 2026 clesiorki
 *
 * Licensed under the Apache License, Version 2.0.
 * See the LICENSE file in the project root for full license information.
 */

#include "wifi.h"

#include <string.h>
#include <assert.h>

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "lwip/inet.h"

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

/*
 * ============================================================================
 * 🧾 TAG
 * ============================================================================
 */
static const char *TAG = "WIFI";

#ifndef SERINGA_WIFI_SSID
#error "SERINGA_WIFI_SSID deve ser definido no .env ou no ambiente de build"
#endif

#ifndef SERINGA_WIFI_PASS
#error "SERINGA_WIFI_PASS deve ser definido no .env ou no ambiente de build"
#endif

/*
 * ============================================================================
 * 📡 CONTROLE DE CONEXÃO
 * ============================================================================
 */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

#define MAX_RETRY 10

static EventGroupHandle_t wifi_event_group;
static int retry_count = 0;

/*
 * ============================================================================
 * 📡 EVENT HANDLER
 * ============================================================================
 *
 * RESPONSABILIDADE:
 *  - gerenciar reconexão
 *  - sinalizar estado do sistema
 *
 * IMPORTANTE:
 *  - NÃO bloquear
 *  - NÃO fazer lógica pesada
 */
static void wifi_event_handler(void *arg,
                               esp_event_base_t event_base,
                               int32_t event_id,
                               void *event_data)
{
    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "WiFi start → conectando...");
                esp_wifi_connect();
                break;

            case WIFI_EVENT_STA_DISCONNECTED:

                if (retry_count < MAX_RETRY)
                {
                    esp_wifi_connect();
                    retry_count++;

                    ESP_LOGW(TAG, "Reconectando (%d/%d)...",
                             retry_count, MAX_RETRY);
                }
                else
                {
                    ESP_LOGE(TAG, "Falha ao conectar WiFi");

                    xEventGroupSetBits(wifi_event_group, WIFI_FAIL_BIT);
                }

                xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
                break;

            default:
                break;
        }
    }
    else if (event_base == IP_EVENT &&
             event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

        ESP_LOGI(TAG, "IP obtido: " IPSTR,
                 IP2STR(&event->ip_info.ip));

        retry_count = 0;

        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/*
 * ============================================================================
 * 🌐 CONFIGURAÇÃO DE REDE
 * ============================================================================
 *
 * OBS:
 *  - DHCP por padrão (mais seguro)
 *  - IP fixo pode ser habilitado se necessário
 */
static void configure_network(esp_netif_t *netif)
{
#if 0
    /*
     * 🔒 IP FIXO (DESABILITADO POR PADRÃO)
     */
    ESP_ERROR_CHECK(esp_netif_dhcpc_stop(netif));

    esp_netif_ip_info_t ip_info = {
        .ip.addr      = inet_addr("192.168.0.200"),
        .gw.addr      = inet_addr("192.168.0.1"),
        .netmask.addr = inet_addr("255.255.255.0"),
    };

    ESP_ERROR_CHECK(esp_netif_set_ip_info(netif, &ip_info));
#endif
}

/*
 * ============================================================================
 * 🚀 INIT WIFI
 * ============================================================================
 *
 * BLOQUEANTE CONTROLADO:
 *  - aguarda conexão OU falha
 */
void wifi_init(void)
{
    /*
     * 🔹 Event Group
     */
    wifi_event_group = xEventGroupCreate();

    /*
     * 🔹 Stack de rede
     */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /*
     * 🔹 Interface STA
     */
    esp_netif_t *netif = esp_netif_create_default_wifi_sta();
    assert(netif != NULL);

    configure_network(netif);

    /*
     * 🔹 WiFi driver
     */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /*
     * 🔹 Eventos
     */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &wifi_event_handler,
        NULL,
        NULL));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &wifi_event_handler,
        NULL,
        NULL));

    /*
     * 🔹 Config STA
     */
    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    strcpy((char *)wifi_config.sta.ssid, SERINGA_WIFI_SSID);
    strcpy((char *)wifi_config.sta.password, SERINGA_WIFI_PASS);

    /*
     * 🔹 Modo
     */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    /*
     * 🔹 Start
     */
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi inicializado");

    /*
     * ⏳ Aguarda conexão OU falha
     */
    EventBits_t bits = xEventGroupWaitBits(
        wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE,
        pdFALSE,
        pdMS_TO_TICKS(10000) // timeout de segurança
    );

    /*
     * 📊 Resultado
     */
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "WiFi conectado");
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGE(TAG, "Falha ao conectar WiFi");
    }
    else
    {
        ESP_LOGW(TAG, "Timeout WiFi (seguindo sem conexão)");
    }
}
