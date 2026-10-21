# Integracion C y JSON

Las cabeceras de la API se encuentran en `include/sentinel/`. El ejecutable
`sentineldtl` enlaza todos los modulos de `src/` y devuelve un snapshot JSON
por escenario. Para un consumidor externo, la API C requiere inicializar
motor, gestionar el ciclo de cuentas/sesiones y comprobar cada `sg_status_t`.

## Consulta de capacidad

```c
#include "sentinel/analytics.h"

sg_liquidity_envelope_t envelope;
sg_error_t error;
sg_error_clear(&error);
sg_status_t status = sg_liquidity_envelope_from_vault(
    &engine.vault, 1000, &envelope, &error);
if (status == SG_OK) {
    /* envelope.redeem_capacity is an integer amount in vault cash units. */
}
```

El primer argumento es un vault existente; `1000` representa un estres del
10 %. La funcion no modifica el vault. Un `stress_bps` superior a `10_000`,
punteros nulos o un estado inconsistente devuelven error. El consumidor no
debe usar campos de salida si `status != SG_OK`.

## CLI

```bash
bash scripts/build.sh
./build/sentineldtl --list
./build/sentineldtl baseline
./build/sentineldtl protection
```

En Windows con WSL, el binario compilado por Bash reside en el filesystem
montado y se ejecuta desde el mismo entorno Linux. `scenario` no requiere
conexiones externas.

El JSON contiene `policy`, `vault`, `accounts`, `sessions`, `orders`, `events`,
`risk` y `checks`. Los IDs son textos estables; importes y contadores son
enteros. Una integracion debe validar la presencia de campos obligatorios,
conservar el orden de eventos y no convertir importes de 64 bits a flotantes.

## Compatibilidad

`v1.0.0` define la superficie estable de la CLI y cabeceras. Nuevos campos
de lectura pueden agregarse sin alterar campos existentes. Cambios de
unidades, estados o significado economico requieren version nueva y tests de
compatibilidad del JSON.
