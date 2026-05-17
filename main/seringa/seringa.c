/*
 * Copyright 2026 clesiorki
 *
 * Licensed under the Apache License, Version 2.0.
 * See the LICENSE file in the project root for full license information.
 */

#include "seringa.h"

#include "seringa_motion.h"

#include "motor.h"

#include "esp_log.h"

#include <stdbool.h>

/*
 * ============================================================================
 * 🧾 TAG DE LOG
 * ============================================================================
 *
 * Utilizada pelo ESP-IDF:
 *
 *  ESP_LOGI()
 *  ESP_LOGW()
 *  ESP_LOGE()
 *
 * Permite:
 *  - filtragem
 *  - debug
 *  - rastreamento
 * ============================================================================
 */
static const char *TAG = "SERINGA";

/*
 * ============================================================================
 * 🧠 ESTADO INTERNO GLOBAL
 * ============================================================================
 *
 * Representa o estado lógico atual da seringa.
 *
 * IMPORTANTE:
 *  - NÃO representa posição absoluta
 *  - representa estado operacional
 *
 * Estados possíveis:
 *
 *  SERINGA_IDLE
 *      -> parada intermediária
 *
 *  SERINGA_MOVING
 *      -> movimento em andamento
 *
 *  SERINGA_CHEIA
 *      -> endstop traseiro acionado
 *
 *  SERINGA_VAZIA
 *      -> endstop frontal acionado
 *
 * ============================================================================
 */
static seringa_status_t g_current_status =
    SERINGA_IDLE;

/*
 * ============================================================================
 * 🧠 ATUALIZA ESTADO LÓGICO
 * ============================================================================
 *
 * Esta função é a:
 *
 *      "FONTE ÚNICA DA VERDADE"
 *
 * sobre o estado da seringa.
 *
 * ============================================================================
 * 📌 ORDEM DE PRIORIDADE
 * ============================================================================
 *
 * 1. MOVIMENTO
 *      -> prioridade máxima
 *
 * 2. ENDSTOP TRASEIRO
 *      -> seringa cheia
 *
 * 3. ENDSTOP FRONTAL
 *      -> seringa vazia
 *
 * 4. IDLE
 *      -> posição intermediária
 *
 * ============================================================================
 * ⚠️ IMPORTANTE
 * ============================================================================
 *
 * Centralizar a lógica evita:
 *
 *  - duplicação
 *  - inconsistência
 *  - estados inválidos
 *  - bugs difíceis de rastrear
 *
 * ============================================================================
 */
static void seringa_update_status(void)
{
    /*
     * ================================================================
     * 🚀 MOVIMENTO TEM PRIORIDADE
     * ================================================================
     *
     * Mesmo que o endstop esteja acionado,
     * durante movimento o estado deve ser MOVING.
     */
    if (motor_is_running()) {

        g_current_status =
            SERINGA_MOVING;

        return;
    }

    /*
     * ================================================================
     * 🔴 ENDSTOP TRASEIRO
     * ================================================================
     *
     * Êmbolo totalmente retraído.
     *
     * Estado:
     *  -> CHEIA
     */
    if (motor_back_endstop_triggered()) {

        g_current_status =
            SERINGA_CHEIA;

        return;
    }

    /*
     * ================================================================
     * 🔴 ENDSTOP FRONTAL
     * ================================================================
     *
     * Êmbolo totalmente avançado.
     *
     * Estado:
     *  -> VAZIA
     */
    if (motor_front_endstop_triggered()) {

        g_current_status =
            SERINGA_VAZIA;

        return;
    }

    /*
     * ================================================================
     * 😴 POSIÇÃO INTERMEDIÁRIA
     * ================================================================
     *
     * Não está:
     *  - movimentando
     *  - cheia
     *  - vazia
     *
     * Logo:
     *  -> parada intermediária
     */
    g_current_status =
        SERINGA_IDLE;
}

/*
 * ============================================================================
 * 🚀 INICIALIZAÇÃO
 * ============================================================================
 *
 * Responsabilidades:
 *
 *  - sincronizar estado inicial
 *  - validar sensores
 *  - preparar domínio lógico
 *
 * IMPORTANTE:
 *  - NÃO inicializa hardware
 *  - hardware pertence ao motor.c
 *
 * ============================================================================
 */
void seringa_init(void)
{
    /*
     * ================================================================
     * 🔄 SINCRONIZA ESTADO
     * ================================================================
     */
    seringa_update_status();

    /*
     * ================================================================
     * 🧾 LOG
     * ================================================================
     */
    ESP_LOGI(
        TAG,
        "Seringa inicializada"
    );
}

/*
 * ============================================================================
 * 💉 INJETAR ML
 * ============================================================================
 *
 * Camada de domínio.
 *
 * Responsabilidades:
 *
 *  - validar estado
 *  - proteger operação
 *  - chamar motion layer
 *
 * NÃO executa:
 *  - stepping
 *  - GPIO
 *  - rampas
 *
 * ============================================================================
 */
bool seringa_injetar_ml(
    float ml,
    seringa_flow_profile_t profile
)
{
    /*
     * ================================================================
     * 🔄 REFRESH ESTADO
     * ================================================================
     */
    seringa_update_status();

    /*
     * ================================================================
     * 🚫 MOTOR OCUPADO
     * ================================================================
     *
     * Evita:
     *  - concorrência
     *  - sobreposição
     *  - fila perigosa
     */
    if (motor_is_running()) {

        ESP_LOGW(
            TAG,
            "Motor ocupado"
        );

        return false;
    }

    /*
     * ================================================================
     * 🔴 SERINGA VAZIA
     * ================================================================
     *
     * Não pode avançar mais.
     */
    if (motor_front_endstop_triggered()) {

        ESP_LOGW(
            TAG,
            "Seringa vazia"
        );

        return false;
    }

    /*
     * ================================================================
     * 🚀 EXECUTA MOVIMENTO
     * ================================================================
     */
    bool success =
        seringa_motion_injetar(
            ml,
            profile
        );

    /*
     * ================================================================
     * 🔄 REFRESH PÓS-EXECUÇÃO
     * ================================================================
     */
    seringa_update_status();

    return success;
}

