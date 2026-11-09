#!/usr/bin/env bash
# Comparacion de la etapa 5 de ft_ls contra GNU ls (Linux).
set -u

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FT="$ROOT/ft_ls"
RESULTS="$ROOT/test/results_stage5"
PASS=0
FAIL=0
SKIP=0

if [[ ! -x "$FT" ]]; then
    printf 'ERROR: no existe el ejecutable %s\nEjecuta make desde la raiz.\n' "$FT" >&2
    exit 2
fi
if ! command -v ls >/dev/null || ! command -v diff >/dev/null; then
    echo 'ERROR: faltan ls o diff.' >&2
    exit 2
fi
mkdir -p "$RESULTS" || exit 2
TMP="$(mktemp -d)" || exit 2
trap 'rm -rf "$TMP"' EXIT
mkdir -p "$TMP/data/mix" "$TMP/data/empty"
printf 'archivo de prueba\n' > "$TMP/data/mix/normal.txt"
printf 'otro archivo\n' > "$TMP/data/mix/otro.txt"
printf 'oculto\n' > "$TMP/data/mix/.oculto"
ln -s normal.txt "$TMP/data/mix/enlace"
ln -s inexistente "$TMP/data/mix/enlace_roto"
: > "$TMP/data/mix/antiguo"
: > "$TMP/data/mix/futuro"
touch -d '2020-01-01 12:00:00 UTC' "$TMP/data/mix/antiguo"
touch -d '2030-01-01 12:00:00 UTC' "$TMP/data/mix/futuro"
# Asegura fechas reproducibles y evita que el orden de creacion importe.

run_case() {
    local id="$1" label="$2"; shift 2
    local dir="$RESULTS/$id"
    mkdir -p "$dir"
    (cd "$ROOT" && LC_ALL=C ls "$@") > "$dir/ls.out" 2> "$dir/ls.err"
    echo "$?" > "$dir/ls.code"
    (cd "$ROOT" && LC_ALL=C ./ft_ls "$@") > "$dir/ft.out" 2> "$dir/ft.err"
    echo "$?" > "$dir/ft.code"
    # Los nombres de los programas son la unica diferencia aceptada en stderr.
    sed 's/^ls:/PROGRAM:/' "$dir/ls.err" > "$dir/ls.err.norm"
    sed 's|^\./ft_ls:|PROGRAM:|' "$dir/ft.err" > "$dir/ft.err.norm"
    diff -u "$dir/ls.out" "$dir/ft.out" > "$dir/stdout.diff" || :
    diff -u "$dir/ls.err.norm" "$dir/ft.err.norm" > "$dir/stderr.diff" || :
    diff -u "$dir/ls.code" "$dir/ft.code" > "$dir/exit.diff" || :
    if [[ ! -s "$dir/stdout.diff" && ! -s "$dir/stderr.diff" && ! -s "$dir/exit.diff" ]]; then
        printf '[PASS] %s\n' "$label"
        PASS=$((PASS + 1))
    else
        printf '[FAIL] %s  (ver %s)\n' "$label" "$dir"
        FAIL=$((FAIL + 1))
    fi
}

skip_case() {
    printf '[SKIP] %s — %s\n' "$1" "$2"
    SKIP=$((SKIP + 1))
}

printf '\n=== FT_LS — ETAPA 5: PRUEBAS DE REGRESION ===\n\n'
run_case dates 'Fechas antigua y futura (-l)' -l "$TMP/data/mix/antiguo" "$TMP/data/mix/futuro"
run_case chars 'Dispositivos de caracteres (-l)' -l /dev/null /dev/zero
# No todos los sistemas (contenedores/VM) exponen nodos de bloque.
block=''
for candidate in /dev/loop0 /dev/sda /dev/vda /dev/nvme0n1; do
    if [[ -b "$candidate" ]]; then block="$candidate"; break; fi
done
if [[ -n "$block" ]]; then
    run_case blocks 'Dispositivo de bloques (-l)' -l "$block"
else
    skip_case 'Dispositivo de bloques (-l)' 'no hay nodo de bloques conocido accesible'
fi
run_case la 'Opcion -la (incluye . y ..)' -la "$TMP/data/mix"
run_case lt 'Opcion -lt' -lt "$TMP/data/mix"
run_case lr 'Opcion -lr' -lr "$TMP/data/mix"
run_case ltr 'Opcion -ltr' -ltr "$TMP/data/mix"
run_case files 'Archivos como operandos (-l)' -l "$ROOT/Makefile" "$ROOT/src/main.c"
run_case symlinks 'Enlace valido y roto (-l)' -l "$TMP/data/mix/enlace" "$TMP/data/mix/enlace_roto"
run_case empty 'Directorio vacio (-la)' -la "$TMP/data/empty"

printf '\n=============================================\n'
printf 'RESULTADO: %d PASS | %d FAIL | %d SKIP\n' "$PASS" "$FAIL" "$SKIP"
printf 'Resultados guardados en: %s\n' "$RESULTS"
printf '=============================================\n'
if (( FAIL > 0 )); then exit 1; fi
exit 0
