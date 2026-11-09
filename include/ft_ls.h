/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_ls.h                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/09/30 12:05:33 by gebravo           #+#    #+#             */
/*   Updated: 2026/11/09 15:25:50 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef FT_LS_H
#define FT_LS_H

#include <stdio.h>
#include <unistd.h>																	/* readlink() */
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <stdint.h>
#include <time.h>
#include <sys/types.h>
#include <pwd.h>																	/* getpwuid() información de un usuario por su UID */
#include <grp.h>																	/* getgrgid() información de un grupo por su GID */
#include <sys/sysmacros.h>															/* major() y minor() */
#include <inttypes.h>																/* no existe un especificador de printf() específico para blkcnt_t -> '%jd' es una forma portable de imprimir un entero con signo como blkcnt_t, convirtiéndolo a intmax_t */
#include <stdbool.h>

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
	int	big_r;																		/* -R */
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
 * printed  indica si ya hemos impreso algo (un bloque en stdout) anteriormente
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
	bool			printed;
	bool			headers;
	t_fatal			fatal;
	int				error_errno;													/* errno asociado al error fatal de la ejecución */
	struct timespec	now;

}	t_ctx;

typedef struct s_operand_info
{
	const char		*name;
	t_operand_kind	kind;
	int				err;															/* errno asociado al error de este operando */
	struct stat		st;
	int				stat_ok;														/* 1: st válido; 0: lstat() no llamado o fallido */
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
 * Tipo             Campo       Significado                  Formato  Conversión
 * -----------------------------------------------------------------------------------
 * mode_t           st_mode     Tipo de archivo y permisos   %jo      (uintmax_t) octal sin signo
 * off_t            st_size     Tamaño lógico en bytes       %jd      (intmax_t) decimal con signo
 * dev_t            st_rdev     Identificador de dispositivo N/A      major() / minor()
 * struct timespec  st_mtim     Última modificación          N/A      localtime_r()
 * uid_t            st_uid      ID del propietario           %ju      (uintmax_t) decimal sin signo
 * gid_t            st_gid      ID del grupo                 %ju      (uintmax_t) decimal sin signo
 * nlink_t          st_nlink    Número de enlaces duros      %ju      (uintmax_t) decimal sin signo
 * blkcnt_t         st_blocks   Bloques de 512 B (Linux)     %jd      (intmax_t) decimal con signo
 *
 * %jd  -> Entero decimal con signo (intmax_t).
 * %ju  -> Entero decimal sin signo (uintmax_t).
 * %jo  -> Entero octal sin signo (uintmax_t).
 *
 * <stdint.h> define intmax_t y uintmax_t.
 * <inttypes.h> proporciona también las macros de formato PRI...
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
 * st_rdev es de tipo dev_t y representa un identificador
 * de dispositivo, no un tamaño en bytes.
 *
 * DIFERENCIA ENTRE st_size Y st_blocks
 *
 * st_size:
 * - Representa el tamaño lógico del archivo en bytes.
 * - Se utiliza para calcular el ancho de la columna de tamaño
 *   cuando se imprime el formato largo (-l).
 * - En archivos de dispositivo, ls -l muestra major, minor
 *   en lugar de st_size.
 *
 * st_blocks:
 * - Representa el espacio asignado en unidades de 512 bytes
 *   en Linux.
 * - Se utiliza para calcular el total que aparece al listar
 *   directorios con -l.
 *
 * Ejemplo:
 * Un archivo puede tener:
 * - st_size = 1000 bytes.
 * - st_blocks = 8 bloques de 512 bytes = 4096 bytes asignados.
 *
 * DISPOSITIVOS ESPECIALES: st_rdev, major() y minor()
 *
 * En Linux, algunos archivos representan dispositivos.
 * Su tipo se identifica mediante st_mode:
 *
 * - S_ISCHR(st_mode): dispositivo de caracteres (c).
 * - S_ISBLK(st_mode): dispositivo de bloques (b).
 *
 * Para estos archivos, ls -l no muestra st_size.
 * En su lugar, muestra dos números: major, minor.
 *
 * st_rdev:
 * - Es un campo de struct stat, de tipo dev_t.
 * - Contiene el identificador del dispositivo representado.
 * - No debe confundirse con st_dev, que identifica el
 *   dispositivo donde reside el propio archivo.
 *
 * major() y minor():
 * - No son campos de struct stat.
 * - Son macros que extraen los componentes de st_rdev en Linux.
 * - major(): identifica el controlador o grupo de dispositivos.
 * - minor(): identifica el dispositivo dentro de ese grupo.
 *
 * En Linux se declaran mediante:
 *
 * #include <sys/sysmacros.h>
 *
 * Ejemplo:
 *
 * ls -l /dev/null
 * crw-rw-rw- 1 root root 1, 3 Oct  8 13:53 /dev/null
 *
 * - c: dispositivo de caracteres.
 * - 1: major.
 * - 3: minor.
 *
 * En C:
 *
 * major(st->st_rdev)
 * minor(st->st_rdev)
 *
 * Para imprimir sus valores decimales podemos convertirlos
 * a uintmax_t y utilizar el formato %ju.
 *
 * IMPORTANTE PARA ft_ls:
 *
 * - Archivos normales: imprimir st_size.
 * - Directorios: imprimir st_size.
 * - Enlaces simbólicos: imprimir st_size del propio enlace.
 * - Dispositivos de caracteres: imprimir major, minor.
 * - Dispositivos de bloques: imprimir major, minor.
 *
 * La preparación de los anchos de -l debe contemplar:
 *
 * - El ancho máximo de los tamaños (st_size).
 * - El ancho máximo de major.
 * - El ancho máximo de minor.
 * - El ancho total necesario para representar major, minor.
 *
 * La columna compartida por tamaños e identificadores
 * debe tener espacio suficiente para ambos formatos.
 *
 * UTILIZACIÓN DE LOS METADATOS EN ft_ls
 *
 * -l: utiliza el tipo de archivo, permisos, número de enlaces,
 *     propietario, grupo, tamaño o identificador de dispositivo
 *     (major, minor) y fecha de modificación para imprimir
 *     el formato largo.
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
 * - path: ruta de la entrada que queremos consultar.
 * - buf: dirección de la estructura donde guardar los metadatos.
 * - Devuelve 0 si tiene éxito.
 * - Devuelve -1 si falla y establece errno.
 *
 * Para consultar una entrada de un directorio, construimos
 * su ruta mediante build_path().
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
 * TIPO DE ENTRADA: d_type
 *
 * readdir() devuelve un puntero a struct dirent, que contiene,
 * entre otros campos, el nombre (d_name) y el tipo (d_type).
 *
 * En Linux, d_type es de tipo unsigned char.
 * Es un valor entero sin signo, no un carácter de texto.
 *
 * VALORES HABITUALES DE d_type:
 *
 *   DT_REG     Archivo regular                 (-)
 *   DT_DIR     Directorio                      (d)
 *   DT_LNK     Enlace simbólico                (l)
 *   DT_CHR     Dispositivo de caracteres       (c)
 *   DT_BLK     Dispositivo de bloques          (b)
 *   DT_FIFO    Tubería con nombre (FIFO)       (p)
 *   DT_SOCK    Socket                          (s)
 *   DT_UNKNOWN Tipo desconocido                (?)
 *
 * Los caracteres entre paréntesis son los que representan
 * esos tipos en la primera posición del formato largo (-l).
 * No son los valores numéricos de las constantes DT_*.
 *
 * LIMITACIONES DE d_type:
 *
 * d_type no siempre permite conocer el tipo de entrada.
 * Algunos sistemas de archivos devuelven DT_UNKNOWN.
 *
 * Además, d_type no contiene permisos, propietario, grupo,
 * tamaño ni fechas. Para obtenerlos necesitamos lstat().
 *
 * ¿POR QUÉ GUARDAMOS d_type?
 *
 * readdir() puede encontrar una entrada, pero lstat() puede
 * fallar después, por ejemplo, si otro proceso elimina
 * el archivo entre ambas llamadas.
 *
 * Si lstat() falla, no disponemos de metadatos válidos en st,
 * pero podemos conservar el nombre y el tipo obtenido
 * previamente mediante readdir().
 *
 * Por eso guardamos d_type en t_directory_entry.
 *
 * No guardamos toda la estructura struct dirent:
 * únicamente copiamos los datos que necesitamos.
 *
 * - name: copia del nombre obtenido de d_name.
 * - d_type: copia del tipo indicado por readdir().
 *
 * Esto permite representar el tipo en ls -l cuando sea
 * conocido, aunque los demás campos sean desconocidos.
 *
 * Ejemplo:
 *
 *   name    = "archivo.txt"
 *   d_type  = DT_REG
 *   stat_ok = 0
 *
 * readdir() encontró "archivo.txt" y lo identificó como
 * archivo regular, pero lstat() no proporcionó sus metadatos.
 *
 * Podemos utilizar '-' como carácter de tipo y '?' para
 * representar los permisos y otros datos desconocidos.
 *
 * Si d_type == DT_UNKNOWN, tampoco conocemos el tipo
 * y utilizamos '?' en la primera posición.
 *
 * IMPORTANTE:
 *
 * d_type refleja la información obtenida por readdir().
 * Puede quedar desactualizado si la entrada cambia.
 *
 * Si lstat() tiene éxito, utilizamos st_mode para determinar
 * el tipo real observado por esa llamada.
 *
 * VALIDEZ DE LOS METADATOS: stat_ok
 *
 * stat_ok indica si hemos obtenido correctamente los
 * metadatos de una entrada mediante lstat().
 *
 * - stat_ok = 1:
 *   lstat() ha tenido éxito.
 *   entry->st contiene metadatos válidos.
 *   Podemos consultar sus campos y obtener el tipo de st_mode.
 *
 * - stat_ok = 0:
 *   lstat() todavía no se ha ejecutado o ha fallado.
 *   No podemos utilizar los campos de entry->st.
 *   Conservamos el nombre y d_type obtenidos de readdir().
 *
 * IMPORTANTE:
 *
 * stat_ok no indica si readdir() encontró la entrada.
 * Indica si lstat() consiguió obtener sus metadatos.
 *
 * Nunca debemos utilizar entry->st cuando stat_ok == 0.
 *
 * Esto afecta especialmente al cálculo de anchos de -l,
 * a la ordenación por fecha (-t) y a la recursividad (-R).
 *
 * En el formato largo (-l), si stat_ok == 0, podemos
 * utilizar d_type para representar el tipo cuando sea
 * conocido y '?' para los metadatos no disponibles.
 */
typedef struct s_directory_entry
{
	char			*name;
	struct stat		st;
	int				stat_ok;														/* 1: st válido; 0: lstat() no llamado o fallido */
	unsigned char	d_type;															/* tipo devuelto por readdir(); DT_UNKNOWN si se desconoce */
}	t_directory_entry;

typedef struct s_directory_list
{
	const char			*directory_name;
	t_directory_entry	*items;
	size_t				count;      												/* entradas almacenadas */
	size_t				capacity;   												/* espacio reservado, en número de entradas */
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
	size_t	nlink;																	/* ancho máximo de st_nlink (número de hard links) */
	size_t	size;																	/* ancho máximo de st_size, en caracteres */
	size_t	owner;																	/* ancho máximo del propietario (st_uid -> getpwuid()) */
	size_t	group;																	/* ancho máximo del grupo (st_gid -> getgrgid()) */
	size_t	major;																	/* ancho máximo del número major (inicialmente sus dígitos máximos; después incluye el relleno izquierdo para alinearlo con st_size) - identifica el controlador o tipo de dispositivo gestionado por el kernel */
	size_t	minor;																	/* ancho máximo del número minor - identifica el dispositivo concreto o la unidad dentro de los gestionados por ese controlador */
}	t_print_widths;



/* init.c */
void	init_ctx(t_ctx *ctx, const char *prog);
void	init_ops(t_operands *ops);
void	init_operand_list(t_operand_list *operand_list);
void	init_long_format_time(t_ctx *ctx);
void	init_directory_list(const char *directory_name, t_directory_list *directory_list);
void	init_print_widths(t_print_widths *print_widths);

/* parser.c */
int		parse_args(int argc, char **argv, t_ctx *ctx, t_operands *ops);

/* run.c */
void	run_ls(t_ctx *ctx, t_operands *ops);

/* operands_classify.c */
void	classify_operands(t_ctx *ctx, t_operands *ops, t_operand_list *operand_list);

/* operands_handle.c */
/* nivel de operandos-recorrer operand_list
   e identificar los operandos OPS_ERROR, OPS_FILES, OPS_DIRECTORY */
void	handle_operands(t_ctx *ctx, t_operand_list *operand_list);

/* directory_entries.c */
/* nivel de directorio individual-ciclo de vida de un directorio concreto */
void	list_directory(t_ctx *ctx, const char *directory_name);

/* load_directory_entries.c (open, read et store)*/
int		load_directory_entries(t_ctx *ctx, const char *name, t_directory_list *directory_list);

/* display_directory_entries.c */
/* ordena y coordina la presentación de las entradas */
void	handle_directory_entries(t_ctx *ctx, t_directory_list *directory_list);

/* directory_entries_utils.c */
int		calculate_capacity(size_t capacity, size_t *new_capacity);
int		load_stat(t_ctx *ctx, const char *directory_name, t_directory_entry *directory_entry);
int		ensure_capacity(t_directory_list *directory_list);

/* sort_directory.c */
void	sort_directory_entries(t_ctx *ctx, t_directory_list *directory_list);

/* sort_operand.c */
void	sort_operands(t_ctx *ctx, t_operand_list *operand_list);

/* long_format.c */
/* fase 1: preparación del listado */
void	update_print_widths(const struct stat *st, t_print_widths *widths);
void	prepare_size_widths(t_print_widths *widths);
/* fase 2: impresión del formato largo */
/* 2a.- si metadatos (st) válidos - imprime utilizando los metadatos de lstat() */
void	print_long_format(t_ctx *ctx, const char *entry_name, const char *path, const struct stat *st, const t_print_widths *widths);
/* 2b.- si metadatos (st) NO válidos - d_type y ? para los metadatos desconocidos */
void	print_unknown_long_format(const char *entry_name, unsigned char d_type, const t_print_widths *widths);

/* long_format_utils.c */
size_t	count_digits(size_t value);
void	update_max_width(size_t width, size_t *max_width);
void	format_file_mode(mode_t mode, char permissions[11]);
void	print_file_owner(const struct stat *st, size_t width);
void	print_file_group(const struct stat *st, size_t width);
void	format_file_time(const struct stat *st,const struct timespec *now, char date[32]);
char	*read_link_target(t_ctx *ctx, const char *path, const struct stat *st);

/* error.c */
void	report_fatal(t_ctx *ctx);

/* clean.c */
void	clean_all(t_ctx *ctx, t_operands *ops);
void	clean_directory_list(t_directory_list *list);

/* utils.c */
char	*build_path(const char *name_directory, const char *name_entry);

#endif
