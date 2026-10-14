/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   1_parse_args_checkpoint.c                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/11 09:08:48 by gebravo           #+#    #+#             */
/*   Updated: 2026/10/14 11:35:35 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "checkpoint.h"

/*
 * tabla completa de pruebas (g_parse_cases)
 * contiene los resultados esperados que hemos definido para comprobar el parser
 * P01–P20: pruebas que deben terminar correctamente (ret = 0)
 * E01–E13: pruebas que deben detectar una opción inválida (ret = -1)
 */
const t_case g_parse_cases[] = {
	{"P01", {NULL}, 0, FATAL_NONE, "", 0, 0, 0, {-1}},															/* ./test/checkpoint (sin argumentos) */
	{"P02", {"-l", NULL}, 0, FATAL_NONE, "l", 0, 0, 0, {-1}},													/* ./test/checkpoint -l (opción válida por separado)*/
	{"P03", {"-R", NULL}, 0, FATAL_NONE, "R", 0, 0, 0, {-1}},													/* ./test/checkpoint -R (opción válida por separado)*/
	{"P04", {"-a", NULL}, 0, FATAL_NONE, "a", 0, 0, 0, {-1}},													/* ./test/checkpoint -a (opción válida por separado)*/
	{"P05", {"-r", NULL}, 0, FATAL_NONE, "r", 0, 0, 0, {-1}},													/* ./test/checkpoint -r (opción válida por separado)*/
	{"P06", {"-t", NULL}, 0, FATAL_NONE, "t", 0, 0, 0, {-1}},													/* ./test/checkpoint -t (opción válida por separado)*/
	{"P07", {"-lRart", NULL}, 0, FATAL_NONE, "lRart", 0, 0, 0, {-1}},											/* ./test/checkpoint -lRart (opciones agrupadas) */
	{"P08", {"-l", "-R", "-a", "-r", "-t", NULL}, 0, FATAL_NONE, "lRart", 0, 0, 0, {-1}},						/* ./test/checkpoint -l -R -a -r -t (opciones separadas) */
	{"P09", {"-ll", "-l", NULL}, 0, FATAL_NONE, "l", 0, 0, 0, {-1}},											/* ./test/checkpoint -ll -l (opciones repetidas) */
	{"P10", {"d1", "-l", NULL}, 0, FATAL_NONE, "l", 0, 0, 0, {1, -1}},											/* ./test/checkpoint d1 -l (un operando seguido de una opción) */
	{"P11", {"d1", "-a", "d2", "-t", NULL}, 0, FATAL_NONE, "at", 0, 0, 0, {1, 3, -1}},							/* ./test/checkpoint d1 -a d2 -t (dos operandos intercalados con opciones.) */
	{"P12", {"a", "b", "c", NULL}, 0, FATAL_NONE, "", 0, 0, 0, {1, 2, 3, -1}},									/* ./test/checkpoint a b c (tres operandos sin opciones) */
	{"P13", {"--", NULL}, 0, FATAL_NONE, "", 0, 0, 0, {-1}},													/* ./test/checkpoint -- (-- sin argumentos posteriores) */
	{"P14", {"--", "-l", "d1", NULL}, 0, FATAL_NONE, "", 0, 0, 0, {2, 3, -1}},									/* ./test/checkpoint -- -l d1 (después de --, -l es un operando, no una opción) */
	{"P15", {"-l", "--", "-a", "--", "d1", NULL}, 0, FATAL_NONE, "l", 0, 0,0, {3, 4, 5, -1}},					/* ./test/checkpoint -l -- -a -- d1 (-l opcion, -a operando, después de --, incluso otro -- es un operando.) */
	{"P16", {"--", "-z", NULL}, 0, FATAL_NONE, "", 0, 0, 0, {2, -1}},											/* ./test/checkpoint -- -z después de --, -z no provoca error porque es un operando */
	{"P17", {"--", "--foo", NULL}, 0, FATAL_NONE, "", 0, 0, 0, {2, -1}},										/* ./test/checkpoint -- --foo después de --, --foo es un operando  */
	{"P18", {"-", NULL}, 0, FATAL_NONE, "", 0, 0, 0, {1, -1}},													/* ./test/checkpoint - (un - aislado es un operando) */
	{"P19", {"", NULL}, 0, FATAL_NONE, "", 0, 0, 0, {1, -1}},													/* ./test/checkpoint "" (una cadena vacía "" es un operando) */
	{"P20", {"-la", "-", "", "--", "-x", NULL}, 0, FATAL_NONE, "la", 0, 0, 0, {2, 3, 5, -1}},					/* ./test/checkpoint -la - "" -- -x  (combinación de opciones, operandos y --) */
	{"E01", {"-z", NULL}, -1, FATAL_OPTION, NULL, 'z', 0, 0, {-1}},												/* ./test/checkpoint -z  (opción inválida-short) */
	{"E02", {"-lz", NULL}, -1, FATAL_OPTION, NULL, 'z', 0, 0, {-1}},											/* ./test/checkpoint -lz (opción válida seguida de una inválida) */
	{"E03", {"-l-", NULL}, -1, FATAL_OPTION, NULL, '-', 0, 0, {-1}},											/* ./test/checkpoint -l- (el segundo carácter - es inválido dentro de -l-) */
	{"E04", {"-'", NULL}, -1, FATAL_OPTION, NULL, '\'', 0, 0, {-1}},											/* ./test/checkpoint "-'"  (el carácter ' es una opción inválida) */
	{"E05", {"-z", "-y", NULL}, -1, FATAL_OPTION, NULL, 'z', 0, 0, {-1}},										/* ./test/checkpoint -z -y (se detecta el primer error (-z) y no se continúa con -y) */
	{"E06", {"-z", "--foo", NULL}, -1, FATAL_OPTION, NULL, 'z', 0, 0, {-1}},									/* ./test/checkpoint -z --foo (se detecta -z y no se continúa con --foo) */
	{"E07", {"--foo", "-z", NULL}, -1, FATAL_OPTION, NULL, '\0', 1, 1, {-1}},									/* ./test/checkpoint --foo -z (se detecta -foo y no se continúa con -z) */
	{"E08", {"--foo", NULL}, -1, FATAL_OPTION, NULL, '\0', 1, 1, {-1}},											/* ./test/checkpoint --foo (opción inválida-long) */
	{"E09", {"--foo=bar", NULL}, -1, FATAL_OPTION, NULL, '\0', 1, 1, {-1}},										/* ./test/checkpoint --foo=bar (parser no admite opciones largas, por lo que rechaza el argumento completo --foo=bar) */
	{"E10", {"--l", NULL}, -1, FATAL_OPTION, NULL, '\0', 1, 1, {-1}},											/* ./test/checkpoint --l (se rechaza --l, aunque -l sea válida) */
	{"E11", {"---", NULL}, -1, FATAL_OPTION, NULL, '\0', 1, 1, {-1}},											/* ./test/checkpoint --- (opción inválida-long) */
	{"E12", {"d1", "-z", "d2", NULL}, -1, FATAL_OPTION, NULL, 'z', 0, 0, {-1}},									/* ./test/checkpoint d1 -z d2 (se detecta -z aunque aparezca entre operandos) */
	{"E13", {"-l", "directorio", "--foo", "--bar", NULL}, -1, FATAL_OPTION, NULL, '\0', 1, 3, {-1}},			/* ./test/checkpoint -l directorio --foo --bar (dos opciones largas inválidas: debe detectar la primera, --foo) */
	{.id = NULL}																								/* sentinel */
};

