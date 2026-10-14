/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/02 15:22:41 by gebravo           #+#    #+#             */
/*   Updated: 2026/10/14 11:51:30 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

/**
 * ¿qué tipo de argumento es?
 * clasificar, no validar ni modificar el estado del parser
 *
 * 'end_opts' es un flag interno del parser que recuerda
 * si ya hemos encontrado "--"
 * end_opts = 0 → todavía estamos interpretando opciones
 * end_opts = 1 → ya apareció "--"; no interpretar más opciones
 * 
 * ¿end_opts == 1?              → ARG_OPERAND
 * ¿no empieza por '-'?         → ARG_OPERAND
 * ¿es exactamente "-"?         → ARG_OPERAND
 * ¿es exactamente "--"?        → ARG_END_OPTIONS
 * ¿empieza por "--..."?        → ARG_LONG
 * resto que empieza por "-"    → ARG_SHORT
 *
 * end_opts          → 0 → falso
 * arg[0] != '-'     → '-' != '-' → falso
 * arg[1] == '\0'    → verdadero
 * → ARG_OPERAND
 *
 * si llegamos al segundo if, ya sabemos dos cosas:
 * arg[0] == '-'      porque arg[0] != '-' no se cumplió
 * arg[1] != '\0'     porque arg[1] == '\0' no se cumplió
 * escribir arg[0] == '-' sería redundante
 *
 * si llegamos al tercer if, es porque previamente hemos descartado
 * exactamente "--" -> necesariamente arg[2] != '\0'
 * 
 * if y no else if: el primer if ya hace return,
 * así que si continúa la ejecución sabemos necesariamente que no se cumplió
 */
static t_argkind	classify_arg(const char *arg, int end_opts)
{
	if (end_opts || arg[0] != '-' || arg[1] == '\0')
		return (ARG_OPERAND);
	if (arg[1] == '-' && arg[2] == '\0')
		return (ARG_END_OPTIONS);
	if (arg[1] == '-')
		return (ARG_LONG);
	return (ARG_SHORT);
}

/**
 * Si classify_arg() devuelve ARG_SHORT:
 * - comprobar arg[i]: procesa sus letras: l, R, a, r, t [SUBJECT]
 * - activa los flags correspondientes en ctx->opts
 * - si encuentra una letra no válida, devuelve -1
 *
 * i = 1 saltamos el '-' y empezamos directamente
 *       por la primera letra de opción.
 *
 * -lax
 * 'l' → activa l
 * 'a' → activa a
 * 'x' → default → return -1
 * En una opción agrupada, se procesan los caracteres de izquierda a derecha
 * y se detiene el parseo en el primer carácter no reconocido (x, en este caso)
 */
static int	parse_option(t_ctx *ctx, const char *arg)
{
	int	i;

	i = 1;
	while (arg[i])
	{
		switch (arg[i])
		{
			case 'l':
				ctx->opts.l = 1;
				break ;
			case 'R':
				ctx->opts.big_r = 1;
				break ;
			case 'a':
				ctx->opts.a = 1;
				break ;
			case 'r':
				ctx->opts.r = 1;
				break ;
			case 't':
				ctx->opts.t = 1;
				break ;
			default:
				ctx->invalid = &arg[i];
				return (-1);
		}
		i++;
	}
	return (0);
}	

/**
 * scan_args() recorre argv, clasifica los argumentos mediante
 * classify_arg(), valida las opciones y cuenta los operandos.
 *
 * Conocido el número de operandos, parse_args() podrá reservar
 * únicamente la memoria necesaria.
 *
 * store_operands() realizará una segunda pasada para guardar
 * los operandos en el orden en que aparecen en argv.
 *
 * int *operand_count
 * Contador temporal utilizado durante la primera pasada para averiguar
 * cuántos operandos ha escrito el usuario.
 * Lo necesitamos principalmente para saber cuánta memoria reservar.
 *
 * ops->count
 * Número de operandos almacenados en ops->items.
 *
 * Primera pasada:
 * operand_count = 3   -> hemos contado 3 operandos
 * ops->count = 0      -> todavía no hemos almacenado ninguno
 *
 * Reservamos memoria para 3 operandos.
 *
 * Segunda pasada:
 * ops->count = 1
 * ops->count = 2
 * ops->count = 3
 *
 * Al terminar correctamente:
 * operand_count  = 3
 * ops->count     = 3
 *
 * No son la misma variable, aunque su valor final esperado coincida.
 *
 * scan_args tiene que actuar según el tipo que devuelve classify_arg()
 * ARG_OPERAND     → incrementar contador
 * ARG_END_OPTIONS → poner end_opts = 1
 * ARG_SHORT       → procesar la opción (-a, -l, etc.)
 * ARG_LONG        → registrar error
 */
