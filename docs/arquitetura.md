# Arquitetura

## Fluxo de Boot

1. `app_main()` inicializa NVS.
2. O RNG é semeado com `esp_random()` e tempo do sistema.
3. `motor_init()` configura GPIOs e inicia a task interna do motor.
4. `storage_init()`, `calibration_init()` e `seringa_init()` carregam o estado lógico.
5. `wifi_init()` conecta o ESP32 à rede configurada.
6. `start_webserver()` registra rotas estáticas e rotas de API.

## Camadas

| Camada | Responsabilidade |
| --- | --- |
| `main/main.c` | Sequência de inicialização. |
| `main/motor` | Movimento físico, rampa, GPIOs e parada. |
| `main/seringa` | Regras de injeção, recarga, volume e estados. |
| `main/calibration` | Valor de `steps_per_ml` e limites de calibração. |
| `main/storage` | Persistência em NVS. |
| `main/wifi` | Conexão WiFi em station mode. |
| `main/web` | HTTP, autenticação, arquivos estáticos e API. |
| `data` | HTML, CSS e JavaScript servidos via SPIFFS. |

## Segurança

- WiFi e hash do PIN são injetados por `.env`/ambiente no build.
- O token de sessão fica apenas em RAM e expira após 24 horas de inatividade.
- Endpoints de controle exigem header `Authorization` com o token retornado pelo login.
- O hash do PIN não é mais registrado nos logs de tentativa de login.

## Diagramas

Diagramas editáveis e imagens renderizadas:

- `docs/arquitetura.puml` / `docs/arquitetura.png`: componentes e integrações.
- `docs/casos-uso.puml` / `docs/casos-uso.png`: atores e casos de uso.
- `docs/fluxo-boot.puml` / `docs/fluxo-boot.png`: inicialização do firmware.
- `docs/fluxo-login-comando.puml` / `docs/fluxo-login-comando.png`: autenticação e execução de comando.
- `docs/estados-seringa.puml` / `docs/estados-seringa.png`: estados principais da seringa.
