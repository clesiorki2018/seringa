/*
 * Copyright 2026 clesiorki
 *
 * Licensed under the Apache License, Version 2.0.
 * See the LICENSE file in the project root for full license information.
 */

#ifndef MOTOR_CONFIG_H
#define MOTOR_CONFIG_H

/*
 * ============================================================================
 * 🔧 CONFIGURAÇÃO DE HARDWARE
 * ============================================================================
 *
 * Hardware:
 *  - ESP32 WROOM DevKit 38 pinos
 *  - Shield/placa de expansão ESP32 38 pinos
 *  - ULN2003
 *  - 28BYJ-48
 *  - Fuso TR/M8 equivalente
 *  - Acoplador flexível metálico
 *
 * Cuidados elétricos:
 *  - use o shield apenas para alimentação e distribuição de energia
 *  - evite usar GPIOs do shield para sinais críticos do motor
 *  - utilize capacitor eletrolítico de 470 µF a 1000 µF na alimentação do motor
 *  - adicione capacitor de 100 nF próximo ao ULN2003 / driver
 *
 * IMPORTANTE:
 *  - Centralizar TODOS os parâmetros aqui
 *  - Nunca espalhar GPIOs/timing pelo projeto
 * ============================================================================
 */

/*
 * ============================================================================
 * 🔌 GPIOs DO DRIVER
 * ============================================================================
 */

#define MOTOR_GPIO_IN1                  18
#define MOTOR_GPIO_IN2                  19
#define MOTOR_GPIO_IN3                  21
#define MOTOR_GPIO_IN4                  22

/*
 * ============================================================================
 * 🔴 FINS DE CURSO
 * ============================================================================
 *
 * Fins de curso temporariamente desabilitados nesta branch.
 *
 * FRONT:
 *  êmbolo avançado / seringa vazia
 *
 * BACK:
 *  êmbolo retraído / seringa cheia
 *
 * MOTOR_ENDSTOPS_INSTALLED:
 *  0 -> hardware ainda sem fins de curso instalados
 *  1 -> sensores físicos instalados e conectados
 *
 * IMPORTANTE:
 *  - enquanto estiver em 0, homing/enchimento total deve ficar bloqueado
 *  - movimentos parciais continuam limitados por steps/calibração
 *  - mudar para 1 apenas depois de retomar a branch de endstops e
 *    validar elétrica, pinagem e sentido mecânico
 * ============================================================================
 */

#define MOTOR_ENDSTOPS_INSTALLED        0

#define MOTOR_ENDSTOP_FRONT_GPIO        26
#define MOTOR_ENDSTOP_BACK_GPIO         27

/*
 * Pinagem preservada apenas para retomada futura dos endstops.
 *
 * IMPORTANTE:
 *  - com MOTOR_ENDSTOPS_INSTALLED em 0, estes GPIOs não são configurados
 *    como endstops pelo firmware
 *  - a branch feature/endstops-futuro preserva o ponto de retomada
 */
#define MOTOR_ENDSTOP_ACTIVE_LEVEL      1

/*
 * ============================================================================
 * 🔘 BOTÕES FÍSICOS
 * ============================================================================
 *
 * Temporariamente removidos do firmware para liberar GPIOs aos endstops.
 * ============================================================================
 */

/*
 * ============================================================================
 * 🔄 INVERSÃO GLOBAL DE DIREÇÃO
 * ============================================================================
 *
 * Corrige montagem mecânica invertida.
 *
 * IMPORTANTE:
 *  - inverter AQUI
 *  - nunca inverter em lógica de negócio
 * ============================================================================
 */

#define MOTOR_DIRECTION_INVERTED        1

/*
 * ============================================================================
 * ⚙️ MECÂNICA DO SISTEMA
 * ============================================================================
 */

/*
 * 28BYJ-48 em half-step:
 *
 * ~4096 half-steps por volta.
 */
#define MOTOR_STEPS_PER_REVOLUTION      4096

/*
 * Passo do fuso.
 *
 * Exemplo:
 *  - fuso 5 mm
 *  - 1 volta = 5 mm lineares
 */
#define MOTOR_LEADSCREW_PITCH_MM        5.0f

/*
 * ============================================================================
 * ⏱️ TIMING BASE
 * ============================================================================
 *
 * IMPORTANTE:
 *
 * Sistema possui:
 *  - carga axial
 *  - atrito
 *  - compressão de fluido
 *  - acoplador flexível
 *
 * Portanto:
 *  - velocidade agressiva causa:
 *      -> perda de passo
 *      -> stick-slip
 *      -> ressonância
 *      -> torque baixo
 * ============================================================================
 */

