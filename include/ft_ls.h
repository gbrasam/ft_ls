/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_ls.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/30 12:05:33 by gebravo           #+#    #+#             */
/*   Updated: 2026/11/02 16:50:08 by gebravo          ###   ########.fr       */
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
#include <sys/types.h>
#include <pwd.h>													/* getpwuid() información de un usuario por su UID */
#include <grp.h>													/* getgrgid() información de un grupo por su GID */
#include <sys/sysmacros.h>											/* major() y minor() */

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
 * t_fatal identifica los errores que obligan a detener el procesamiento.
 *
 * FATAL_NONE   (=0) -> no hay error fatal
 * FATAL_OPTION (=1) -> opción de línea de comandos no válida
 * FATAL_NOMEM  (=2) -> fallo al reservar memoria dinámica
 * FATAL_CLOCK  (=3) -> fallo al consultar el reloj del sistema
 * FATAL_WRITE  (=4) -> fallo al escribir la salida (pendiente de implementar)
 *
 * DIFERENCIA ENTRE ERRORES RECUPERABLES Y FATALES
 *
 * Error recuperable:
 * - El error ha ocurrido, pero podemos continuar procesando.
 * - No establece ctx->fatal.
 * - Actualiza ctx->status según su gravedad (1 o 2).
 *
 * Ejemplos:
 * - lstat() falla sobre una entrada encontrada con readdir():
 *   status = 1; continuamos con las demás entradas.
 * - Un operando de argv no existe:
 *   status = 2; continuamos con los demás operandos.
 *
 * Error fatal:
 * - Impide continuar el procesamiento normal.
 * - Establece ctx->fatal con la causa del error.
 * - report_fatal() establece status = 2 y muestra el mensaje.
 * - Fallo al consultar el reloj: FATAL_CLOCK.
 *
 * Ejemplos:
 * - Opción no válida: FATAL_OPTION.
 * - Fallo de memoria: FATAL_NOMEM.
 * - Fallo al escribir la salida: FATAL_WRITE (pendiente).
 *
 * IMPORTANTE:
 * status = 2 no implica necesariamente un error fatal.
 * ctx->status indica la gravedad del error.
 * ctx->fatal indica si debemos detenernos y por qué.
 */
typedef enum e_fatal
{
	FATAL_NONE,
	FATAL_OPTION,
	FATAL_NOMEM,
	FATAL_CLOCK,
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
	int	big_r;														/* -R */
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
 * now      fecha y hora actual de referencia (struct timespec).
 *          Se inicializa una sola vez en init_long_format_time(),
 *          únicamente con -l, para decidir cómo mostrar la fecha
 *          de modificación de los archivos.
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
	int				error_errno;									/* errno asociado al error fatal de la ejecución */
	struct timespec	now;

}	t_ctx;