/*
 * Cuenta los argumentos hasta NULL, sin incluirlo
 * Ejemplo: {"d1", "-l", NULL} devuelve 2
 *
 * const char *const *args:
 * No permite modificar los caracteres ni los punteros del array
 * Sí permite avanzar el puntero local args
 */
static int	count_args(const char *const *args)
{
	int	i;

	i = 0;
	while (args[i])
		i++;
	return (i);
}

/*
 * Cuenta los índices hasta el sentinel -1.
 * Por ejemplo, en {"P20", ..., {2, 3, 5, -1} devuelve 3
 */
static int	count_indices(const int *indices)
{
	int	i;

	i = 0;
	while (indices[i] != -1)
		i++;
	return (i);
}

/*
 * copia los argumentos del caso en argv, a partir de argv[1]
 * porque argv[0] ya contiene el nombre del programa ("parse_args_checkpoint")
 * solo copia punteros; no duplica las cadenas
 */
static void	fill_argv(const t_case *test, char **argv)
{
	int i;

	i = 0;
	while (test->args[i])
	{
		argv[i + 1] = (char *)test->args[i];
		i++;
	}
	argv[i + 1] = NULL;
}

/*
 * resultado (ret): resultado real de parse_args()
 * vs
 * test->ret: resultado esperado, definido en la tabla g_parse_cases
 */
