#ifndef STORAGE_H
#define STORAGE_H

#include <stdbool.h>
#include <stdint.h>

/*
 * ============================================================================
 * 🧠 STORAGE - PERSISTÊNCIA CENTRAL (NVS ABSTRACTION)
 * ============================================================================
 *
 * Papel do módulo:
 *
 *  - Persistir estado do sistema
 *  - Garantir consistência após reboot
 *  - Abstrair NVS completamente
 *
 * NÃO FAZ:
 *  - lógica de negócio
 *  - controle de motor
 *  - calibração matemática
 *
 * FAZ:
 *  - salvar/carregar dados críticos
 *  - versionamento de schema
 * ============================================================================
 */

/*
 * ============================================================================
 * 🚀 INIT
 * ============================================================================
 *
 * Deve ser chamado no boot antes de qualquer uso de calibração.
 */
void storage_init(void);

/*
 * ============================================================================
 * 📌 VERSÃO DO STORAGE SCHEMA
 * ============================================================================
 *
 * IMPORTANTE:
 *  - evita corrupção futura
 *  - permite migração de dados
 */
uint32_t storage_get_version(void);

/*
 * ============================================================================
 * 🎯 CALIBRAÇÃO (SERINGA)
 * ============================================================================
 */
bool storage_save_steps_per_ml(float value);
bool storage_load_steps_per_ml(float *value);

/*
 * ============================================================================
 * 🔄 CONFIGURAÇÃO DO MOTOR
 * ============================================================================
 */

/*
 * Direção invertida persistente
 */
bool storage_save_motor_inverted(bool inverted);
bool storage_load_motor_inverted(bool *inverted);

/*
 * ============================================================================
 * ⚙️ OFFSETS MECÂNICOS
 * ============================================================================
 *
 * Compensação de folga / montagem
 */
bool storage_save_backlash_steps(int32_t steps);
bool storage_load_backlash_steps(int32_t *steps);

/*
 * ============================================================================
 * 🧪 RESET DE FÁBRICA
 * ============================================================================
 *
 * Restaura tudo para padrão seguro
 */
bool storage_factory_reset(void);

#endif