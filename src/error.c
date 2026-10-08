/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   error.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/02 15:08:11 by gebravo           #+#    #+#             */
/*   Updated: 2026/10/08 14:16:32 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

/* 
 * [GNU] 'ctx->prog' conserva argv[0] para reproducir el prefijo
 *       utilizado en los mensajes de error.
 * root@debian-4gb-ft-linux-x86:~# LC_ALL=C ls -j 1>/tmp/ls_out 2>/tmp/ls_err; printf 'status=%d\n' "$?"; cat /tmp/ls_err
 * status=2
 * ls: invalid option -- 'j'
 * Try 'ls --help' for more information.
 * 
 * Da igual que antes de j haya opciones válidas:
 * root@debian-4gb-ft-linux-x86:~# LC_ALL=C ls -arjlt 1>/tmp/ls_out 2>/tmp/ls_err; printf 'status=%d\n' "$?"; cat /tmp/ls_err
 * status=2
 * ls: invalid option -- 'j'
 * Try 'ls --help' for more information.
 */
void	report_fatal(t_ctx *ctx)
{
	if (ctx->fatal == FATAL_OPTION)
	{
		ctx->status = 2;
		fprintf(stderr, "%s: invalid option -- '%c'\n", ctx->prog, ctx->invalid_option);
		fprintf(stderr, "Try 'ls --help' for more information.\n");
	}
	/*
	else if (ctx->fatal == FATAL_NOMEM)
	{

	}
	else if (ctx->fatal == FATAL_WRITE)
	{
	
	}
	*/
}
