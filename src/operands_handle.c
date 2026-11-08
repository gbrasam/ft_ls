/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   operands_handle.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/15 16:09:05 by gebravo           #+#    #+#             */
/*   Updated: 2026/11/08 13:10:15 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

/* ----------------------------------------------------------- handle_error ----------------------------------------------------------- */

/*
 * Gestiona los operandos clasificados como OPS_ERROR.
 *
 * Se ejecuta antes de sort_operands() para conservar el
 * orden original de argv en los mensajes de error.
 *
 * Recorre operand_list y muestra por stderr un mensaje
 * por cada operando cuyo kind sea OPS_ERROR.
 *
 * ctx->status = 2 indica que se ha producido un error.
 * No detiene el procesamiento de los operandos válidos.
 *
 * Las opciones de ordenación (-r y -t) no modifican
 * el orden de los errores mostrados, porque estos
 * se procesan antes de reorganizar el array.
 *
 * Los operandos OPS_ERROR permanecen en el array,
 * pero handle_files() y handle_directories() los omiten.
 */
static void handle_error(t_ctx *ctx, t_operand_list *operand_list)
{
	size_t	i;

	i = 0;
	while (i < operand_list->count)
	{
		if (operand_list->items[i].kind == OPS_ERROR)
		{
			fprintf(stderr, "%s: cannot access '%s': %s\n",
				ctx->prog,
				operand_list->items[i].name,
				strerror(operand_list->items[i].err));
			ctx->status = 2;
		}
		i++;
	}
}

/* ----------------------------------------------------------- handle_files ----------------------------------------------------------- */

/*
 * Solo se consideran los operandos OPS_FILE
 * OPS_DIRECTORY se procesan por separado
 * y OPS_ERROR no dispone de metadatos válidos
 */
static void	prepare_files_widths(const t_operand_list *operand_list, t_print_widths *widths)
{
	size_t	i;

	i = 0;
	while (i < operand_list->count)
	{
		if (operand_list->items[i].kind == OPS_FILE)
			update_print_widths(&operand_list->items[i].st, widths);
		i++;
	}
}

/*
 * en los operandos, 'name' es tanto el nombre mostrado como la ruta del archivo
 */
static void	print_files(t_ctx *ctx, const t_operand_list *operand_list, const t_print_widths *widths)
{
	size_t	i;

	i = 0;
	while (i < operand_list->count)
	{
		if (ctx->fatal != FATAL_NONE)
			return ;
		if (operand_list->items[i].kind == OPS_FILE)
		{
			if (ctx->opts.l)
				print_long_format(ctx, operand_list->items[i].name, operand_list->items[i].name, &operand_list->items[i].st, widths);
			else
				fprintf(stdout, "%s\n", operand_list->items[i].name);
		}
		i++;
	}
}

static void	handle_files(t_ctx *ctx, t_operand_list *operand_list)
{
	t_print_widths	widths;

	init_print_widths(&widths);
	if (ctx->opts.l)
		prepare_files_widths(operand_list, &widths);
	print_files(ctx, operand_list, &widths);
}

/* -------------------------------------------------------- handle_directories -------------------------------------------------------- */

/*
 * handle_directories() — nivel de operandos
 * Gestiona los directorios que el usuario ha pasado por argumentos
 * Su responsabilidad es recorrer operand_list, identificar los operandos OPS_DIRECTORY
 * y solicitar el procesamiento de cada uno.
 */
static void	handle_directories(t_ctx *ctx, t_operand_list *operand_list)
{
	size_t	i;

	i = 0;
	while (i < operand_list->count)
	{
		if (ctx->fatal != FATAL_NONE)
			return ;
		if (operand_list->items[i].kind == OPS_DIRECTORY)
			list_directory(ctx, operand_list->items[i].name);
		i++;
	}
}

/* --------------------------------------------------------- handle_operands --------------------------------------------------------- */

/*
 * [GNU ls]	presenta los operandos no directorio antes de listar los directorios,
 * 			independientemente del orden de los argumentos
 * [GNU ls]	informa de todos los errores primero, luego muestra los archivos
 *			y por último los directorios
 *
 * Con handle_error() antes de sort_operands() se resuelve la falta de estabilidad de qsort
 * sin necesidad de un índice ni de un algoritmo estable:
 * los errores ya están impresos en el orden de argv cuando el array se reordena
 */
void	handle_operands(t_ctx *ctx, t_operand_list *operand_list)
{
	if (ctx->fatal != FATAL_NONE)
		return ;
	handle_error(ctx, operand_list);

	if (ctx->fatal != FATAL_NONE)
		return ;
	init_long_format_time(ctx);												/* inicializa ctx->now (clock_gettime()) solo si está activada la opción -l */

	if (ctx->fatal != FATAL_NONE)
		return ;
	sort_operands(ctx, operand_list);

	if (ctx->fatal != FATAL_NONE)
		return ;
	handle_files(ctx, operand_list);

	if (ctx->fatal != FATAL_NONE)
		return ;
	handle_directories(ctx, operand_list);
}
