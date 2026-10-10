/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   clean.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/07 13:37:12 by gebravo           #+#    #+#             */
/*   Updated: 2026/10/10 11:10:10 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

/**
 * No libero individualmente ops->items[i], porque esos punteros apuntan a
 * cadenas de argv, que no he reservado yo.
 */
void	clean_all(t_ctx *ctx, t_operands *ops)
{
	(void)ctx;
	free(ops->items);
	ops->items = NULL;
	ops->count = 0;
}
