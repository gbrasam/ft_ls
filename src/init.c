/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   init.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/02 08:43:05 by gebravo           #+#    #+#             */
/*   Updated: 2026/11/08 15:25:08 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

/**
 * Inicializa el contexto general
 * Se ejecuta antes de procesar los operandos
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

/**
 * Inicializa ctx->now únicamente cuando está activada la opción -l.
 *
 * Se consulta el reloj una sola vez para disponer de una referencia
 * temporal común durante toda la ejecución.
 *
 * Con -l, ctx->now se comparará con st_mtim para decidir si mostramos
 * la hora o el año de modificación de cada archivo.
 *
 * Funcionamiento:
 * 1. Si -l no está activado, salir sin consultar el reloj.
 * 2. Si -l está activado, obtener la fecha actual.
 * 3. Si falla la consulta, registrar el error e impedir continuar.
 *
 * clock_gettime(clockid_t clockid, struct timespec *tp):
 * - Obtiene el tiempo del reloj indicado y lo guarda en *tp.
 * - Devuelve 0 si tiene éxito y -1 si falla (establece errno).
 *
 * CLOCK_REALTIME:
 * - Representa la fecha y hora del sistema desde la época Unix.
 * - Puede cambiar si se ajusta el reloj del sistema.
 * - Permite comparar ctx->now con st_mtim.
 *
 * CLOCK_MONOTONIC:
 * - Se utiliza para medir intervalos de tiempo.
 * - No representa fechas de calendario.
 * - No sufre saltos por cambios manuales de la hora del sistema.
 *
 * &ctx->now proporciona la dirección de la estructura timespec
 * donde clock_gettime() guardará segundos y nanosegundos.
 */
void	init_long_format_time(t_ctx *ctx)
{
	if (!ctx->opts.l)
		return ;
	if (clock_gettime(CLOCK_REALTIME, &ctx->now) != 0)
	{
		ctx->error_errno = errno;
		ctx->fatal = FATAL_CLOCK;
	}
}

void	init_directory_list(const char *directory_name, t_directory_list *directory_list)
{
	directory_list->directory_name = directory_name;
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
