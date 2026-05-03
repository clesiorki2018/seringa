#ifndef MOTOR_H
#define MOTOR_H

#include <stdbool.h>

/*
 * ============================================================================
 * 🧠 MOTOR - INTERFACE PÚBLICA (DRIVER ABSTRATO)
 * ============================================================================
 *
 * Este módulo é responsável por:
 *
 *  - Controlar o motor de passo (28BYJ-48 + ULN2003)
 *  - Executar movimentos de forma ASSÍNCRONA (via FreeRTOS)
 *  - Garantir controle seguro (STOP imediato + fim de curso)
 *
 * IMPORTANTE:
 *  - Este módulo NÃO deve conter lógica de negócio
 *  - Ele NÃO sabe o que é "seringa", "ml", etc
 *  - Ele apenas EXECUTA movimentos
 *
 * Arquitetura:
 *
 *   seringa.c  ───► motor.c ───► GPIO (hardware)
 *
 * Ou seja:
 *   - motor = camada de execução (baixo nível)
 *   - seringa = camada de decisão (alto nível)
 *
 * ============================================================================
 */


/*
 * ============================================================================
 * 🔄 TIPOS DE COMANDO
 * ============================================================================
 *
 * Representa ações possíveis para o motor.
 *
 * OBS:
 *  - MOTOR_CMD_NONE → reservado (não usado diretamente)
 *  - MOTOR_CMD_FORWARD → avança (empurra êmbolo)
 *  - MOTOR_CMD_BACKWARD → recua (puxa êmbolo)
 *  - MOTOR_CMD_STOP → parada imediata (interrompe qualquer movimento)
 */
typedef enum {
    MOTOR_CMD_NONE = 0,
    MOTOR_CMD_FORWARD,
    MOTOR_CMD_BACKWARD,
    MOTOR_CMD_STOP
} motor_cmd_t;


/*
 * ============================================================================
 * 🚀 INICIALIZAÇÃO
 * ============================================================================
 *
 * Responsável por:
 *  - Configurar GPIOs (ULN2003)
 *  - Configurar fins de curso (inputs)
 *  - Criar fila de comandos (FreeRTOS Queue)
 *  - Criar task dedicada (motor_task)
 *
 * Deve ser chamado UMA única vez no boot (main.c).
 */
void motor_init(void);


/*
 * ============================================================================
 * 📡 ENVIO DE COMANDOS (ASSÍNCRONO)
 * ============================================================================
 *
 * Envia um comando para a fila interna do motor.
 *
 * Parâmetros:
 *  - cmd   : tipo de movimento (forward/backward/stop)
 *  - steps : quantidade de passos a executar
 *
 * Comportamento:
 *  - NÃO bloqueia (retorna imediatamente)
 *  - Se a fila estiver cheia, o comando pode ser descartado
 *  - Execução ocorre na motor_task (thread separada)
 *
 * Regras:
 *  - steps deve ser > 0 (exceto para STOP)
 *  - não garante execução imediata (depende da fila)
 *
 * Segurança:
 *  - fim de curso é validado dentro da task (runtime)
 */
void motor_send_command(motor_cmd_t cmd, int steps);


/*
 * ============================================================================
 * 🛑 PARADA IMEDIATA
 * ============================================================================
 *
 * Envia comando STOP para a fila.
 *
 * Garantias:
 *  - interrompe movimento em andamento
 *  - desliga bobinas do motor
 *
 * Observação:
 *  - não bloqueia
 *  - depende da latência da task (quase tempo real)
 */
void motor_stop(void);


/*
 * ============================================================================
 * 📊 ESTADO DO MOTOR
 * ============================================================================
 *
 * Retorna:
 *  - true  → motor está executando movimento
 *  - false → motor parado
 *
 * IMPORTANTE:
 *  - baseado em variável interna (running)
 *  - pode haver pequeno atraso de sincronização (FreeRTOS)
 *
 * Uso típico:
 *  - evitar envio de comandos simultâneos
 *  - UI (status)
 *  - lógica da seringa
 */
bool motor_is_running(void);


#endif