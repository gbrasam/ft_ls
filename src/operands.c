/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   operands.c                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/15 16:09:05 by gebravo           #+#    #+#             */
/*   Updated: 2026/10/19 17:10:28 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

static void	handle_error(t_ctx *ctx, t_operand_info *operand)
{
	fprintf(stderr, "%s: cannot access '%s': %s\n",
		ctx->prog,
		operand->name,
		strerror(operand->err));
	ctx->status = 2;
}

static void	handle_file(t_ctx *ctx, t_operand_info *operand)
{
	(void)ctx;
	fprintf(stdout, "%s\n", operand->name);
	/* TODO incorporar la presentación de metadatos para -l */
}

static void	handle_non_directories(t_ctx *ctx, t_operand_list *operand_list)
{
	size_t	i;

	i = 0;
	while (i < operand_list->count)
	{
		if (operand_list->items[i].kind == OPS_ERROR)
			handle_error(ctx, &operand_list->items[i]);
		else if (operand_list->items[i].kind == OPS_FILE)
			handle_file(ctx, &operand_list->items[i]);
		i++;
	}
}

static void	handle_directory(t_ctx *ctx, t_operand_info *operand)
{
	list_directory(ctx, operand);
}

static void	handle_directories(t_ctx *ctx, t_operand_list *operand_list)
{
	size_t	i;

	i = 0;
	while (i < operand_list->count && ctx->fatal == FATAL_NONE)
	{
		if (operand_list->items[i].kind == OPS_DIRECTORY)
			handle_directory(ctx, &operand_list->items[i]);
		i++;
	}
	/* TODO coordinar el procesamiento de directorios, incluida la recursividad -R */
}

/*
 * [GNU ls] presenta los operandos no directorio antes de listar los directorios,
 * 			independientemente del orden de los argumentos
 */
void	handle_operands(t_ctx *ctx, t_operand_list *operand_list)
{
	if (ctx->fatal != FATAL_NONE)
		return ;
	sort_operands(ctx, operand_list);
	if (ctx->fatal != FATAL_NONE)
		return ;
	handle_non_directories(ctx, operand_list);
	if (ctx->fatal != FATAL_NONE)
		return ;
	handle_directories(ctx, operand_list);
}
