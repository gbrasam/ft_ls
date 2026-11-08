/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/02 08:21:45 by gebravo           #+#    #+#             */
/*   Updated: 2026/11/08 13:10:10 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

int	main(int argc, char **argv)
{
	t_ctx		ctx;
	t_operands	ops;

	init_ctx(&ctx, argv[0]);
	if (parse_args(argc, argv, &ctx, &ops) == 0)
		run_ls(&ctx, &ops);
	if (ctx.fatal != FATAL_NONE)
		report_fatal(&ctx);
	clean_all(&ctx, &ops);
	return (ctx.status);
}
