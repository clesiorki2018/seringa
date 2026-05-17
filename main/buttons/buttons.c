#include "buttons.h"

#include "motor_config.h"
#include "seringa.h"

#include "driver/gpio.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdbool.h>
#include <stdint.h>

static const char *TAG = "BUTTONS";

#define BUTTONS_TASK_STACK_SIZE         3072
#define BUTTONS_TASK_PRIORITY           5

#define BUTTONS_POLL_INTERVAL_MS        20
#define BUTTONS_DEBOUNCE_MS             80
#define BUTTONS_REARM_MS                300

typedef enum {

    BUTTON_ACTION_INJECT_1ML = 0,
    BUTTON_ACTION_FILL_TOTAL

} button_action_t;

typedef struct {

    gpio_num_t gpio;
    button_action_t action;
    bool stable_pressed;
    bool last_raw_pressed;
    bool fill_running;
    uint32_t debounce_ms;
    uint32_t rearm_ms;

} button_state_t;

static button_state_t g_buttons[] = {

    {
        .gpio = MOTOR_BUTTON_INJECT_1ML_GPIO,
        .action = BUTTON_ACTION_INJECT_1ML,
    },
    {
        .gpio = MOTOR_BUTTON_FILL_GPIO,
        .action = BUTTON_ACTION_FILL_TOTAL,
    },
};

static bool button_is_pressed(
    gpio_num_t gpio
)
{
    return gpio_get_level(gpio) == 0;
}

static void button_handle_press(
    button_state_t *button
)
{
    if (button == NULL) {

        return;
    }

    if (seringa_is_busy()) {

        ESP_LOGW(TAG, "Comando por botao ignorado: motor ocupado");
        return;
    }

    bool ok = false;

    switch (button->action) {

        case BUTTON_ACTION_INJECT_1ML:
            ESP_LOGI(TAG, "Botao: injetar 1 ml");

            ok =
                seringa_injetar_ml(
                    1.0f,
                    SERINGA_FLOW_NORMAL
                );
            break;

        case BUTTON_ACTION_FILL_TOTAL:
            ESP_LOGI(TAG, "Botao pressionado: recarregar ate fim de curso traseiro");

            ok =
                seringa_encher_total();

            button->fill_running =
                ok;
            break;
    }

    if (!ok) {

        ESP_LOGW(TAG, "Comando por botao recusado");
    }
}

static void button_handle_release(
    button_state_t *button
)
{
    if (button == NULL) {

        return;
    }

    if (button->action != BUTTON_ACTION_FILL_TOTAL) {

        return;
    }

    if (!button->fill_running) {

        return;
    }

    button->fill_running =
        false;

    if (!seringa_is_busy()) {

        return;
    }

    ESP_LOGI(TAG, "Botao solto: parando recarga");
    seringa_stop();
}

static void buttons_task(
    void *arg
)
{
    (void)arg;

    const uint32_t poll_ms =
        BUTTONS_POLL_INTERVAL_MS;

    while (true) {

        for (size_t i = 0; i < sizeof(g_buttons) / sizeof(g_buttons[0]); i++) {

            button_state_t *button =
                &g_buttons[i];

            bool raw_pressed =
                button_is_pressed(button->gpio);

            if (raw_pressed != button->last_raw_pressed) {

                button->last_raw_pressed =
                    raw_pressed;

                button->debounce_ms =
                    0;
            } else if (button->debounce_ms < BUTTONS_DEBOUNCE_MS) {

                button->debounce_ms +=
                    poll_ms;
            }

            if (button->rearm_ms > 0) {

                if (button->rearm_ms > poll_ms) {

                    button->rearm_ms -=
                        poll_ms;
                } else {

                    button->rearm_ms =
                        0;
                }
            }

            if (button->debounce_ms < BUTTONS_DEBOUNCE_MS) {

                continue;
            }

            if (raw_pressed &&
                !button->stable_pressed &&
                button->rearm_ms == 0) {

                button->stable_pressed =
                    true;

                button->rearm_ms =
                    BUTTONS_REARM_MS;

                button_handle_press(
                    button
                );
            } else if (!raw_pressed) {

                if (button->stable_pressed) {

                    button_handle_release(
                        button
                    );
                }

                button->stable_pressed =
                    false;
            }
        }

        vTaskDelay(
            pdMS_TO_TICKS(BUTTONS_POLL_INTERVAL_MS)
        );
    }
}

void buttons_init(void)
{
    gpio_config_t input_conf = {

        .pin_bit_mask =
            (1ULL << MOTOR_BUTTON_INJECT_1ML_GPIO) |
            (1ULL << MOTOR_BUTTON_FILL_GPIO),

        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    gpio_config(
        &input_conf
    );

    for (size_t i = 0; i < sizeof(g_buttons) / sizeof(g_buttons[0]); i++) {

        g_buttons[i].last_raw_pressed =
            button_is_pressed(g_buttons[i].gpio);

        g_buttons[i].stable_pressed =
            g_buttons[i].last_raw_pressed;
    }

    xTaskCreate(
        buttons_task,
        "buttons_task",
        BUTTONS_TASK_STACK_SIZE,
        NULL,
        BUTTONS_TASK_PRIORITY,
        NULL
    );

    ESP_LOGI(
        TAG,
        "Botoes inicializados: injetar 1ml GPIO %d, recarregar GPIO %d",
        MOTOR_BUTTON_INJECT_1ML_GPIO,
        MOTOR_BUTTON_FILL_GPIO
    );
}