static int	scan_args(int argc, char **argv, t_ctx *ctx, int *operand_count)
{
	int			i;
	int			end_opts;
	t_argkind	arg_kind;

	i = 1;
	end_opts = 0;
	while (i < argc)
	{
		arg_kind = classify_arg(argv[i], end_opts);
		if (arg_kind == ARG_OPERAND)
			(*operand_count)++;
		else if (arg_kind == ARG_SHORT)
		{
			if (parse_option(ctx, argv[i]) == -1)
			{
				ctx->invalid_kind = ARG_SHORT;
				ctx->fatal = FATAL_OPTION;
				return (-1);
			}
		}
		else if (arg_kind == ARG_LONG)
		{
			ctx->invalid = argv[i];
			ctx->invalid_kind = ARG_LONG;
			ctx->fatal = FATAL_OPTION;
			return (-1);
		}
		else if (arg_kind == ARG_END_OPTIONS)
			end_opts = 1;
		i++;
	}
	return (0);
}

static void	store_operands(int argc, char **argv, t_operands *ops)
{
	int			i;
	int			end_opts;
	t_argkind	arg_kind;

	i = 1;
	end_opts = 0;
	while (i < argc)
	{
		arg_kind = classify_arg(argv[i], end_opts);
		if (arg_kind == ARG_END_OPTIONS)
			end_opts = 1;
		else if (arg_kind == ARG_OPERAND)
		{
			ops->items[ops->count] = argv[i];
			ops->count++;
		}
		i++;
	}
	ops->items[ops->count] = NULL;
}

/**
 * Reglas principales del parser:
 *
 * [SUBJECT] Opciones obligatorias de ft_ls: -l, -R, -a, -r y -t.
 *
 * [DISEÑO] Cualquier otra opción se considera no admitida.
 *          Debe detectarse, comunicar el error por stderr y finalizar
 *          con el estado de error correspondiente.
 *
 * [GNU] Para el comportamiento observable del error tomamos GNU ls
 *       como referencia.
 *
 * [C] argv[0] contiene el nombre con el que se invocó el programa.
 *     Los argumentos proporcionados al programa comienzan en argv[1].
 *
 * [GNU] Las opciones cortas pueden aparecer separadas o agrupadas:
 *       -l -a -t
 *       -lat
 *
 * [GNU] Las opciones pueden aparecer antes o después de los operandos.
 *       Mientras no aparezca "--", seguimos reconociendo opciones:
 *       ./ft_ls -a dir1
 *       ./ft_ls dir1 -a
 *
 * [GNU] "-" por sí solo es un operando.
 *
 * [GNU] "--" es opcional.
 *       Si aparece, marca el fin de las opciones y no es un operando.
 *       Todo argumento posterior se considera operando, aunque empiece
 *       por '-':
 *       ./ft_ls -- -a
 *		 
 *		 Para controlar este comportamiento usamos end_opts:
 *       end_opts = 0 -> todavía podemos encontrar opciones.
 *       end_opts = 1 -> ya apareció "--"; desde ahora todo es operando.
 *
 *       Si "--" no aparece, end_opts permanece a 0 durante todo
 *       el recorrido de la línea de comandos.
 *
 * [GNU] Un argumento que empiece por "--" y no sea exactamente "--"
 *       se interpreta como una opción larga. Como ft_ls no implementa
 *       opciones largas, se considera una opción no admitida.
 *
 * [GNU] Las opciones repetidas mantienen activado su flag:
 *       -a -a
 *       -aa
 *
 * [GNU] Si una opción corta agrupada contiene un carácter no válido,
 *       se procesan los caracteres en orden y el error se detecta en
 *       el primer carácter no reconocido.
 *
 * [DISEÑO] Los operandos se cuentan primero y posteriormente se almacenan
 *          conservando el orden en el que los escribió el usuario.
 *
 * [DISEÑO] Si el usuario no escribe ningún operando, el parser NO añade ".".
 *          El "." implícito se gestionará posteriormente.
 *
 * operand_count == 0, significa que:
 * Se han recorrido todos los argumentos de argv
 * No se ha detectado ningún error de opciones
 * Se han registrado las opciones válidas que hubiera (-l, -a, ...)
 * No se ha encontrado ningún operando explícito
 * Por tanto, el análisis ha terminado correctamente y no hay operandos que almacenar.
 */
int	parse_args(int argc, char **argv, t_ctx *ctx, t_operands *ops)
{
	int	operand_count;

	operand_count = 0;
	init_ops(ops);
	if (scan_args(argc, argv, ctx, &operand_count) == -1)
		return (-1);
	if (operand_count == 0)
		return (0);
	ops->items = malloc(sizeof(char *) * (operand_count + 1));
	if (!ops->items)
	{
		ctx->fatal = FATAL_NOMEM;
		return (-1);
	}
	store_operands(argc, argv, ops);
	return (0);
}
