/*
 * Copyright 2026 clesiorki
 *
 * Licensed under the Apache License, Version 2.0.
 * See the LICENSE file in the project root for full license information.
 */

#include "storage.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "esp_log.h"

#include <stdint.h>

/*
 * ============================================================================
 * 🧾 TAG
 * ============================================================================
 */
static const char *TAG = "STORAGE";

/*
 * ============================================================================
 * 🔑 NAMESPACE / KEYS
 * ============================================================================
 *
 * Namespace:
 *  - área lógica dentro do NVS
 *
 * Keys:
 *  - nomes dos valores persistidos
 * ============================================================================
 */
#define STORAGE_NAMESPACE              "seringa"
#define STORAGE_SCHEMA_VERSION         1

#define KEY_STEPS_PER_ML               "steps_ml"
#define KEY_MOTOR_INVERTED             "mot_inv"
#define KEY_BACKLASH_STEPS             "backlash"

/*
 * ============================================================================
 * 🚀 INIT
 * ============================================================================
 *
 * O NVS normalmente já é inicializado no main.c.
 *
 * Aqui validamos apenas que o módulo está disponível.
 * ============================================================================
 */
void storage_init(void)
{
    ESP_LOGI(
        TAG,
        "Storage pronto"
    );
}

/*
 * ============================================================================
 * 📌 VERSÃO DO STORAGE
 * ============================================================================
 */
uint32_t storage_get_version(void)
{
    return STORAGE_SCHEMA_VERSION;
}

/*
 * ============================================================================
 * 🧠 ABRE NVS
 * ============================================================================
 *
 * Centraliza abertura para reduzir duplicação.
 * ============================================================================
 */
static bool storage_open(
    nvs_open_mode_t mode,
    nvs_handle_t *handle
)
{
    esp_err_t err =
        nvs_open(
            STORAGE_NAMESPACE,
            mode,
            handle
        );

    if (err != ESP_OK) {

        ESP_LOGE(
            TAG,
            "Falha ao abrir NVS: %s",
            esp_err_to_name(err)
        );

        return false;
    }

    return true;
}

/*
 * ============================================================================
 * 💾 SALVA FLOAT
 * ============================================================================
 *
 * NVS não possui tipo float direto.
 * Por isso usamos blob.
 * ============================================================================
 */
static bool storage_save_float(
    const char *key,
    float value
)
{
    nvs_handle_t handle;

    if (!storage_open(NVS_READWRITE, &handle)) {
        return false;
    }

    esp_err_t err =
        nvs_set_blob(
            handle,
            key,
            &value,
            sizeof(value)
        );

    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }

    nvs_close(handle);

    if (err != ESP_OK) {

        ESP_LOGE(
            TAG,
            "Falha salvando float '%s': %s",
            key,
            esp_err_to_name(err)
        );

        return false;
    }

    ESP_LOGI(
        TAG,
        "Float salvo %s=%.3f",
        key,
        value
    );

    return true;
}

/*
 * ============================================================================
 * 📥 CARREGA FLOAT
 * ============================================================================
 */
static bool storage_load_float(
    const char *key,
    float *value
)
{
    if (value == NULL) {
        return false;
    }

    nvs_handle_t handle;

    if (!storage_open(NVS_READONLY, &handle)) {
        return false;
    }

    size_t size = sizeof(float);

    esp_err_t err =
        nvs_get_blob(
            handle,
            key,
            value,
            &size
        );

    nvs_close(handle);

    if (err != ESP_OK || size != sizeof(float)) {

        ESP_LOGW(
            TAG,
            "Float '%s' não encontrado",
            key
        );

        return false;
    }

    ESP_LOGI(
        TAG,
        "Float carregado %s=%.3f",
        key,
        *value
    );

    return true;
}

/*
 * ============================================================================
 * 💾 SALVA INT32
 * ============================================================================
 */
static bool storage_save_i32(
    const char *key,
    int32_t value
)
{
    nvs_handle_t handle;

    if (!storage_open(NVS_READWRITE, &handle)) {
        return false;
    }

    esp_err_t err =
        nvs_set_i32(
            handle,
            key,
            value
        );

    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }

    nvs_close(handle);

    return err == ESP_OK;
}

/*
 * ============================================================================
 * 📥 CARREGA INT32
 * ============================================================================
 */
static bool storage_load_i32(
    const char *key,
    int32_t *value
)
{
    if (value == NULL) {
        return false;
    }

    nvs_handle_t handle;

    if (!storage_open(NVS_READONLY, &handle)) {
        return false;
    }

    esp_err_t err =
        nvs_get_i32(
            handle,
            key,
            value
        );

    nvs_close(handle);

    return err == ESP_OK;
}

/*
 * ============================================================================
 * 🎯 CALIBRAÇÃO - STEPS POR ML
 * ============================================================================
 */
bool storage_save_steps_per_ml(float value)
{
    return storage_save_float(
        KEY_STEPS_PER_ML,
        value
    );
}

bool storage_load_steps_per_ml(float *value)
{
    return storage_load_float(
        KEY_STEPS_PER_ML,
        value
    );
}

/*
 * ============================================================================
 * 🔄 MOTOR INVERTED
 * ============================================================================
 *
 * bool é salvo como int32 para simplificar.
 * ============================================================================
 */
bool storage_save_motor_inverted(bool inverted)
{
    return storage_save_i32(
        KEY_MOTOR_INVERTED,
        inverted ? 1 : 0
    );
}

bool storage_load_motor_inverted(bool *inverted)
{
    if (inverted == NULL) {
        return false;
    }

    int32_t value = 0;

    if (!storage_load_i32(KEY_MOTOR_INVERTED, &value)) {
        return false;
    }

    *inverted = (value != 0);

    return true;
}

/*
 * ============================================================================
 * ⚙️ BACKLASH STEPS
 * ============================================================================
 */
bool storage_save_backlash_steps(int32_t steps)
{
    return storage_save_i32(
        KEY_BACKLASH_STEPS,
        steps
    );
}

bool storage_load_backlash_steps(int32_t *steps)
{
    return storage_load_i32(
        KEY_BACKLASH_STEPS,
        steps
    );
}

/*
 * ============================================================================
 * 🧪 RESET DE FÁBRICA
 * ============================================================================
 *
 * Apaga namespace inteiro.
 * ============================================================================
 */
bool storage_factory_reset(void)
{
    nvs_handle_t handle;

    if (!storage_open(NVS_READWRITE, &handle)) {
        return false;
    }

    esp_err_t err =
        nvs_erase_all(handle);

    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }

    nvs_close(handle);

    if (err != ESP_OK) {

        ESP_LOGE(
            TAG,
            "Falha reset factory: %s",
            esp_err_to_name(err)
        );

        return false;
    }

    ESP_LOGW(
        TAG,
        "Storage resetado"
    );

    return true;
}