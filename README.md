# Seringa Automatizada

Firmware ESP-IDF para controle de uma seringa automatizada com motor de passo, fins de curso opcionais, calibração persistida em NVS e interface web embarcada.

## Visão Geral

O sistema inicializa o ESP32, configura storage em NVS, carrega a calibração, sincroniza o domínio da seringa, conecta no WiFi e publica uma interface web pelo servidor HTTP interno do ESP-IDF.

Módulos principais:

- `main/motor`: controle assíncrono do motor, GPIOs, rampa, segmentos e fins de curso.
- `main/seringa`: regras de domínio para injetar, recarregar, encher e parar.
- `main/calibration`: leitura, validação e persistência de `steps_per_ml`.
- `main/storage`: fachada de persistência sobre NVS.
- `main/web`: servidor HTTP, autenticação, rotas estáticas e API.
- `main/wifi`: conexão WiFi em modo station.
- `data`: interface web gravada na partição SPIFFS.

## Configuração Sensível

Os dados sensíveis ficam fora do código-fonte, em `.env`.

```sh
cp .env.example .env
```

Edite:

```env
SERINGA_WIFI_SSID=nome-da-rede
SERINGA_WIFI_PASS=senha-da-rede
SERINGA_PIN_HASH=0x00000000
```

O `.env` é ignorado pelo Git. O CMake também aceita as mesmas variáveis vindas do ambiente do shell.

## Build e Flash

Com ESP-IDF carregado no ambiente:

```sh
idf.py set-target esp32
idf.py build
idf.py flash monitor
```

A partição SPIFFS é gerada a partir de `data/` pelo `spiffs_create_partition_image`.

## Interface Web

Depois do boot, veja o IP no monitor serial. A interface web fica disponível em:

```text
http://IP_DO_ESP32/
```

O login envia o PIN em texto para `/api/login`; o firmware compara o hash DJB2 configurado por `SERINGA_PIN_HASH` e retorna um token de sessão em RAM.

## Documentação

- [Configuração](docs/configuracao.md)
- [Arquitetura](docs/arquitetura.md)
- [API HTTP](docs/api.md)
- [Arquitetura PlantUML](docs/arquitetura.puml) / [PNG](docs/arquitetura.png)
- [Casos de Uso PlantUML](docs/casos-uso.puml) / [PNG](docs/casos-uso.png)
- [Fluxo de Boot PlantUML](docs/fluxo-boot.puml) / [PNG](docs/fluxo-boot.png)
- [Fluxo Login/Comando PlantUML](docs/fluxo-login-comando.puml) / [PNG](docs/fluxo-login-comando.png)
- [Estados da Seringa PlantUML](docs/estados-seringa.puml) / [PNG](docs/estados-seringa.png)
