/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/02 08:21:45 by gebravo           #+#    #+#             */
/*   Updated: 2026/11/10 12:30:58 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

/*
 * Inicializa el contexto, procesa los argumentos y ejecuta el listado
 * si no se ha producido un error durante el análisis de opciones.
 *
 * Antes de finalizar, comprueba explícitamente los errores de stdout.
 * Esto es necesario porque la salida puede permanecer en un búfer
 * y una escritura pendiente podría fallar al terminar el programa,
 * cuando ya no sería posible actualizar el código de salida.
 *
 * Los errores fatales se comunican por stderr antes de liberar
 * los recursos y devolver el estado final de ejecución.
 *
 * El estado de salida se conserva en ctx.status:
 * - 0: ejecución correcta.
 * - 1: errores no graves durante el listado.
 * - 2: errores graves o fatales.
 */
int	main(int argc, char **argv)
{
	t_ctx		ctx;
	t_operands	ops;

	init_ctx(&ctx, argv[0]);
	if (parse_args(argc, argv, &ctx, &ops) == 0)
		run_ls(&ctx, &ops);
	check_stdout_error(&ctx);
	if (ctx.fatal != FATAL_NONE)
		report_fatal(&ctx);
	clean_all(&ctx, &ops);
	return (ctx.status);
}
