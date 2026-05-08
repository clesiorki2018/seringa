#include "storage.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "esp_log.h"

/*
 * ============================================================================
 * 🧾 TAG
 * ============================================================================
 */
static const char *TAG = "STORAGE";

/*
 * ============================================================================
 * 🔑 CONFIG
 * ============================================================================
 */
#define NAMESPACE "seringa"
#define KEY_CALIB "calib"

/*
 * ============================================================================
 * 🚀 INIT
 * ============================================================================
 *
 * OBS:
 * NVS já é inicializado no main, então aqui só validamos acesso
 */
void storage_init(void)
{
    ESP_LOGI(TAG, "Storage pronto");
}

/*
 * ============================================================================
 * 💾 SALVAR CALIBRAÇÃO
 * ============================================================================
 */
bool storage_save_calibration(float value)
{
    nvs_handle_t handle;

    esp_err_t err = nvs_open(NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro abrindo NVS");
        return false;
    }

    err = nvs_set_blob(handle, KEY_CALIB, &value, sizeof(value));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro salvando calibração");
        nvs_close(handle);
        return false;
    }

    nvs_commit(handle);
    nvs_close(handle);

    ESP_LOGI(TAG, "Calibração salva: %.3f", value);

    return true;
}

/*
 * ============================================================================
 * 📥 CARREGAR CALIBRAÇÃO
 * ============================================================================
 */
bool storage_load_calibration(float *value)
{
    nvs_handle_t handle;

    esp_err_t err = nvs_open(NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS não inicializada (usando default)");
        return false;
    }

    size_t size = sizeof(float);

    err = nvs_get_blob(handle, KEY_CALIB, value, &size);

    nvs_close(handle);

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Calibração não encontrada (default)");
        return false;
    }

    ESP_LOGI(TAG, "Calibração carregada: %.3f", *value);

    return true;
}