static int	check_ret(const t_case *test, int resultado)
{
	if (resultado != test->ret)
		return (1);
	return (0);
}

static int	check_fatal(const t_case *test, const t_ctx *ctx)
{
	if (test->fatal != ctx->fatal)
		return (1);
	return (0);
}

/*
 * Comprueba las cinco opciones: l, R, a, r, t.
 *
 * char *strchr(const char *s, int c) — <string.h>
 * Busca un carácter en una cadena.
 * Devuelve un puntero si lo encuentra; NULL si no lo encuentra.
 *
 * test->flags contiene las opciones que esperamos activadas.
 * ctx->opts contiene las opciones realmente activadas por parse_args().
 *
 * Si test->flags == NULL, se omite la comprobación:
 * la tabla no establece un resultado esperado para las opciones.
 * No debe confundirse NULL con "", que exige todas desactivadas.
 *
 * Para cada opción comparamos dos valores booleanos:
 * - Esperado: strchr(test->flags, 'l') != NULL
 * - Real:     ctx->opts.l != 0
 *
 * Si son diferentes (!=), la comprobación falla y devuelve 1.
 * Si todas coinciden, devuelve 0.
 *
 * Se comprueban tanto las opciones que deben estar activadas
 * como las que deben permanecer desactivadas.
 */
static int	check_flags(const t_case *test, const t_ctx *ctx)
{
	if (!test->flags)
		return (0);
	
	if ((strchr(test->flags, 'l') != NULL) != (ctx->opts.l != 0))
		return (1);
	if ((strchr(test->flags, 'R') != NULL) != (ctx->opts.big_r != 0))
		return (1);
	if ((strchr(test->flags, 'a') != NULL) != (ctx->opts.a != 0))
    	return (1);
	if ((strchr(test->flags, 'r') != NULL) != (ctx->opts.r != 0))
		return (1);
	if ((strchr(test->flags, 't') != NULL) != (ctx->opts.t != 0))
		return (1);

	return (0);
}

/*
 * Comprueba qué opción ha provocado el error y de qué tipo es.
 *
 * a.- ¿Qué tipo de opción inválida es?
 * Comparamos el tipo esperado (test->bad_long) con
 * ctx->invalid_kind (ARG_SHORT o ARG_LONG)
 *
 * b.- Si es una opción corta, ¿qué carácter es inválido?
 * ctx->invalid es un puntero (const char *),
 * mientras que test->bad_char es un carácter (char)
 * Comparamos *ctx->invalid con test->bad_char
 *
 * c.- Si es una opción larga, ¿qué argumento es inválido?
 * Comparamos la cadena señalada por ctx->invalid con
 * el argumento esperado mediante strcmp() (*)
 *
 * Devuelve 0 si las comprobaciones coinciden y 1 si alguna falla.
 * (*)
 * int strcmp(const char *s1, const char *s2); — <string.h>
 * Compara dos cadenas de caracteres, carácter a carácter
 * Devuelve 0 si son iguales
 * Devuelve un valor < 0 si s1 es menor que s2
 * Devuelve un valor > 0 si s1 es mayor que s2
 * 
 * Ejemplo E13:
 * {"E13", {"-l", "directorio", "--foo", "--bar", NULL}, -1, FATAL_OPTION, NULL, 0, 1, {-1}},
 *
 * argv[0] = "parse_args_checkpoint"
 * argv[1] = "-l"
 * argv[2] = "directorio"
 * argv[3] = "--foo"
 * argv[4] = "--bar"
 * argv[5] = NULL
 *
 * parse_args() debe detenerse en la primera opción inválida:
 * "--foo", sin continuar hasta "--bar"
 *
 * Esperamos:
 * ctx->invalid_kind == ARG_LONG
 * strcmp(ctx->invalid, argv[3]) == 0
 *
 * Si ctx->invalid señala "--bar", la prueba debe fallar
 *
 * ¿cómo sabe check_invalid() que debe comparar con argv[3]?
 * test->bad_long_pos guarda el índice de la primera opción
 * larga inválida que esperamos encontrar en argv.
 * Este valor está definido en la tabla de pruebas.
 *
 * Ejemplo E13:
 * bad_long_pos = 3
 * argv[3] = "--foo"
 * strcmp(ctx->invalid, argv[test->bad_long_pos]) == 0
 */
