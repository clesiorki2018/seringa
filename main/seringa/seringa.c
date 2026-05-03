#include "seringa.h"
#include "motor.h"

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
 */
#define PIN_FIM_RETRAIDO 17  // CHEIA
#define PIN_FIM_AVANCADO 16  // VAZIA

/*
 * ============================================================================
 * 🧠 ESTADO
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
 * 🧠 ATUALIZA STATUS (BASEADO NA REALIDADE)
 * ============================================================================
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
 * 🔧 INIT
 * ============================================================================
 */
void seringa_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_FIM_RETRAIDO) |
                        (1ULL << PIN_FIM_AVANCADO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE
    };

    gpio_config(&io_conf);

    update_status();

    ESP_LOGI(TAG, "Seringa inicializada (modo async)");
}

/*
 * ============================================================================
 * ➕ INJETAR
 * ============================================================================
 */
void seringa_injetar_steps(int steps)
{
    update_status();

    if (motor_is_running()) {
        ESP_LOGW(TAG, "Ignorado: motor ocupado");
        return;
    }

    if (is_avancado()) {
        ESP_LOGW(TAG, "Bloqueado: já está VAZIA (fim de curso frente)");
        return;
    }

    ESP_LOGI(TAG, "Injetar %d steps", steps);

    motor_send_command(MOTOR_CMD_FORWARD, steps);
}

/*
 * ============================================================================
 * ➖ RECARREGAR
 * ============================================================================
 */
void seringa_recarregar_steps(int steps)
{
    update_status();

    if (motor_is_running()) {
        ESP_LOGW(TAG, "Ignorado: motor ocupado");
        return;
    }

    if (is_retraido()) {
        ESP_LOGW(TAG, "Bloqueado: já está CHEIA (fim de curso trás)");
        return;
    }

    ESP_LOGI(TAG, "Recarregar %d steps", steps);

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