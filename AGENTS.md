# AGENTS

## Projeto Seringa - ESP32

Hardware suportado:

- ESP32 WROOM DevKit 38 pinos
- Shield/placa de expansão ESP32 38 pinos
- Motor de passo 28BYJ-48 com driver ULN2003
- Pinos do motor no ESP32:
  - IN1 = GPIO 18
  - IN2 = GPIO 19
  - IN3 = GPIO 21
  - IN4 = GPIO 22
- Target ESP-IDF: `esp32`

## Regras de desenvolvimento

1. Use ESP-IDF. Não use Arduino, a menos que seja solicitado explicitamente.
2. Escreva código em C.
3. Comente no estilo Linux: objetivo do bloco, decisões importantes e limitações de hardware.
4. Aplique Clean Code e princípios SOLID quando fizer sentido.
5. Separe responsabilidades entre módulos claros e coesos.
6. Mantenha definições de hardware centralizadas em `main/motor/motor_config.h`.
7. Evite usar GPIOs do shield para sinais críticos; use o shield apenas como fonte/distribuição de energia.
8. Considere o condicionamento de energia:
   - capacitor eletrolítico de 470 µF a 1000 µF na alimentação do motor;
   - capacitor de 100 nF próximo ao ULN2003 e ao motor.

## Processo de alteração

- Antes de qualquer refatoração significativa, sugira criar um checkpoint Git (`git status`, `git add`, `git commit`, ou `git branch` conforme necessário).
- Depois de alterar o código, sugira sempre:
  - rodar o build (`idf.py build`);
  - revisar as diferenças (`git diff`);
  - commitar as mudanças com mensagem clara.

## Observações adicionais

- Prefira configurações e constantes no código em vez de espalhar valores mágicos.
- Documente qualquer ajuste de pinagem ou mudança de hardware no `README.md` e em `docs/configuracao.md`.
- Mantenha o foco em estabilidade elétrica e robustez do motor, especialmente na alimentação.
