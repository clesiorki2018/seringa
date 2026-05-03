#include "motor.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/*
 * ============================================================================
 * 🔧 CONFIGURAÇÃO DE HARDWARE
 * ============================================================================
 *
 * ULN2003 + 28BYJ-48 (modo half-step)
 */
#define IN1_GPIO 18
#define IN2_GPIO 19
#define IN3_GPIO 21
#define IN4_GPIO 22

/*
 * 🔴 FIM DE CURSO (ativo em LOW)
 */
#define ENDSTOP_BACK   17   // totalmente retraído
#define ENDSTOP_FRONT  16   // totalmente avançado

/*
 * ⏱️ TIMING DO MOTOR
 *
 * Ajusta velocidade:
 *  menor valor → mais rápido (cuidado com torque)
 */
#define STEP_DELAY_US 1200

/*
 * Debounce simples por repetição (não bloqueante)
 */
#define ENDSTOP_CONFIRM_COUNT 3

static const char *TAG = "MOTOR";

/*
 * ============================================================================
 * 🧠 ESTADO INTERNO
 * ============================================================================
 */
static volatile bool running = false;
static int step_index = 0;

/*
 * ============================================================================
 * 📬 FILA DE COMANDOS
 * ============================================================================
 */
typedef struct {
    motor_cmd_t cmd;
    int steps;
} motor_msg_t;

static QueueHandle_t motor_queue;

/*
 * ============================================================================
 * 🔄 SEQUÊNCIA HALF-STEP
 * ============================================================================
 */
static const uint8_t seq[8][4] = {
    {1,0,0,0},{1,1,0,0},{0,1,0,0},{0,1,1,0},
    {0,0,1,0},{0,0,1,1},{0,0,0,1},{1,0,0,1}
};

/*
 * ============================================================================
 * 🔌 CONTROLE DAS BOBINAS
 * ============================================================================
 */
static inline void apply_step(int s)
{
    gpio_set_level(IN1_GPIO, seq[s][0]);
    gpio_set_level(IN2_GPIO, seq[s][1]);
    gpio_set_level(IN3_GPIO, seq[s][2]);
    gpio_set_level(IN4_GPIO, seq[s][3]);
}

static void coils_off(void)
{
    gpio_set_level(IN1_GPIO, 0);
    gpio_set_level(IN2_GPIO, 0);
    gpio_set_level(IN3_GPIO, 0);
    gpio_set_level(IN4_GPIO, 0);
}

/*
 * ============================================================================
 * 🧠 LEITURA DE FIM DE CURSO (NÃO BLOQUEANTE)
 * ============================================================================
 *
 * Estratégia:
 *  - lê várias vezes seguidas
 *  - evita delay (não trava o motor)
 *  - muito mais rápido que vTaskDelay
 */
static bool endstop_confirmed(gpio_num_t pin)
{
    int count = 0;

    for (int i = 0; i < ENDSTOP_CONFIRM_COUNT; i++) {
        if (gpio_get_level(pin) == 0)
            count++;
    }

    return count == ENDSTOP_CONFIRM_COUNT;
}

static inline bool endstop_front_triggered(void)
{
    return endstop_confirmed(ENDSTOP_FRONT);
}

static inline bool endstop_back_triggered(void)
{
    return endstop_confirmed(ENDSTOP_BACK);
}

/*
 * ============================================================================
 * ⚙️ TASK PRINCIPAL DO MOTOR
 * ============================================================================
 *
 * Responsabilidades:
 *  - consumir fila de comandos
 *  - executar movimento
 *  - respeitar STOP em tempo real
 *  - respeitar fim de curso
 *
 * IMPORTANTE:
 *  - nunca bloquear desnecessariamente
 *  - manter loop rápido
 */
