#include "motor.h"

#include "motor_hw.h"
#include "motor_motion.h"

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include <stdbool.h>
#include <stdint.h>

/*
 * ============================================================================
 * 🧾 TAG DE LOG
 * ============================================================================
 */
static const char *TAG = "MOTOR";

/*
 * ============================================================================
 * 📬 FILA DE MOVIMENTOS
 * ============================================================================
 *
 * Esta fila recebe comandos assíncronos vindos de:
 *  - seringa
 *  - calibração
 *  - web
 *
 * IMPORTANTE:
 *  - motor.c NÃO sabe o que é seringa
 *  - apenas executa movimentos
 * ============================================================================
 */
typedef struct {

    motor_motion_profile_t profile;

} motor_queue_msg_t;

/*
 * ============================================================================
 * 🧠 ESTADO GLOBAL
 * ============================================================================
 */
static QueueHandle_t g_motor_queue = NULL;

static volatile bool g_motor_running = false;

/*
 * Flag de STOP imediato.
 *
 * IMPORTANTE:
 *  - acessada em tempo real pela motion layer
 */
static volatile bool g_stop_requested = false;

/*
 * ============================================================================
 * ⚙️ TASK PRINCIPAL
 * ============================================================================
 *
 * Responsabilidades:
 *  - consumir fila
 *  - executar movimentos
 *  - controlar estados
 *
 * NÃO executa:
 *  - GPIO
 *  - stepping
 *  - rampas
 * ============================================================================
 */
static void motor_task(void *arg)
{
    motor_queue_msg_t msg;

    while (true) {

        /*
         * ================================================================
         * 📬 AGUARDA COMANDO
         * ================================================================
         */
        if (xQueueReceive(
                g_motor_queue,
                &msg,
                portMAX_DELAY) != pdTRUE) {

            continue;
        }

        /*
         * ================================================================
         * 🚫 SEGURANÇA
         * ================================================================
         */
        if (g_motor_running) {

            ESP_LOGW(
                TAG,
                "Motor ocupado, ignorando comando"
            );

            continue;
        }

        /*
         * ================================================================
         * 🚀 INICIA MOVIMENTO
         * ================================================================
         */
        g_motor_running = true;

        g_stop_requested = false;

        ESP_LOGI(
            TAG,
            "Movimento iniciado (%lu steps)",
            (unsigned long)msg.profile.steps
        );

        /*
         * ================================================================
         * ⚙️ EXECUTA MOVIMENTO
         * ================================================================
         */
        motor_motion_execute(
            &msg.profile,
            &g_stop_requested
        );

        /*
         * ================================================================
         * 🧹 FINALIZA
         * ================================================================
         */
        g_motor_running = false;

        ESP_LOGI(TAG, "Movimento finalizado");
    }
}

/*
 * ============================================================================
 * 🚀 INICIALIZAÇÃO
 * ============================================================================
 */
void motor_init(void)
{
    /*
     * ================================================================
     * 🔧 HARDWARE
     * ================================================================
     */
    motor_hw_init();

    /*
     * ================================================================
     * 📬 FILA
     * ================================================================
     */
    g_motor_queue = xQueueCreate(
        5,
        sizeof(motor_queue_msg_t)
    );

    /*
     * ================================================================
     * ⚠️ VALIDAÇÃO
     * ================================================================
     */
    if (g_motor_queue == NULL) {

        ESP_LOGE(
            TAG,
            "Falha ao criar fila do motor"
        );

        return;
    }

    /*
     * ================================================================
     * ⚙️ TASK
     * ================================================================
     */
    xTaskCreate(
        motor_task,
        "motor_task",
        4096,
        NULL,
        5,
        NULL
    );

    ESP_LOGI(TAG, "Motor inicializado");
}

/*
 * ============================================================================
 * 📡 API PÚBLICA
 * ============================================================================
 */

bool motor_move(const motor_move_t *move)
{
    /*
     * ================================================================
     * ⚠️ VALIDAÇÃO
     * ================================================================
     */
    if (move == NULL) {
        return false;
    }

    if (move->steps == 0) {
        return false;
    }

    /*
     * ================================================================
     * 🚫 EVITA FILA SUJA
     * ================================================================
     *
     * Estratégia:
     *  - rejeita novos comandos enquanto ocupado
     *  - evita backlog perigoso
     * ================================================================
     */
    if (g_motor_running) {

        ESP_LOGW(
            TAG,
            "Motor ocupado"
        );

        return false;
    }

    /*
     * ================================================================
     * 🧠 CONVERTE API -> PROFILE
     * ================================================================
     */
    motor_queue_msg_t msg = {

        .profile = {

            .direction =
                (move->direction ==
                 MOTOR_DIRECTION_FORWARD),

            .steps = move->steps,

            .use_ramp = move->use_ramp,

            .anti_stiction =
                move->anti_stiction_enable
        }
    };

    /*
     * ================================================================
     * 📬 ENVIA FILA
     * ================================================================
     */
    if (xQueueSend(
            g_motor_queue,
            &msg,
            0) != pdTRUE) {

        ESP_LOGW(
            TAG,
            "Fila cheia"
        );

        return false;
    }

    return true;
}

/*
 * ============================================================================
 * 🛑 STOP IMEDIATO
 * ============================================================================
 */
void motor_stop(void)
{
    /*
     * Flag monitorada em tempo real
     * pela motion layer.
     */
    g_stop_requested = true;
}

/*
 * ============================================================================
 * 📊 ESTADO
 * ============================================================================
 */
bool motor_is_running(void)
{
    return g_motor_running;
}

motor_state_t motor_get_state(void)
{
    if (g_motor_running) {
        return MOTOR_STATE_RUNNING;
    }

    return MOTOR_STATE_IDLE;
}

/*
 * ============================================================================
 * 🔴 ENDSTOPS
 * ============================================================================
 */

bool motor_front_endstop_triggered(void)
{
    return motor_hw_front_endstop_triggered();
}

bool motor_back_endstop_triggered(void)
{
    return motor_hw_back_endstop_triggered();
}