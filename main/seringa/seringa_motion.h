/*
 * Copyright 2026 clesiorki
 *
 * Licensed under the Apache License, Version 2.0.
 * See the LICENSE file in the project root for full license information.
 */

#ifndef SERINGA_MOTION_H
#define SERINGA_MOTION_H

#include "seringa.h"

#include <stdbool.h>

/*
 * ============================================================================
 * 🧠 SERINGA MOTION - CAMADA DE MOVIMENTO HIDRÁULICO
 * ============================================================================
 *
 * Responsabilidades:
 *
 *  - converter volume em passos
 *  - traduzir perfil hidráulico em perfil de motor
 *  - executar injeção
 *  - executar recarga parcial
 *  - executar enchimento total
 *
 * NÃO deve conter:
 *
 *  - HTTP
 *  - GPIO
 *  - NVS
 *  - lógica de autenticação
 *  - manipulação direta de bobinas
 *
 * Arquitetura:
 *
 *   seringa.c
 *      ↓
 *   seringa_motion.c
 *      ↓
 *   motor.c
 *
 * ============================================================================
 */

/*
 * ============================================================================
 * 💉 INJEÇÃO
 * ============================================================================
 *
 * Converte ml para steps e solicita avanço do êmbolo.
 *
 * Retorna:
 *  true  -> comando aceito pelo motor
 *  false -> volume inválido, calibração inválida ou motor ocupado
 */
bool seringa_motion_injetar(
    float ml,
    seringa_flow_profile_t profile
);

/*
 * ============================================================================
 * ♻️ RECARGA PARCIAL
 * ============================================================================
 *
 * Converte ml para steps e solicita recuo do êmbolo.
 *
 * Retorna:
 *  true  -> comando aceito pelo motor
 *  false -> volume inválido, calibração inválida ou motor ocupado
 */
bool seringa_motion_recarregar(
    float ml,
    seringa_flow_profile_t profile
);

/*
 * ============================================================================
 * 🧪 ENCHIMENTO TOTAL
 * ============================================================================
 *
 * Recolhe o êmbolo até o fim de curso traseiro.
 *
 * Estratégia:
 *  - envia um movimento grande
 *  - o motor para ao detectar o endstop
 *
 * Retorna:
 *  true  -> comando aceito
 *  false -> motor ocupado ou falha no envio
 */
bool seringa_motion_encher_total(void);

#endif