static int	check_invalid(const t_case *test, const t_ctx *ctx, char **argv)
{
	if (!ctx->invalid)
		return (1);																							/* estamos comprobando un caso en el que esperamos una opción inválida; si ctx->invalid es NULL, el parser no ha identificado qué opción ha provocado el error */
	if (test->bad_long == 1 && ctx->invalid_kind != ARG_LONG)
		return (1);
	if (test->bad_long == 0 && ctx->invalid_kind != ARG_SHORT)
		return (1);
	if (ctx->invalid_kind == ARG_SHORT && *ctx->invalid != test->bad_char)
		return (1);
	if (test->bad_long == 1 && strcmp(ctx->invalid, argv[test->bad_long_pos]) != 0)
		return (1);
	return (0);
}

/*
 * Comprueba que el parser ha almacenado los operandos correctos,
 * tanto en número como en orden.
 *
 * test->ops contiene los índices de argv donde esperamos encontrar
 * los operandos (sentinel = -1)
 *
 * ops->items contiene los punteros a las cadenas de los operandos
 * que realmente ha almacenado parse_args()
 * ops->count indica cuántos operandos ha almacenado
 *
 * Por ejemplo, en
 * {"P11", {"d1", "-a", "d2", "-t", NULL}, 0, FATAL_NONE, "at", 0, 0, 0, {1, 3, -1}},
 *
 * argv[1] = "d1"       operando
 * argv[2] = "-a"       opción
 * argv[3] = "d2"       operando
 * argv[4] = "-t"       opción
 *
 * test->ops = {1, 3, -1}
 *
 * Resultado esperado:
 * ops->count == 2
 * strcmp(ops->items[0], argv[1]) == 0   ("d1")
 * strcmp(ops->items[1], argv[3]) == 0   ("d2")
 *
 * Devuelve 0 si coinciden el número y el orden de los operandos,
 * o 1 si alguna comprobación falla.
 */
static int	check_operands(const t_case *test, const t_operands *ops, char **argv)
{
	int	i;

	if (ops->count != count_indices(test->ops))																/* número de operandos */
		return (1);

	if (ops->count == 0)																					/* casos sin operandos (P01, P02, ...) */
		return (ops->items != NULL);																		/* sin operandos y items == NULL -> 0 (correcto); sin operandos y items != NULL -> 1 (fallo)*/

	if (ops->count > 0 && !ops->items)
		return (1);

	i = 0;
	while (test->ops[i] != -1)
	{
		if (strcmp(ops->items[i], argv[test->ops[i]]) != 0)													/* mismos operandos y en orden */
			return (1);
		i++;
	}
	return (0);
}

/*
 * Ejecuta un caso de prueba y comprueba sus resultados
 * Devuelve 0 si todas las comprobaciones pasan; 1 si alguna falla
 *
 * const t_case *test:
 * No permite modificar los miembros del struct a través de test
 * Sí puedes cambiar a dónde apunta test, porque el puntero no es constante
 * En test->args, protege tanto los caracteres como los punteros del array
 * Por eso count_args recibe const char *const *args
 */
int	run_case(const t_case *test)
{
	int			argc;
	char		*argv[MAX_ARGS + 1];																		/* array local de tamaño fijo: nombre del programa + 7 args + NULL */
	t_ctx		ctx;
	t_operands	ops;
	int			ret;
	int			failed;

	argc = count_args(test->args) + 1;																		/* +1 para incluir argv[0], nombre del programa */
	argv[0] = "parse_args_checkpoint";
	fill_argv(test, argv);
	
	init_ctx(&ctx, argv[0]);
	init_ops(&ops);
	ret = parse_args(argc, argv, &ctx, &ops);
	failed = 0;
	failed += check_ret(test, ret);
	failed += check_fatal(test, &ctx);
	failed += check_flags(test, &ctx);
	if (test->fatal == FATAL_OPTION)																		/* si se espera un error de opción, comprobar que el parser haya identificado correctamente la opción inválida, su tipo y su posición o carácter */
		failed += check_invalid(test, &ctx, argv);
	if (test->ret == 0)																						/* si se espera éxito, comprobar los operandos almacenados */
		failed += check_operands(test, &ops, argv);															
	free(ops.items);
	return (failed != 0);
}