/*
 * Delay inicial da rampa.
 *
 * Partida extremamente suave.
 */
#define MOTOR_RAMP_START_DELAY_US       3200

/*
 * Delay mínimo operacional.
 *
 * NÃO reduzir demais no 28BYJ.
 */
#define MOTOR_RAMP_MIN_DELAY_US         1400

/*
 * Região de aceleração/desaceleração.
 */
#define MOTOR_RAMP_STEPS                160

/*
 * ============================================================================
 * 🧠 SEGMENTAÇÃO DE MOVIMENTO
 * ============================================================================
 *
 * Divide movimentos longos.
 *
 * Benefícios:
 *  - STOP mais responsivo
 *  - menor erro acumulado
 *  - permite compensação dinâmica
 * ============================================================================
 */

#define MOTOR_SEGMENT_STEPS             64

/*
 * ============================================================================
 * ⚙️ ANTI-STICTION / ANTI-STICK-SLIP
 * ============================================================================
 *
 * Estratégia inspirada em:
 *  - CNC peck drilling
 *  - extrusoras
 *  - bombas dosadoras
 *
 * Problema:
 *
 * Sistemas com:
 *  - fuso
 *  - acoplador flexível
 *  - pressão hidráulica
 *  - seringas
 *
 * sofrem:
 *  - atrito estático
 *  - acumulação elástica
 *  - micro travamentos
 *  - "solavancos"
 *
 * Solução:
 *
 *      avança
 *      retrai microscopicamente
 *      avança novamente
 *
 * Isso:
 *  - alivia tensão mecânica
 *  - reduz stick-slip
 *  - melhora linearidade
 *  - reduz pulsos bruscos
 * ============================================================================
 */

#define MOTOR_ANTI_STICTION_ENABLE          1

/*
 * Intervalo entre compensações.
 */
#define MOTOR_ANTI_STICTION_INTERVAL        48

/*
 * Micro retração.
 *
 * IMPORTANTE:
 *  - manter pequeno
 *  - valores altos criam pulsação visível
 */
#define MOTOR_ANTI_STICTION_BACKSTEPS       1

/*
 * Delay da micro retração.
 *
 * Movimento lento e suave.
 */
#define MOTOR_ANTI_STICTION_DELAY_US        2500

/*
 * ============================================================================
 * 🔄 COMPENSAÇÃO DE BACKLASH
 * ============================================================================
 *
 * Backlash:
 *  - folga mecânica
 *  - elasticidade torsional
 *  - folga da engrenagem do 28BYJ
 *
 * Especialmente importante em:
 *  - reversão de direção
 *  - dosagem precisa
 *
 * Estratégia:
 *
 * Ao inverter direção:
 *  aplica micro movimento de assentamento.
 * ============================================================================
 */

#define MOTOR_BACKLASH_COMPENSATION_ENABLE      1

/*
 * Quantidade de passos de assentamento.
 */
#define MOTOR_BACKLASH_COMPENSATION_STEPS       3

/*
 * ============================================================================
 * 🔌 HOLD TORQUE
 * ============================================================================
 *
 * Mantém bobinas energizadas após movimento.
 *
 * DECISÃO DE HARDWARE:
 *  - DESABILITADO neste projeto
 *  - a estrutura mecânica já impede rotação livre quando desenergizada
 *  - já houve superaquecimento do ULN2003 e do 28BYJ-48 com bobinas ligadas
 *
 * IMPORTANTE:
 *  - manter em 0 por segurança térmica
 *  - motor_motion.c sempre desenergiza bobinas ao final do movimento
 * ============================================================================
 */

#define MOTOR_HOLD_ENABLE                   0

/*
 * Mantido apenas como documentação/configuração futura.
 * Não é usado enquanto MOTOR_HOLD_ENABLE estiver 0.
 */
#define MOTOR_HOLD_TIMEOUT_MS               0

/*
 * ============================================================================
 * 🔴 ENDSTOP FILTER
 * ============================================================================
 */

#define MOTOR_ENDSTOP_CONFIRMATIONS         5

/*
 * ============================================================================
 * 🚨 LIMITES DE SEGURANÇA
 * ============================================================================
 */

/*
 * Proteção contra comandos absurdos.
 */
#define MOTOR_MAX_ALLOWED_STEPS             500000UL

/*
 * ============================================================================
 * 🧠 DEBUG
 * ============================================================================
 */

#define MOTOR_VERBOSE_LOGGING               0

#endif
