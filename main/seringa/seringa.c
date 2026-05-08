#include "seringa.h"
#include "motor.h"
#include "calibration.h"

#include "driver/gpio.h"
#include "esp_log.h"

/*
 * ============================================================================
 * 🧾 TAG
 * ============================================================================
 */
static const char *TAG = "SERINGA";

/*
 * ============================================================================
 * 🔌 GPIOs (FIM DE CURSO)
 * ============================================================================
 *
 * Ativo em LOW (pull-up interno)
 */
#define PIN_FIM_RETRAIDO 17  // CHEIA
#define PIN_FIM_AVANCADO 16  // VAZIA

/*
 * ============================================================================
 * 🧠 ESTADO INTERNO
 * ============================================================================
 */
static seringa_status_t current_status = SERINGA_IDLE;

/*
 * ============================================================================
 * 📥 LEITURA DOS SENSORES
 * ============================================================================
 */
static inline bool is_retraido(void)
{
    return gpio_get_level(PIN_FIM_RETRAIDO) == 0;
}

static inline bool is_avancado(void)
{
    return gpio_get_level(PIN_FIM_AVANCADO) == 0;
}

/*
 * ============================================================================
 * 🧠 ATUALIZA STATUS (FONTE DA VERDADE)
 * ============================================================================
 *
 * Prioridade:
 * 1. Movimento
 * 2. Fim de curso
 * 3. Idle
 */
static void update_status(void)
{
    if (motor_is_running()) {
        current_status = SERINGA_MOVING;
        return;
    }

    if (is_retraido()) {
        current_status = SERINGA_CHEIA;
    } else if (is_avancado()) {
        current_status = SERINGA_VAZIA;
    } else {
        current_status = SERINGA_IDLE;
    }
}

/*
 * ============================================================================
 * 🚀 INIT
 * ============================================================================
 */
void seringa_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_FIM_RETRAIDO) |
                        (1ULL << PIN_FIM_AVANCADO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE
    };

    gpio_config(&io_conf);

    update_status();

    ESP_LOGI(TAG, "Seringa inicializada");
}

/*
 * ============================================================================
 * ➕ INJETAR (EM ML)
 * ============================================================================
 *
 * Camada de domínio:
 *  - recebe ml
 *  - converte para steps
 *  - envia para motor
 */
void seringa_injetar_ml(float ml)
{
    update_status();

    if (motor_is_running()) {
        ESP_LOGW(TAG, "Ignorado: motor ocupado");
        return;
    }

    if (is_avancado()) {
        ESP_LOGW(TAG, "Bloqueado: seringa VAZIA");
        return;
    }

    int steps = calibration_ml_to_steps(ml);

    ESP_LOGI(TAG, "Injetando %.2f ml (%d steps)", ml, steps);

    motor_send_command(MOTOR_CMD_FORWARD, steps);
}

/*
 * ============================================================================
 * ➖ RECARREGAR COMPLETAMENTE
 * ============================================================================
 *
 * Vai até fim de curso traseiro (cheio)
 */
void seringa_recarregar_total(void)
{
    update_status();

    if (motor_is_running()) {
        ESP_LOGW(TAG, "Ignorado: motor ocupado");
        return;
    }

    if (is_retraido()) {
        ESP_LOGW(TAG, "Já está CHEIA");
        return;
    }

    /*
     * 🔥 Estratégia:
     * manda passos grandes → motor para no endstop
     */
    int steps = 30000;

    ESP_LOGI(TAG, "Recarregando totalmente");

    motor_send_command(MOTOR_CMD_BACKWARD, steps);
}

/*
 * ============================================================================
 * ➕ INJETAR (LEGADO - STEPS)
 * ============================================================================
 */
void seringa_injetar_steps(int steps)
{
    ESP_LOGW(TAG, "Uso de API antiga (steps)");

    motor_send_command(MOTOR_CMD_FORWARD, steps);
}

/*
 * ============================================================================
 * ➖ RECARREGAR (LEGADO - STEPS)
 * ============================================================================
 */
void seringa_recarregar_steps(int steps)
{
    ESP_LOGW(TAG, "Uso de API antiga (steps)");

    motor_send_command(MOTOR_CMD_BACKWARD, steps);
}

/*
 * ============================================================================
 * 🛑 STOP
 * ============================================================================
 */
void seringa_stop(void)
{
    ESP_LOGW(TAG, "STOP acionado");

    motor_stop();
    update_status();
}

/*
 * ============================================================================
 * 📊 STATUS
 * ============================================================================
 */
seringa_status_t seringa_get_status(void)
{
    update_status();
    return current_status;
}

/*
 * ============================================================================
 * 🔎 HELPERS
 * ============================================================================
 */
bool seringa_is_busy(void)
{
    return motor_is_running();
}

bool seringa_is_cheia(void)
{
    return is_retraido();
}

bool seringa_is_vazia(void)
{
    return is_avancado();
}