typedef struct s_operand_info
{
	const char		*name;
	t_operand_kind	kind;
	int				err;											/* errno asociado al error de este operando */
	struct stat		st;
	int				stat_ok;										/* 1: st válido; 0: lstat() no llamado o fallido */
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
 * METADATOS DE UNA ENTRADA DE DIRECTORIO: struct stat
 *
 * st es una estructura que almacena los metadatos del archivo.
 * Cuando una función recibe const struct stat *st, recibe
 * un puntero a esa estructura y no puede modificarla a través de él.
 *
 * PRINCIPALES CAMPOS DE struct stat (LINUX)
 *
 * Tipo             Campo       Significado
 * --------------------------------------------------------------
 * mode_t           st_mode     Tipo de archivo y permisos.
 * off_t            st_size     Tamaño lógico en bytes.
 * struct timespec  st_mtim     Fecha y hora de última modificación
 *                             (Linux).
 * uid_t            st_uid      Identificador del propietario.
 * gid_t            st_gid      Identificador del grupo.
 * nlink_t          st_nlink    Número de enlaces duros.
 * blkcnt_t         st_blocks   Bloques de 512 bytes asignados
 *                             (Linux).
 *
 * TIPOS DE DATOS Y CÁLCULO DE DÍGITOS
 *
 * Aunque los campos tengan tipos diferentes, podemos utilizar
 * una misma función para contar los dígitos de sus valores
 * enteros no negativos, siempre que la conversión a size_t
 * sea adecuada.
 *
 * st_size es de tipo off_t, normalmente con signo.
 * Si su valor fuera negativo, habría que tratarlo expresamente
 * antes de convertirlo a size_t.
 *
 * st_mtim es una estructura timespec, no un número entero.
 * Su tratamiento es diferente al de los campos numéricos.
 *
 * DIFERENCIA ENTRE st_size Y st_blocks
 *
 * st_size:
 * - Representa el tamaño lógico del archivo en bytes.
 * - Se utiliza para calcular el ancho de la columna de tamaño
 *   cuando se imprime el formato largo (-l).
 *
 * st_blocks:
 * - Representa el espacio asignado en unidades de 512 bytes
 *   en Linux.
 * - Se utilizará posteriormente para calcular el total
 *   que aparece al listar directorios con -l.
 *
 * Ejemplo:
 * Un archivo puede tener:
 * - st_size = 1000 bytes.
 * - st_blocks = 8 bloques de 512 bytes = 4096 bytes asignados.
 *
 * UTILIZACIÓN DE LOS METADATOS EN ft_ls
 *
 * -l: utiliza el tipo de archivo, permisos, número de enlaces,
 *     propietario, grupo, tamaño y fecha de modificación
 *     para imprimir el formato largo.
 *
 * -t: utiliza st_mtim para ordenar los archivos según
 *     su fecha y hora de última modificación.
 *
 * -R: utiliza S_ISDIR(st.st_mode) para identificar directorios
 *     y decidir si debe descender recursivamente, teniendo
 *     en cuenta las reglas de seguimiento de enlaces simbólicos.
 *
 * OBTENCIÓN DE LOS METADATOS: lstat()
 *
 * Los metadatos de cada entrada se obtienen mediante lstat().
 *
 * lstat(const char *path, struct stat *buf):
 * - path: ruta completa de la entrada que queremos consultar.
 * - buf: dirección de la estructura donde guardar los metadatos.
 * - Devuelve 0 si tiene éxito.
 * - Devuelve -1 si falla y establece errno.
 *
 * Para consultar una entrada de un directorio, construimos
 * su ruta completa mediante build_path().
 *
 * Ejemplo:
 * - Directorio: "src"
 * - Nombre de la entrada: "clean.c"
 * - Ruta construida: "src/clean.c"
 *
 * path = build_path(directorio, entry->name);
 * lstat(path, &entry->st);
 *
 * IMPORTANTE:
 * - lstat() consulta los metadatos del propio archivo.
 * - Si la entrada es un enlace simbólico, no sigue el enlace
 *   hasta su destino.
 * - Si lstat() funciona, entry->st contiene los metadatos
 *   del propio enlace simbólico, no los de su destino.
 * - La memoria reservada por build_path() debe liberarse
 *   mediante free(path) cuando ya no se necesite.
 *
 * DIFERENCIA ENTRE LAS ENTRADAS Y LOS OPERANDOS
 *
 * Las entradas de directorio se consultan mediante lstat().
 *
 * Los operandos recibidos por argv tienen una clasificación
 * diferente, implementada en get_operand_info(), que utiliza
 * stat() y lstat() según el caso.
 *
 * Por tanto, no debemos confundir la obtención de metadatos
 * de una entrada con la clasificación de los operandos.
 *
 * VALIDEZ DE LOS METADATOS: stat_ok
 *
 * stat_ok indica si los metadatos de la entrada son válidos.
 *
 * - stat_ok = 1:
 *   lstat() ha tenido éxito y st contiene datos válidos.
 *
 * - stat_ok = 0:
 *   Todavía no se ha llamado a lstat() o la llamada ha fallado.
 *
 * IMPORTANTE:
 * Nunca debemos utilizar los campos de entry->st
 * cuando entry->stat_ok == 0.
 *
 * Esto afecta especialmente al cálculo de anchos de -l,
 * al ordenamiento por fecha (-t) y a la recursividad (-R).
 */
typedef struct s_directory_entry
{
	char		*name;
	struct stat	st;
	int			stat_ok;											/* 1: st válido; 0: lstat() no llamado o fallido */
}	t_directory_entry;

typedef struct s_directory_list
{
	t_directory_entry	*items;
	size_t				count;      								/* entradas almacenadas */
	size_t				capacity;   								/* espacio reservado, en número de entradas */
}	t_directory_list;

/*
 * Almacena los anchos máximos de las columnas del formato largo (-l).
 *
 * Solo se almacenan los anchos de las columnas cuya longitud puede
 * variar entre archivos: enlaces, propietario, grupo y tamaño.
 * Las columnas de ancho fijo (tipo, permisos y fecha) y el nombre
 * del archivo no necesitan este cálculo.
 *
 * Los anchos se calculan por listado, antes de imprimir sus entradas,
 * para mantener las columnas alineadas.
 *
 * - Operandos OPS_FILE: se calculan los anchos de los archivos
 *   recibidos como argumentos, sin mostrar la línea "total".
 *
 * - Entradas de directorio (operandos OPS_DIRECTORY):
 *   se calculan los anchos y, por separado, el total de bloques
 *   de las entradas que se van a mostrar.
 *   Cada directorio tiene sus propios anchos y su propio total.
 *
 * Ejemplo: ls -l src/
 *
 * total 16
 * -rw-r--r-- 1 ana staff  800 oct 31 12:00 main.c
 * -rw-r--r-- 1 ana staff 2500 oct 30 10:00 utils.c
 *
 * "total" no representa el número de archivos ni la suma de st_size,
 * sino los bloques de disco asignados, calculados a partir de st_blocks.
 * En Linux, st_blocks se expresa normalmente en unidades de 512 bytes;
 * GNU ls suele mostrar el total en unidades de 1024 bytes por defecto,
 * aunque puede variar según la configuración.
 *
 * Al listar archivos directamente (ls -l main.c utils.c), no se
 * muestra "total", porque no se está listando un directorio.
 * 
 * st->st_size → tamaño real del archivo, expresado en bytes para archivos regulares
 * widths.size → número máximo de caracteres necesarios para imprimir ese tamaño dentro del listado
 * st_size = 800	→ 3 caracteres widths.size = 3;
 * st_size = 2500	→ 4 caracteres widths.size = 4;
 * Esto permite reservar 'size' posiciones para la columna del tamaño
 * y alinear los números a la derecha
 * 
 * IMPORTANTE: El ancho se refiere al número de caracteres de la
 * representación impresa, no al tamaño del dato en memoria.
 *
 * Por ejemplo, st_nlink puede imprimirse como 1, 12 o 125
 * (1, 2 o 3 caracteres), aunque su tipo ocupe un tamaño fijo
 * en memoria. Lo mismo ocurre con st_size.
 *
 * Por eso utilizamos size_t: cada campo almacena una cantidad
 * de caracteres, no el valor original del metadato.
 */
typedef struct s_print_widths
{
	size_t	nlink;													/* ancho máximo de st_nlink (número de hard links) */
	size_t	size;													/* ancho máximo de la columna st_size, en caracteres */
	size_t	owner;													/* ancho máximo del propietario (st_uid -> getpwuid()) */
	size_t	group;													/* ancho máximo del grupo (st_gid -> getgrgid()) */
}	t_print_widths;



/* init.c */
void	init_ctx(t_ctx *ctx, const char *prog);
void	init_ops(t_operands *ops);
void	init_operand_list(t_operand_list *operand_list);
void	init_long_format_time(t_ctx *ctx);
void	init_directory_list(t_directory_list *directory_list);
void	init_print_widths(t_print_widths *print_widths);

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
int		store_entry(t_ctx *ctx, const char *name_directory, const char *name_entry, t_directory_list *directory_list);

/* sort_directory.c */
void	sort_directory_entries(t_ctx *ctx, t_directory_list *directory_list);

/* sort_operand.c */
void	sort_operands(t_ctx *ctx, t_operand_list *operand_list);

/* long_format.c */
/* fase 1: preparación del listado */
void	update_print_widths(const struct stat *st, t_print_widths *widths);
/* fase 2: impresión del formato largo */
void	print_long_format(t_ctx *ctx, const char *name, const char *path, const struct stat *st, const t_print_widths *widths);

/* error.c */
void	report_fatal(t_ctx *ctx);

/* clean.c */
void	clean_all(t_ctx *ctx, t_operands *ops);
void	clean_directory_list(t_directory_list *list);

/* utils.c */
char	*build_path(const char *name_directory, const char *name_entry);

#endif
