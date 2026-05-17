/*
 * Copyright 2026 clesiorki
 *
 * Licensed under the Apache License, Version 2.0.
 * See the LICENSE file in the project root for full license information.
 */

#include "calibration.h"
#include "storage/storage.h"

#include "esp_log.h"

/*
 * ============================================================================
 * 🧾 TAG
 * ============================================================================
 */
static const char *TAG = "CALIBRATION";

/*
 * ============================================================================
 * 📊 LIMITES DE SEGURANÇA
 * ============================================================================
 *
 * Protege contra:
 *  - erro humano
 *  - valores absurdos
 *  - risco mecânico
 */
#define CALIB_MIN   100.0f
#define CALIB_MAX   50000.0f
#define CALIB_STEP  50.0f

/*
 * ============================================================================
 * 🧠 ESTADO INTERNO
 * ============================================================================
 *
 * IMPORTANTE:
 *  - Nunca acessar diretamente fora deste módulo
 *  - Sempre via API
 */
static float calibration_factor = CALIBRATION_DEFAULT;

/*
 * ============================================================================
 * 🔒 CLAMP (SANITIZAÇÃO)
 * ============================================================================
 *
 * Garante invariantes do sistema
 */
static float clamp(float v)
{
    if (v < CALIB_MIN) return CALIB_MIN;
    if (v > CALIB_MAX) return CALIB_MAX;
    return v;
}

/*
 * ============================================================================
 * 🚀 INIT
 * ============================================================================
 *
 * Fluxo:
 *  - tenta carregar do storage
 *  - fallback para default
 */
void calibration_init(void)
{
    float stored;

    if (storage_load_steps_per_ml(&stored)) {
        calibration_factor = clamp(stored);
        ESP_LOGI(TAG, "Calibração carregada: %.2f steps/ml", calibration_factor);
    } else {
        calibration_factor = CALIBRATION_DEFAULT;
        ESP_LOGW(TAG, "Usando default: %.2f", calibration_factor);
    }
}

/*
 * ============================================================================
 * 📥 GET
 * ============================================================================
 */
float calibration_get(void)
{
    return calibration_factor;
}

/*
 * ============================================================================
 * 📤 SET
 * ============================================================================
 *
 * Regra:
 *  - valida
 *  - aplica
 *  - persiste
 */
void calibration_set(float value)
{
    calibration_factor = clamp(value);

    if (!storage_save_steps_per_ml(calibration_factor)) {
        ESP_LOGE(TAG, "Falha ao persistir calibração");
    }

    ESP_LOGI(TAG, "Nova calibração: %.2f steps/ml", calibration_factor);
}

/*
 * ============================================================================
 * ➕ INCREMENTO
 * ============================================================================
 */
void calibration_increase(void)
{
    calibration_set(calibration_factor + CALIB_STEP);
}

/*
 * ============================================================================
 * ➖ DECREMENTO
 * ============================================================================
 */
void calibration_decrease(void)
{
    calibration_set(calibration_factor - CALIB_STEP);
}

/*
 * ============================================================================
 * 🔄 CONVERSÃO
 * ============================================================================
 *
 * ml → steps
 *
 * OBS:
 *  - função pura (sem efeitos colaterais)
 *  - fácil de testar
 */
int calibration_ml_to_steps(float ml)
{
    if (ml <= 0.0f)
        return 0;

    return (int)(ml * calibration_factor);
}