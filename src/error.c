/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   error.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/02 15:08:11 by gebravo           #+#    #+#             */
/*   Updated: 2026/11/10 10:50:08 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

/* 
 * Informar de errores que impiden continuar
 *
 * report_fatal()
 * ├── fflush(stdout)
 * ├── Imprimir mensaje fatal
 * └── Establecer ctx->status = 2
 * 
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
	fflush(stdout);
	ctx->status = 2;
	if (ctx->fatal == FATAL_OPTION)
	{
		if (ctx->invalid_kind == ARG_LONG)
			fprintf(stderr, "%s: unrecognized option '%s'\n", ctx->prog, ctx->invalid);
		else
			fprintf(stderr, "%s: invalid option -- '%c'\n", ctx->prog, ctx->invalid[0]);
		
		fprintf(stderr, "Try '%s --help' for more information.\n", ctx->prog);
	}
	else if (ctx->fatal == FATAL_NOMEM)
		fprintf(stderr, "%s: memory exhausted\n", ctx->prog);
	else if (ctx->fatal == FATAL_CLOCK)
		fprintf(stderr, "%s: clock_gettime: %s\n", ctx->prog, strerror(ctx->error_errno));
	/*
	else if (ctx->fatal == FATAL_WRITE)
	{
		TODO
	}
	*/
}

/*
 * Informa de errores que permiten continuar la ejecución
 *
 * fflush(stdout) envía los datos pendientes de stdout a su destino
 * (terminal, archivo, pipe, etc.) antes de imprimir el error en stderr
 * Así se respeta el orden de salida cuando ambos comparten destino,
 * siempre que fflush() termine correctamente
 *
 * error_errno conserva el error original, ya que fflush()
 * podría modificar errno.
 *
 * 'serious' indica la gravedad del error, no su origen
 * Puede recibir 'is_operand' cuando el error afecta a la apertura,
 * lectura o cierre de un directorio:
 * - true:  directorio recibido como operando (gravedad 2)
 * - false: subdirectorio descubierto durante -R (gravedad 1)
 * En otros errores, la gravedad se determina directamente:
 * - Error de un operando: gravedad 2
 * - Error de lstat() de una entrada o readlink(): gravedad 1
 * Un error de gravedad 1 nunca rebaja un status 2 anterior
 */
void	report_error(t_ctx *ctx, bool serious, const char *error_message, const char *path, int error_errno)
{
	fflush(stdout);
	fprintf(stderr, "%s: %s '%s': %s\n", ctx->prog, error_message, path, strerror(error_errno));
	if (serious)
		ctx->status = 2;
	else if (ctx->status != 2)
		ctx->status = 1;
}
