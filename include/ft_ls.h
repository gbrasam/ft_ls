/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_ls.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/30 12:05:33 by gebravo           #+#    #+#             */
/*   Updated: 2026/10/29 13:05:11 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_LS_H
#define FT_LS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <stdint.h>
#include <time.h>

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

typedef enum e_operand_kind
{
	OPS_FILE,
	OPS_DIRECTORY,
	OPS_ERROR
}	t_operand_kind;

/**
 * t_fatal representa los errores que obligan a detener el procesamiento.
 *
 * FATAL_NONE   (=0) -> no hay error fatal
 * FATAL_OPTION (=1) -> opción de línea de comandos no válida
 * FATAL_NOMEM  (=2) -> fallo al reservar memoria dinámica
 * FATAL_WRITE  (=3) -> fallo al escribir la salida
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
	int	big_r;									/* -R */
	int	a;
	int	r;
	int	t;
}	t_opts;

/**
 * Contexto de la ejecución.
 *
 * opts     qué opciones están activadas
 * *prog    nombre del programa, prefijo de los mensajes de error
 * invalid  apunta a la información de una opción no admitida:
 *          - opción corta: apunta al carácter inválido
 *          - opción larga: apunta al argumento completo
 *          No modifica la cadena original ni reserva memoria.
 * status (*) código con el que terminará el programa (0, 1 o 2)
 *			0 = ejecución correcta, sin errores
 *			1 = problema menor, p. ej. no poder acceder a un subdirectorio con -R
 *			2 = problema grave: opción no válida, operando de argv inaccesible, sin memoria
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
 * 
 * (*) status
 * GNU ls distingue la gravedad de los errores:
 *
 * - status = 0: no se han producido errores.
 * - status = 1: error menor al procesar una entrada encontrada dentro
 *   de un directorio (por ejemplo, lstat() falla sobre una entrada
 *   obtenida mediante readdir()).
 * - status = 2: error grave, por ejemplo, cuando no se puede acceder
 *   a un operando solicitado directamente en argv.
 *
 * ctx->status es global a toda la ejecución de ft_ls, no pertenece
 * a cada operando ni a cada entrada del directorio.
 *
 * Por tanto, debe conservar el error de mayor gravedad encontrado:
 * si ya vale 2 por un error anterior, un error menor posterior
 * no debe reducirlo a 1.
 *
 * Solo establecemos status = 1 cuando su valor actual es 0.
 * Si ya vale 1 o 2, lo mantenemos.
 *
 * El fallo de lstat() no es fatal para el procesamiento:
 * conservamos el nombre de la entrada, dejamos stat_ok = 0
 * y continuamos con las demás entradas del directorio.
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

}	t_ctx;

typedef struct s_operand_info
{
	const char		*name;
	t_operand_kind	kind;
	int				err;						/* errno */
	struct stat		st;
	int				stat_ok;					/* flag 1: st válido; 0: stat() no llamado o fallido */
}	t_operand_info;

typedef struct s_operand_list
{
	t_operand_info	*items;
	size_t			count;
}	t_operand_list;

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
	size_t	count;
}	t_operands;

/*
 * st: estructura donde se almacenan los metadatos del archivo
 * st_mode		tipo de archivo y permisos
 * st_size		tamaño en bytes
 * st_mtim		fecha y hora de última modificación, en Linux
 * st_uid		identificador del propietario
 * st_gid		identificador del grupo
 * st_nlink		número de enlaces duros
 * st_blocks	bloques de 512 bytes asignados
 * 
 * esta información será necesaria para implementar las opciones
 * -l: necesita permisos, propietario, grupo, tamaño, fecha, etc
 * -t: necesita la fecha de modificación para ordenar
 * -R: usa S_ISDIR(st.st_mode) de la entrada para decidir si desciende
 *
 * rellenamos st mediante lstat()
 * lstat(path, &entry->st); path = build_path(directorio, entry->name), p. ej. "src/clean.c"
 * si lstat() tiene éxito, devuelve 0
 * si falla, devuelve -1 y establece errno
 * 
 * stat_ok: ¿hemos obtenido correctamente los metadatos?
 * flag stat_ok = 1: lstat() ha tenido éxito y st contiene datos válidos
 * flag stat_ok = 0: todavía no se han solicitado los metadatos o lstat() ha fallado
 */
typedef struct s_directory_entry
{
	char		*name;
	struct stat	st;
	int			stat_ok;						/* flag 1: st válido; 0: lstat() no llamado o fallido */
}	t_directory_entry;

typedef struct s_directory_list
{
	t_directory_entry	*items;
	size_t				count;      			/* entradas almacenadas */
	size_t				capacity;   			/* espacio reservado, en número de entradas */
}	t_directory_list;


/* init.c */
void	init_ctx(t_ctx *ctx, const char *prog);
void	init_ops(t_operands *ops);
void	init_operand_list(t_operand_list *operand_list);
void	init_directory_list(t_directory_list *list);

/* parser.c */
int		parse_args(int argc, char **argv, t_ctx *ctx, t_operands *ops);

/* run.c */
void	run_ls(t_ctx *ctx, t_operands *ops);

/* operands_classify.c */
void	classify_operands(t_ctx *ctx, t_operands *ops, t_operand_list *operand_list);

/* operands_handle.c */
void	handle_operands(t_ctx *ctx, t_operand_list *operand_list);

/* directory.c */
void	list_directory(t_ctx *ctx, const char *path);

/* directory_entries.c */
int		store_entry(t_ctx *ctx, const char *name_directory, const char *name_entry, t_directory_list *list);

/* sort.c */
void	sort_operands(t_ctx *ctx, t_operand_list *operand_list);

/* error.c */
void	report_fatal(t_ctx *ctx);

/* clean.c */
void	clean_all(t_ctx *ctx, t_operands *ops);
void	clean_directory_list(t_directory_list *list);

/* utils.c */
char	*build_path(const char *name_directory, const char *name_entry);

#endif
