/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_ls.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/30 12:05:33 by gebravo           #+#    #+#             */
/*   Updated: 2026/10/08 16:19:36 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_LS_H
#define FT_LS_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>			
#include <time.h>

/**
 * t_opts agrupa el estado de las opciones (flags) obligatorias
 * del subject (Chapter V Mandatory Part).
 *
 * 1 = opción activada
 * 0 = opción desactivada
 *
 * Por ejemplo, al ejecutar:
 * ./ft_ls -la
 *
 * l     = 1
 * a     = 1
 * big_r = 0
 * r     = 0
 * t     = 0
 */
typedef struct s_opts
{
	int	l;
	int	big_r;				/* -R */
	int	a;
	int	r;
	int	t;
}	t_opts;

/**
 * t_argkind representa la clase de argumento de la línea de comandos.
 * Uso interno de parser.c.
 *
 * ARG_SHORT       (=0) -> opción corta: "-a", "-l", "-lat", etc.
 *
 * ARG_LONG        (=1) -> opción larga: "--algo".
 *                         Necesitamos reconocer estos argumentos como una
 *                         categoría distinta para poder rechazarlos
 *                         correctamente -> error.
 *
 * ARG_OPERAND     (=2) -> operando: "dir", "file", "-", "", etc.
 *
 * ARG_END_OPTIONS (=3) -> fin de las opciones: "--".
 *                         A partir de aquí, todo es operando.
 */
typedef enum e_argkind
{
	ARG_SHORT,
	ARG_LONG,
	ARG_OPERAND,
	ARG_END_OPTIONS
}	t_argkind;

/**
 * t_fatal representa los errores que obligan a detener el procesamiento.
 *
 * FATAL_NONE   (=0) -> no hay error fatal.
 * FATAL_OPTION (=1) -> opción de línea de comandos no válida.
 * FATAL_NOMEM  (=2) -> fallo al reservar memoria dinámica.
 * FATAL_WRITE  (=3) -> fallo al escribir la salida.
 *
 * IMPORTANTE: un error recuperable no significa que el error pueda
 * solucionarse. Significa que el error ha ocurrido, pero el programa
 * puede continuar realizando el resto del trabajo.
 *
 * Error recuperable -> permite continuar el procesamiento.
 * Error fatal       -> obliga a detener el procesamiento.
 */
typedef enum e_fatal
{
	FATAL_NONE,
	FATAL_OPTION,
	FATAL_NOMEM,
	FATAL_WRITE
}	t_fatal;

/**
 * Contexto de la ejecución.
 *
 * opts     qué opciones están activadas
 * *prog    nombre del programa, prefijo de los mensajes de error
 * invalid  apunta a la información de una opción no admitida:
 *          - opción corta: apunta al carácter inválido
 *          - opción larga: apunta al argumento completo
 *          No modifica la cadena original ni reserva memoria.
 * status   código con el que terminará el programa (0, 1 o 2)
 *			0 = ejecución correcta, sin errores
 *			1 = error general de ejecución
 *			2 = error grave o de uso, por ejemplo una opción no válida
 * printed  indica si ya hemos impreso algo anteriormente
 * headers  indica si debemos imprimir el nombre del directorio como cabecera
 * fatal    indica si ha ocurrido un error que obliga a detenerse
 * now      hora de referencia (se usará con -l)
 *
 * IMPORTANTE: status = 2 no significa necesariamente error fatal.
 * Puede haber un error recuperable, establecer status = 2 y continuar
 * procesando el resto de operandos.
 *
 * Acceso a los miembros:
 * - Si tenemos un puntero: t_ctx *ctx; -> ctx->prog
 *   (equivalente a (*ctx).prog)
 * - Si tenemos la struct:  t_ctx ctx;  -> ctx.prog
 */
typedef struct s_ctx
{
	t_opts			opts;
	const char		*prog;
	const char		*invalid;
	t_argkind		invalid_kind;
	int				status;
	int				printed;
	int				headers;
	t_fatal			fatal;
	struct timespec	now;

} t_ctx;

/**
 * Guardar los operandos en su orden
 * Durante el parseo, no debemos perder el orden en el que el usuario escribió
 * los operandos
 *
 * Ejemplo:
 * ./ft_ls -a dir2 fichero dir1
 *
 * ctx->opts	a = 1, resto = 0
 * ops			["dir2", "fichero", "dir1"]
 *
 * El contexto contiene el estado que necesita compartir el programa,
 * mientras que ops almacena el resultado del parseo relativo a los operandos:
 * cuáles escribió el usuario y en qué orden.
 */
typedef struct s_operands
{
	char	**items;
	int		count;
} t_operands;

/* init.c */
void	init_ctx(t_ctx *ctx, const char *prog);
void	init_ops(t_operands *ops);

/* parser.c */
int		parse_args(int argc, char **argv, t_ctx *ctx, t_operands *ops);

/* error.c */
void	report_fatal(t_ctx *ctx);

/* clean.c */
void	clean_all(t_ctx *ctx, t_operands *ops);

#endif
