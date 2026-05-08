#ifndef STORAGE_H
#define STORAGE_H

#include <stdbool.h>

/*
 * ============================================================================
 * 🧠 STORAGE - CAMADA DE PERSISTÊNCIA (NVS)
 * ============================================================================
 *
 * RESPONSABILIDADE:
 *  - Salvar dados persistentes
 *  - Carregar dados persistentes
 *
 * IMPORTANTE:
 *  - Nenhum módulo fora daqui deve usar NVS diretamente
 *  - Aqui é o "driver lógico" de armazenamento
 *
 * SEGUE PRINCÍPIOS:
 *  - SRP  → só cuida de persistência
 *  - DIP  → resto do sistema depende desta interface, não do NVS
 * ============================================================================
 */

/*
 * ============================================================================
 * 🚀 INIT
 * ============================================================================
 */
void storage_init(void);

/*
 * ============================================================================
 * 🎯 CALIBRAÇÃO
 * ============================================================================
 *
 * Armazena fator de calibração da seringa
 */
bool storage_save_calibration(float value);
bool storage_load_calibration(float *value);

#endif