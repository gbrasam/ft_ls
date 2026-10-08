/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/02 08:43:05 by gebravo           #+#    #+#             */
/*   Updated: 2026/10/08 14:50:45 by gebravo          ###   ########.fr       */
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
