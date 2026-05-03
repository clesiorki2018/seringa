#include "wifi.h"

#include <string.h>
#include <assert.h>

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "lwip/inet.h"

// 🔐 Credenciais (protótipo — depois migrar para NVS ou interface web)
#define WIFI_SSID "xxx"
#define WIFI_PASS "tictac@max.20.."

// 📌 IP fixo (útil para acesso direto ao ESP32)
#define STATIC_IP   "192.168.0.200"
#define GATEWAY_IP  "192.168.0.1"
#define NETMASK_IP  "255.255.255.0"

static const char *TAG = "WIFI";

// 🔔 Event Group para sinalizar conexão
static EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

// ==========================================================
// 📡 Handler de eventos WiFi/IP
// Responsável por:
// - reconectar automaticamente
// - detectar quando IP está pronto
// ==========================================================
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
                ESP_LOGI(TAG, "WiFi iniciado, conectando...");
                esp_wifi_connect();
                break;

            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGW(TAG, "WiFi desconectado, tentando reconectar...");
                esp_wifi_connect();
                xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
                break;

            default:
                break;
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

        ESP_LOGI(TAG, "IP obtido: " IPSTR, IP2STR(&event->ip_info.ip));

        // 🔓 Libera execução (WiFi pronto)
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// ==========================================================
// 📶 Inicialização completa do WiFi
// ==========================================================
void wifi_init(void)
{
    // 🔹 Cria grupo de eventos (sincronização)
    wifi_event_group = xEventGroupCreate();

    // 🔹 Inicializa pilha de rede
    ESP_ERROR_CHECK(esp_netif_init());

    // 🔹 Cria loop de eventos padrão
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 🔹 Cria interface STA (cliente)
    esp_netif_t *netif = esp_netif_create_default_wifi_sta();
    assert(netif != NULL);

    // 🔹 Inicializa driver WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // 🔹 Registra handlers de eventos
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

    // 🔒 Desativa DHCP para usar IP fixo
    ESP_ERROR_CHECK(esp_netif_dhcpc_stop(netif));

    // 📌 Configura IP fixo
    esp_netif_ip_info_t ip_info;

    ip_info.ip.addr = inet_addr(STATIC_IP);
    ip_info.gw.addr = inet_addr(GATEWAY_IP);
    ip_info.netmask.addr = inet_addr(NETMASK_IP);

    ESP_ERROR_CHECK(esp_netif_set_ip_info(netif, &ip_info));

    // 📡 Configuração WiFi
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,

            // 🔐 Segurança mínima
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            // 🔄 Gerenciamento de proteção de quadros
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };

    // 🔹 Define modo STA
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    // 🔹 Aplica configuração
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    // 🔹 Inicia WiFi
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi inicializado, aguardando conexão...");

    // ⏳ Aguarda conexão real (substitui vTaskDelay do main)
    xEventGroupWaitBits(
        wifi_event_group,
        WIFI_CONNECTED_BIT,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY
    );

    ESP_LOGI(TAG, "WiFi conectado com IP fixo: %s", STATIC_IP);
}