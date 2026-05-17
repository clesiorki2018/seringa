/*
 * Copyright 2026 clesiorki
 *
 * Licensed under the Apache License, Version 2.0.
 * See the LICENSE file in the project root for full license information.
 */

#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <stdbool.h>

/*
 * ============================================================================
 * 🧠 CALIBRATION MODULE (DOMAIN LOGIC)
 * ============================================================================
 *
 * Responsável por:
 *  - armazenar fator de calibração (steps por ml)
 *  - permitir ajuste fino via UI
 *  - fornecer conversão ml → steps
 *  - persistir valor (NVS futuramente)
 *
 * IMPORTANTE:
 *  - este módulo NÃO conhece motor
 *  - este módulo NÃO conhece hardware
 *  - ele é PURAMENTE lógica de domínio
 *
 * Arquitetura:
 *  UI → calibration → seringa → motor
 * ============================================================================
 */

/*
 * ============================================================================
 * 📊 CONFIGURAÇÃO PADRÃO
 * ============================================================================
 *
 * Valor inicial (fallback caso não exista no storage)
 *
 * Base teórica:
 *  - Motor 28BYJ-48 (4096 steps por volta real)
 *  - Redução 1:64 já incluída
 *  - Barra M8 (passo 1.25 mm)
 *
 * Resultado aproximado:
 *  ~3276 steps por mm
 *
 * Conversão para ml depende da seringa
 */
#define CALIBRATION_DEFAULT 4000.0f

/*
 * ============================================================================
 * 🚀 INIT
 * ============================================================================
 *
 * Carrega calibração do storage (ou usa default)
 */
void calibration_init(void);

/*
 * ============================================================================
 * 📥 GET
 * ============================================================================
 *
 * Retorna fator atual (steps por ml)
 */
float calibration_get(void);

/*
 * ============================================================================
 * 📤 SET DIRETO
 * ============================================================================
 *
 * Permite definir valor absoluto (usado via API/UI)
 */
void calibration_set(float value);

/*
 * ============================================================================
 * ➕ AJUSTE FINO
 * ============================================================================
 *
 * Incremento/decremento pequeno
 * (ideal para botões + / - da interface)
 */
void calibration_increase(void);
void calibration_decrease(void);

/*
 * ============================================================================
 * 🔄 CONVERSÃO
 * ============================================================================
 *
 * Converte ml → steps usando fator atual
 */
int calibration_ml_to_steps(float ml);

#endif