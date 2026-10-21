# Contribuir

Los cambios de contabilidad deben especificar unidades, precondiciones,
transicion de estado y resultado esperado. Las funciones de solo lectura
deben incluir casos de limite y overflow. No cambie el contrato JSON sin
actualizar `docs/integracion.md` y tests Node.

```bash
bash scripts/ci.sh
```

La suite C nativa se ejecuta con `bash scripts/test-native.sh`. Mantenga las
decisiones de seguridad y las excepciones documentadas en `SECURITY.md`.
