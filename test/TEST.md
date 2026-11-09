## Etapa 5 — Formato largo (`-l`)

### Entradas sin metadatos (`stat_ok == 0`)

**Prueba realizada:**
- Directorio con permisos `644`, sin permiso de búsqueda (`x`).
- Comparación con GNU `ls` usando `LC_ALL=C`.
- Salida estándar: idéntica.
- Salida de errores: idéntica, normalizando el nombre del programa.
- Código de retorno: idéntico.

**Caso mixto (`stat_ok == 1` y `stat_ok == 0`):**
- Pendiente de reproducción dinámica.
- Cobertura prevista mediante revisión estática del código:
  cálculo de anchos, impresión de campos desconocidos y
  continuidad del recorrido.
- No se considera probado experimentalmente.
- Estado: pendiente de confirmar la revisión estática.