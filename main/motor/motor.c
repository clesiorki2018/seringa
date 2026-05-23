/*
 * Copyright 2026 clesiorki
 *
 * Licensed under the Apache License, Version 2.0.
 * See the LICENSE file in the project root for full license information.
 */

#include "motor.h"

#include "motor_config.h"
#include "motor_hw.h"
#include "motor_motion.h"

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include <stdbool.h>
#include <stdint.h>

static const char *TAG = "MOTOR";

#ifndef MOTOR_QUEUE_SIZE
#define MOTOR_QUEUE_SIZE 1
#endif

#ifndef MOTOR_TASK_STACK_SIZE
#define MOTOR_TASK_STACK_SIZE 4096
#endif

#ifndef MOTOR_TASK_PRIORITY
#define MOTOR_TASK_PRIORITY 5
#endif

typedef struct {
    motor_motion_profile_t profile;
} motor_queue_msg_t;

static QueueHandle_t g_motor_queue = NULL;
static volatile bool g_motor_busy = false;
static volatile bool g_motor_running = false;
static volatile bool g_stop_requested = false;

static bool motor_effective_direction(motor_direction_t direction)
{
    bool dir = (direction == MOTOR_DIRECTION_FORWARD);

#if MOTOR_DIRECTION_INVERTED
    return !dir;
#else
    return dir;
#endif
}

static bool motor_direction_blocked_by_endstop(
    bool effective_direction
)
{
    /*
     * A camada de movimento usa a direção efetiva já corrigida pela
     * inversão mecânica global:
     *
     *  true  -> avança para o endstop frontal / vazio
     *  false -> retrai para o endstop traseiro / cheio
     */
    if (effective_direction) {

        return motor_hw_front_endstop_triggered();
    }

    return motor_hw_back_endstop_triggered();
}

static void motor_task(void *arg)
{
    motor_queue_msg_t msg;

    while (true) {
        if (xQueueReceive(g_motor_queue, &msg, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        if (g_motor_running) {
            ESP_LOGW(TAG, "Motor ocupado");
            xQueueReset(g_motor_queue);
            continue;
        }

        g_motor_running = true;
        g_stop_requested = false;

        ESP_LOGI(
            TAG,
            "Movimento iniciado (%lu steps)",
            (unsigned long) msg.profile.steps
        );

        motor_motion_execute(
            &msg.profile,
            &g_stop_requested
        );

        motor_hw_coils_off();

        g_motor_running = false;
        g_motor_busy = false;

        ESP_LOGI(TAG, "Movimento finalizado");
    }
}

void motor_init(void)
{
    motor_hw_init();

    g_motor_queue = xQueueCreate(
        MOTOR_QUEUE_SIZE,
        sizeof(motor_queue_msg_t)
    );

    if (g_motor_queue == NULL) {
        ESP_LOGE(TAG, "Falha ao criar fila do motor");
        return;
    }

    BaseType_t result = xTaskCreate(
        motor_task,
        "motor_task",
        MOTOR_TASK_STACK_SIZE,
        NULL,
        MOTOR_TASK_PRIORITY,
        NULL
    );

    if (result != pdPASS) {
        ESP_LOGE(TAG, "Falha ao criar task do motor");
        vQueueDelete(g_motor_queue);
        g_motor_queue = NULL;
        return;
    }

    ESP_LOGI(TAG, "Motor inicializado");
}

bool motor_move(const motor_move_t *move)
{
    if (move == NULL) {
        ESP_LOGW(TAG, "Movimento NULL");
        return false;
    }

    if (move->steps == 0) {
        ESP_LOGW(TAG, "Steps inválidos");
        return false;
    }

#ifdef MOTOR_MAX_ALLOWED_STEPS
    if (move->steps > MOTOR_MAX_ALLOWED_STEPS) {
        ESP_LOGE(TAG, "Steps acima do limite");
        return false;
    }
#endif

    if (g_motor_queue == NULL) {
        ESP_LOGE(TAG, "Fila do motor não inicializada");
        return false;
    }

    if (g_motor_busy || g_motor_running) {
        ESP_LOGW(TAG, "Motor ocupado");
        return false;
    }

    bool effective_direction =
        motor_effective_direction(move->direction);

    if (motor_direction_blocked_by_endstop(effective_direction)) {
        ESP_LOGW(TAG, "Movimento bloqueado por endstop acionado");
        return false;
    }

    motor_queue_msg_t msg = {
        .profile = {
            .direction = effective_direction,
            .steps = move->steps,
            .use_ramp = move->use_ramp,
            .anti_stiction = move->anti_stiction_enable
        }
    };

    g_motor_busy = true;

    if (xQueueSend(g_motor_queue, &msg, 0) != pdTRUE) {
        ESP_LOGW(TAG, "Fila cheia");
        g_motor_busy = false;
        return false;
    }

    return true;
}

void motor_stop(void)
{
    g_stop_requested = true;

    if (g_motor_queue != NULL) {
        xQueueReset(g_motor_queue);
    }

    if (!g_motor_running) {
        g_motor_busy = false;
    }
}

bool motor_is_running(void)
{
    return g_motor_busy || g_motor_running;
}

motor_state_t motor_get_state(void)
{
    return motor_is_running()
        ? MOTOR_STATE_RUNNING
        : MOTOR_STATE_IDLE;
}

bool motor_front_endstop_triggered(void)
{
    return motor_hw_front_endstop_triggered();
}

bool motor_back_endstop_triggered(void)
{
    return motor_hw_back_endstop_triggered();
}

int motor_front_endstop_level(void)
{
    return motor_hw_front_endstop_level();
}

int motor_back_endstop_level(void)
{
    return motor_hw_back_endstop_level();
}

int motor_front_endstop_gpio(void)
{
    return MOTOR_ENDSTOP_FRONT_GPIO;
}

int motor_back_endstop_gpio(void)
{
    return MOTOR_ENDSTOP_BACK_GPIO;
}

bool motor_endstops_installed(void)
{
#if MOTOR_ENDSTOPS_INSTALLED
    return true;
#else
    return false;
#endif
}
