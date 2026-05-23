# API HTTP

Todos os endpoints, exceto `/api/login`, exigem o header:

```text
Authorization: TOKEN_RETORNADO_NO_LOGIN
```

## Endpoints

| Método | Caminho | Corpo | Resposta |
| --- | --- | --- | --- |
| `POST` | `/api/login` | PIN em texto | Token de sessão em texto. |
| `GET` | `/api/status` | vazio | JSON com estado da seringa, motor e fins de curso. |
| `GET` | `/api/inc` | vazio | Injeta o volume padrão e retorna `OK` ou `BUSY`. |
| `GET` | `/api/dec` | vazio | Recarrega o volume padrão e retorna `OK` ou `BUSY`. |
| `GET` | `/api/fill` | vazio | Bloqueado enquanto os fins de curso estiverem desabilitados. |
| `GET` | `/api/calibration/get` | vazio | JSON com `steps_per_ml`. |
| `POST` | `/api/calibration/set` | número em texto | Persiste a calibração e retorna `OK`. |
| `GET` | `/api/stop` | vazio | Para o movimento atual e retorna `OK`. |

## Exemplo de Status

```json
{
  "seringa": "IDLE",
  "motor": "IDLE",
  "busy": false,
  "cheia": false,
  "vazia": false
}
```

## Códigos de Erro

| Código | Situação comum |
| --- | --- |
| `400` | Body inválido, valor inválido ou falha ao iniciar movimento. |
| `401` | PIN incorreto, token ausente, token inválido ou sessão expirada. |
