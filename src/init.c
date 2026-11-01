/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/02 08:43:05 by gebravo           #+#    #+#             */
/*   Updated: 2026/11/01 12:55:18 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

/**
 * Inicializa el contexto general
 *
 * void *memset(void *s, int c, size_t n);
 * s -> dirección donde empezar
 * c -> valor con el que rellenar
 * n -> número de bytes que queremos rellenar con ese valor
 *
 * memset(ctx, 0, sizeof(*ctx)) pone a 0 todos los bytes de la
 * estructura t_ctx a la que apunta ctx.
 *
 * sizeof(*ctx) obtiene el tamaño de la estructura t_ctx, no el
 * tamaño del puntero ctx.
 *
 * Al hacer ctx->prog = prog se copia la dirección almacenada en prog,
 * no el contenido almacenado en la dirección a la que apunta.
 *
 * prog ---------+
 *               +--> "./ft_ls\0"
 * ctx->prog ----+
 */
void	init_ctx(t_ctx *ctx, const char *prog)
{
	memset(ctx, 0, sizeof(*ctx));
	ctx->prog = prog;
	ctx->invalid = NULL;
	ctx->fatal = FATAL_NONE;
}

/**
 * Inicializa la estructura que almacenará los operandos
 * obtenidos durante el parseo
 */
void	init_ops(t_operands *ops)
{
	ops->items = NULL;
	ops->count = 0;
}

void	init_operand_list(t_operand_list *operand_list)
{
	operand_list->items = NULL;
	operand_list->count = 0;
}

void	init_directory_list(t_directory_list *directory_list)
{
	directory_list->items = NULL;
	directory_list->count = 0;
	directory_list->capacity = 0;
}

void	init_print_widths(t_print_widths *print_widths)
{
	print_widths->nlink = 0;
	print_widths->owner = 0;
	print_widths->group = 0;
	print_widths->size = 0;
}
