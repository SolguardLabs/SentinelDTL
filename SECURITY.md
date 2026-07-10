# Seguridad

Este documento describe el modelo de seguridad esperado para SentinelDTL y el
alcance de revision recomendado.

## Modelo de seguridad

SentinelDTL asume un plano de control autorizado que administra pausas,
reanudaciones, modo de proteccion y ajustes de vault. Las cuentas y sesiones se
modelan como entidades internas ya autenticadas por la capa superior.

Los controles principales son:

- limites por sesion;
- limites por orden individual;
- limite agregado de cola;
- conteo maximo de ordenes por sesion;
- pausa global del plano de control;
- modo de proteccion con intake bloqueado;
- floor minimo de reserva de vault;
- conservacion de caja y unidades despues de las transiciones.

## Invariantes esperadas

- La caja total del sistema debe conservarse salvo aportes explicitos al vault.
- Las unidades de riesgo de cuentas deben coincidir con las unidades emitidas
  por el vault.
- Una cuenta no debe ejecutar unidades que no posee o que no tiene reservadas en
  cola.
- Una orden ejecutada o cancelada no puede volver a ejecutarse.
- Una sesion cerrada no debe aceptar nuevas transiciones.
- Una pausa debe bloquear intake y ejecucion.
- El modo de proteccion debe bloquear nuevo intake.
- El vault no debe cruzar su floor de reserva.

## Validacion automatizada

La suite publica cubre:

- settlement basico;
- snapshot determinista de arranque;
- pausa y reanudacion;
- rechazo por limites;
- procesamiento de cola admitida durante proteccion;
- ejecucion batch y cancelacion;
- conservacion de caja y unidades.

Los tests se ejecutan con:

```bash
npm test
```

El pipeline completo se ejecuta con:

```bash
npm run ci
```

## Alcance de revision

Areas prioritarias:

- `src/session.c`: contadores, limites, pausa y proteccion.
- `src/engine.c`: orden de mutaciones, rollback logico y coordinacion entre
  cuenta, sesion y vault.
- `src/vault.c`: calculo de redenciones, floor y versionado.
- `src/amount.c`: overflow, underflow y divisiones enteras.
- `src/json.c`: estabilidad de salida para integraciones.

Fuera de alcance de esta implementacion:

- autenticacion real de operadores;
- persistencia durable;
- concurrencia multihilo;
- red, RPC o transporte;
- gestion de secretos;
- firma criptografica de ordenes.

## Gestion de dependencias

El proyecto no usa dependencias runtime externas. Node se utiliza para tests y
tooling. Dependabot revisa GitHub Actions y el ecosistema npm.

## Reporte interno

Un reporte util debe incluir:

- escenario afectado;
- version o commit revisado;
- precondiciones;
- comandos de reproduccion;
- snapshot JSON relevante;
- impacto sobre caja, unidades, limites o disponibilidad;
- propuesta de correccion;
- tests adicionales sugeridos.
