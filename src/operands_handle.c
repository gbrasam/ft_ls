/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   operands_handle.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/15 16:09:05 by gebravo           #+#    #+#             */
/*   Updated: 2026/10/29 17:40:35 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

/*
 * Los errores se muestran antes de ordenar para conservar
 * el orden original de argv, como hace GNU ls.
 *
 * Después, sort_operands() puede reorganizar el array sin
 * afectar al orden de los errores ya mostrados.
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

static void	handle_files(t_ctx *ctx, t_operand_list *operand_list)
{
	size_t	i;

	(void)ctx;
	i = 0;
	while (i < operand_list->count)
	{
		if (operand_list->items[i].kind == OPS_FILE)
		{
			fprintf(stdout, "%s\n", operand_list->items[i].name);
			/* TODO incorporar metadatos para -l */
		}
		i++;
	}
}

static void	handle_directories(t_ctx *ctx, t_operand_list *operand_list)
{
	size_t	i;

	i = 0;
	while (i < operand_list->count && ctx->fatal == FATAL_NONE)
	{
		if (operand_list->items[i].kind == OPS_DIRECTORY)
			list_directory(ctx, operand_list->items[i].name);
		i++;
	}
	/* TODO coordinar la recursividad -R */
}

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
	sort_operands(ctx, operand_list);
	if (ctx->fatal != FATAL_NONE)
		return ;
	handle_files(ctx, operand_list);
	if (ctx->fatal != FATAL_NONE)
		return ;
	handle_directories(ctx, operand_list);
}