/*
 * ============================================================================
 * ♻️ RECARREGAR ML
 * ============================================================================
 *
 * Move êmbolo para trás.
 *
 * Objetivo:
 *  - puxar líquido
 *  - recarregar seringa
 *
 * ============================================================================
 */
bool seringa_recarregar_ml(
    float ml,
    seringa_flow_profile_t profile
)
{
    /*
     * ================================================================
     * 🔄 REFRESH ESTADO
     * ================================================================
     */
    seringa_update_status();

    /*
     * ================================================================
     * 🚫 MOTOR OCUPADO
     * ================================================================
     */
    if (motor_is_running()) {

        ESP_LOGW(
            TAG,
            "Motor ocupado"
        );

        return false;
    }

    /*
     * ================================================================
     * 🔴 JÁ CHEIA
     * ================================================================
     *
     * Endstop traseiro acionado.
     */
    if (motor_back_endstop_triggered()) {

        ESP_LOGW(
            TAG,
            "Seringa já cheia"
        );

        return false;
    }

    /*
     * ================================================================
     * 🚀 EXECUTA
     * ================================================================
     */
    bool success =
        seringa_motion_recarregar(
            ml,
            profile
        );

    /*
     * ================================================================
     * 🔄 REFRESH
     * ================================================================
     */
    seringa_update_status();

    return success;
}

/*
 * ============================================================================
 * 🧪 ENCHIMENTO TOTAL
 * ============================================================================
 *
 * Homing hidráulico.
 *
 * Estratégia:
 *
 *  - retrai até endstop traseiro
 *  - usado em:
 *
 *      -> inicialização
 *      -> calibração
 *      -> recuperação
 *      -> sincronização
 *
 * ============================================================================
 */
bool seringa_encher_total(void)
{
    /*
     * ================================================================
     * 🔄 REFRESH
     * ================================================================
     */
    seringa_update_status();

    /*
     * ================================================================
     * 🚫 MOTOR OCUPADO
     * ================================================================
     */
    if (motor_is_running()) {

        ESP_LOGW(
            TAG,
            "Motor ocupado"
        );

        return false;
    }

    /*
     * ================================================================
     * 🔴 ENDSTOP TRASEIRO OBRIGATÓRIO
     * ================================================================
     *
     * O enchimento total é um homing hidráulico: ele envia margem de
     * passos e espera que o fim de curso traseiro interrompa o movimento.
     *
     * Sem endstop instalado:
     *  - não existe referência física confiável de parada
     *  - o comando pode comprimir a mecânica contra o limite
     *  - STOP manual vira a única proteção, o que não é aceitável aqui
     */
    if (!motor_endstops_installed()) {

        ESP_LOGW(
            TAG,
            "Enchimento total bloqueado: endstops ausentes"
        );

        return false;
    }

    /*
     * ================================================================
     * 🔴 JÁ CHEIA
     * ================================================================
     */
    if (motor_back_endstop_triggered()) {

        ESP_LOGW(
            TAG,
            "Seringa já cheia"
        );

        return false;
    }

    /*
     * ================================================================
     * 🚀 EXECUTA HOMING
     * ================================================================
     */
    bool success =
        seringa_motion_encher_total();

    /*
     * ================================================================
     * 🔄 REFRESH
     * ================================================================
     */
    seringa_update_status();

    return success;
}

/*
 * ============================================================================
 * 🛑 STOP GLOBAL
 * ============================================================================
 *
 * Interrompe qualquer movimento em andamento.
 *
 * IMPORTANTE:
 *  - parada é assíncrona
 *  - motion layer monitora flag
 *
 * ============================================================================
 */
void seringa_stop(void)
{
    /*
     * ================================================================
     * 🧾 LOG
     * ================================================================
     */
    ESP_LOGW(
        TAG,
        "STOP solicitado"
    );

    /*
     * ================================================================
     * 🛑 STOP MOTOR
     * ================================================================
     */
    motor_stop();

    /*
     * ================================================================
     * 🔄 REFRESH
     * ================================================================
     */
    seringa_update_status();
}

/*
 * ============================================================================
 * 📊 STATUS GLOBAL
 * ============================================================================
 *
 * Retorna estado lógico atualizado.
 *
 * ============================================================================
 */
seringa_status_t seringa_get_status(void)
{
    /*
     * ================================================================
     * 🔄 REFRESH
     * ================================================================
     */
    seringa_update_status();

    return g_current_status;
}

/*
 * ============================================================================
 * 🔎 HELPERS
 * ============================================================================
 *
 * Helpers evitam:
 *  - duplicação
 *  - espalhamento de lógica
 *  - acesso direto ao hardware
 *
 * ============================================================================
 */

/*
 * ============================================================================
 * 🚀 MOTOR OCUPADO
 * ============================================================================
 */
bool seringa_is_busy(void)
{
    return motor_is_running();
}

/*
 * ============================================================================
 * 🔴 SERINGA CHEIA
 * ============================================================================
 */
bool seringa_is_cheia(void)
{
    return
        motor_back_endstop_triggered();
}

/*
 * ============================================================================
 * 🔴 SERINGA VAZIA
 * ============================================================================
 */
bool seringa_is_vazia(void)
{
    return
        motor_front_endstop_triggered();
}