static void motor_task(void *arg)
{
    motor_msg_t msg;

    while (1) {

        /*
         * 🔄 Aguarda comando
         */
        if (xQueueReceive(motor_queue, &msg, portMAX_DELAY)) {

            /*
             * 🛑 STOP imediato
             */
            if (msg.cmd == MOTOR_CMD_STOP) {
                ESP_LOGW(TAG, "STOP recebido");
                coils_off();
                running = false;
                continue;
            }

            /*
             * 🚫 Evita concorrência
             */
            if (running) {
                ESP_LOGW(TAG, "Motor ocupado, ignorando comando");
                continue;
            }

            int dir = (msg.cmd == MOTOR_CMD_FORWARD);

            /*
             * 🔒 Segurança antes de iniciar
             */
            if (dir && endstop_front_triggered()) {
                ESP_LOGW(TAG, "Já no limite FRENTE");
                continue;
            }

            if (!dir && endstop_back_triggered()) {
                ESP_LOGW(TAG, "Já no limite TRÁS");
                continue;
            }

            running = true;

            ESP_LOGI(TAG, "Movimento iniciado (%d steps)", msg.steps);

            for (int i = 0; i < msg.steps; i++) {

                /*
                 * 🛑 STOP em tempo real (peek não bloqueante)
                 */
                motor_msg_t peek;
                if (xQueuePeek(motor_queue, &peek, 0) == pdTRUE) {
                    if (peek.cmd == MOTOR_CMD_STOP) {
                        xQueueReceive(motor_queue, &peek, 0);
                        ESP_LOGW(TAG, "STOP durante execução");
                        break;
                    }
                }

                /*
                 * 🔴 FIM DE CURSO (proteção dura)
                 */
                if (dir && endstop_front_triggered()) {
                    ESP_LOGW(TAG, "Fim de curso FRENTE atingido");
                    break;
                }

                if (!dir && endstop_back_triggered()) {
                    ESP_LOGW(TAG, "Fim de curso TRÁS atingido");
                    break;
                }

                /*
                 * 🔄 Executa passo
                 */
                apply_step(step_index);

                step_index = dir
                    ? (step_index + 1) % 8
                    : (step_index - 1 + 8) % 8;

                /*
                 * ⏱️ Delay crítico (precisão do motor)
                 *
                 * ⚠️ NÃO usar vTaskDelay aqui:
                 *  - granularidade ruim (ms)
                 *  - quebra movimento
                 */
                esp_rom_delay_us(STEP_DELAY_US);
            }

            /*
             * 🔌 Segurança: desliga bobinas
             */
            coils_off();

            running = false;

            ESP_LOGI(TAG, "Movimento finalizado");
        }
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
     * 🔌 Saídas do motor
     */
    gpio_config_t out_conf = {
        .pin_bit_mask =
            (1ULL << IN1_GPIO) |
            (1ULL << IN2_GPIO) |
            (1ULL << IN3_GPIO) |
            (1ULL << IN4_GPIO),
        .mode = GPIO_MODE_OUTPUT
    };

    gpio_config(&out_conf);

    /*
     * 🔴 Entradas (fim de curso)
     */
    gpio_config_t in_conf = {
        .pin_bit_mask =
            (1ULL << ENDSTOP_BACK) |
            (1ULL << ENDSTOP_FRONT),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE
    };

    gpio_config(&in_conf);

    coils_off();

    /*
     * 📬 Cria fila
     */
    motor_queue = xQueueCreate(5, sizeof(motor_msg_t));

    /*
     * ⚙️ Cria task
     */
    xTaskCreate(
        motor_task,
        "motor_task",
        4096,
        NULL,
        5,
        NULL
    );

    ESP_LOGI(TAG, "Motor inicializado (robusto + não bloqueante)");
}

/*
 * ============================================================================
 * 📡 API
 * ============================================================================
 */
void motor_send_command(motor_cmd_t cmd, int steps)
{
    motor_msg_t msg = {
        .cmd = cmd,
        .steps = steps
    };

    if (xQueueSend(motor_queue, &msg, 0) != pdTRUE) {
        ESP_LOGW(TAG, "Fila cheia, comando descartado");
    }
}

void motor_stop(void)
{
    motor_msg_t msg = {
        .cmd = MOTOR_CMD_STOP,
        .steps = 0
    };

    xQueueSend(motor_queue, &msg, 0);
}

bool motor_is_running(void)
{
    return running;
}