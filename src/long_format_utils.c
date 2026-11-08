/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   long_format_utils.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/31 12:41:10 by gebravo           #+#    #+#             */
/*   Updated: 2026/11/08 18:25:18 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

/*
 * Cuenta los dígitos decimales de un entero sin signo.
 *
 * Recibe:
 * - value: número que queremos representar en decimal (size_t)
 *
 * Devuelve:
 * - Número de caracteres necesarios para representar sus dígitos,
 *   sin incluir espacios de alineación
 * 
 * Se inicializa digits a 1, porque incluso 0 ocupa un dígito
 *
 * Ejemplo:
 * value = 125
 * 125 -> 12 -> 1
 * digits = 3
 *
 * Los campos de t_print_widths son size_t porque representan
 * anchos de columnas, es decir, cantidades de caracteres
 *
 * count_digits() permite calcular el ancho de valores numéricos:
 * - st_nlink: número de enlaces duros
 * - st_size: tamaño en bytes, si es no negativo
 * - st_uid: identificador numérico si getpwuid() no encuentra usuario
 * - st_gid: identificador numérico si getgrgid() no encuentra grupo
 *
 * IMPORTANTE:
 * - Los campos originales de struct stat tienen tipos propios
 * - Solo se convierten a size_t cuando la conversión es adecuada
 * - Esta función no contempla números negativos ni su signo '-'
 * - No modifica los metadatos originales: value es una copia
 * - No actualiza t_print_widths; solo devuelve un número
 *
 * Las funciones update_*_width() compararán el resultado
 * con el ancho máximo acumulado y lo actualizarán si es mayor
 */
size_t	count_digits(size_t value)
{
	size_t	digits;

	digits = 1;
	while (value >= 10)
	{
		value /= 10;
		digits++;
	}
	return (digits);
}

/*
 * Recibe un ancho ya calculado y actualiza el máximo
 */
void	update_max_width(size_t width, size_t *max_width)
{
	if (width > *max_width)
		*max_width = width;
}

/*
 * Transformar los bits de st_mode en los 10 caracteres que representan el tipo y permisos
 *
 * IDENTIFICACIÓN DEL TIPO DE ARCHIVO (st_mode):
 *
 * Tipos de archivo (primer carácter de ls -l):
 *
 * '-'  Archivo regular           S_ISREG(mode)
 * 'd'  Directorio                S_ISDIR(mode)
 * 'l'  Enlace simbólico          S_ISLNK(mode)
 * 'c'  Dispositivo de caracteres S_ISCHR(mode)
 * 'b'  Dispositivo de bloques    S_ISBLK(mode)
 * 'p'  FIFO (tubería con nombre) S_ISFIFO(mode)
 * 's'  Socket                    S_ISSOCK(mode)
 * '?'  Tipo desconocido          Valor por defecto
 *
 * El primer carácter de la cadena de permisos indica el tipo
 * de archivo. Los nueve caracteres restantes indican los permisos.
 *
 * Ejemplos:
 *   -rw-r--r--  -> Fichero regular.
 *   drwxr-xr-x  -> Directorio.
 *   lrwxrwxrwx  -> Enlace simbólico.
 *
 * Estas macros comprueban el tipo codificado en st_mode.
 * 
 * mode = 0100664 (octal) → -rw-rw-r--
 * El valor 100664 se descompone así:
 * 100664
 * └── 664 → Permisos del archivo
 * └───── 100000 → Tipo: archivo regular
 * 
 * Permiso           Valor octal
 * -----------------------------
 * Lectura (r)            4
 * Escritura (w)          2
 * Ejecución (x)          1
 * 
 * 
 * FORMAT_FILE_MODE: TIPO DE ARCHIVO Y PERMISOS UNIX/LINUX
 *
 * Transforma los bits de st_mode en una cadena de 10 caracteres
 * equivalente a la primera columna de ls -l.
 *
 *   permissions[0]     -> Tipo de archivo.
 *   permissions[1..3]  -> Permisos del propietario (USR).
 *   permissions[4..6]  -> Permisos del grupo (GRP).
 *   permissions[7..9]  -> Permisos de otros usuarios (OTH).
 *   permissions[10]    -> Terminador '\0'.
 *
 *
 * 1. IDENTIFICACION DEL TIPO DE ARCHIVO
 *
 * El primer caracter identifica el tipo de archivo:
 *
 *   '-'  Archivo regular           S_ISREG(mode)
 *   'd'  Directorio                S_ISDIR(mode)
 *   'l'  Enlace simbolico          S_ISLNK(mode)
 *   'c'  Dispositivo de caracteres S_ISCHR(mode)
 *   'b'  Dispositivo de bloques    S_ISBLK(mode)
 *   'p'  FIFO (tuberia con nombre) S_ISFIFO(mode)
 *   's'  Socket                    S_ISSOCK(mode)
 *   '?'  Tipo desconocido          Valor por defecto
 *
 * Estas macros comprueban el tipo codificado en st_mode.
 *
 * Ejemplos:
 *
 *   -rw-r--r--  -> Archivo regular.
 *   drwxr-xr-x  -> Directorio.
 *   lrwxrwxrwx  -> Enlace simbolico.
 *
 *
 * 2. ESTRUCTURA DE st_mode Y VALORES OCTALES
 *
 * st_mode contiene tres clases de informacion:
 *
 *   - Tipo de archivo.
 *   - Bits especiales.
 *   - Permisos ordinarios (rwx).
 *
 * Ejemplo:
 *
 *   mode = 0100664 (octal)
 *
 *   0100000 -> Tipo: archivo regular.
 *      000 -> Sin bits especiales.
 *      664 -> Permisos: rw-rw-r--.
 *
 *   Resultado: -rw-rw-r--
 *
 * PERMISOS ORDINARIOS
 *
 * Cada categoria (USR, GRP, OTH) utiliza estos valores:
 *
 *   4 -> Lectura   (r)
 *   2 -> Escritura (w)
 *   1 -> Ejecucion (x)
 *
 * Los valores se suman:
 *
 *   7 = 4 + 2 + 1 -> rwx
 *   6 = 4 + 2     -> rw-
 *   5 = 4 + 1     -> r-x
 *   4 = 4         -> r--
 *   0             -> ---
 *
 * Ejemplo:
 *
 *   chmod 755 archivo
 *
 *   USR = 7 -> rwx
 *   GRP = 5 -> r-x
 *   OTH = 5 -> r-x
 *
 *   Resultado: -rwxr-xr-x (archivo regular).
 *
 *
 * 3. PERMISOS ESPECIALES: SETUID, SETGID Y STICKY BIT
 *
 * Existen tres bits especiales independientes de los permisos rwx.
 *
 * 3.1. S_ISUID (setuid)
 *
 * En archivos ejecutables, permite que el programa adopte
 * el UID efectivo del propietario del archivo.
 *
 * Ejemplo:
 * Si el programa pertenece a Ana, Pedro puede ejecutarlo
 * con UID efectivo de Ana, siempre que tenga permiso para
 * ejecutarlo y el sistema aplique setuid.
 *
 * Se representa en permissions[3]: 's' o 'S'.
 *
 * 3.2. S_ISGID (setgid)
 *
 * En archivos ejecutables, permite que el programa adopte
 * el GID efectivo del grupo propietario del archivo.
 *
 * Ejemplo:
 * Un programa pertenece al grupo "empleados".
 * Luis, que pertenece al grupo "ventas", puede ejecutarlo
 * con GID efectivo "empleados" si tiene permiso de ejecucion
 * y el sistema aplica setgid.
 *
 * En directorios, los nuevos archivos y subdirectorios
 * heredan normalmente el grupo propietario del directorio.
 *
 * Se representa en permissions[6]: 's' o 'S'.
 *
 * 3.3. S_ISVTX (sticky bit)
 *
 * En directorios compartidos, restringe el borrado y
 * renombrado de entradas a:
 *
 *   - El propietario de la entrada.
 *   - El propietario del directorio.
 *   - Usuarios con privilegios suficientes.
 *
 * Ejemplo habitual: /tmp.
 *
 * Se representa en permissions[9]: 't' o 'T'.
 *
 *
 * 4. VALORES OCTALES DE LOS BITS ESPECIALES
 *
 * Los bits especiales utilizan tambien los valores 4, 2 y 1,
 * pero en una posicion octal distinta de los permisos rwx.
 *
 *   4 -> setuid      S_ISUID
 *   2 -> setgid      S_ISGID
 *   1 -> sticky bit  S_ISVTX
 *
 * En las cuatro cifras de permisos, la primera representa
 * los bits especiales:
 *
 *   ESPECIAL | USR | GRP | OTH
 *      0        7     5     5     -> 0755
 *      4        7     5     5     -> 4755 (setuid)
 *      2        7     5     5     -> 2755 (setgid)
 *      1        7     5     5     -> 1755 (sticky)
 *
 * Los bits especiales tambien pueden combinarse:
 *
 *   3 = 2 + 1     -> setgid + sticky
 *   5 = 4 + 1     -> setuid + sticky
 *   6 = 4 + 2     -> setuid + setgid
 *   7 = 4 + 2 + 1 -> los tres bits especiales
 *
 * Ejemplos:
 *
 *   chmod 4755 archivo -> Activa setuid.
 *   chmod 2755 archivo -> Activa setgid.
 *   chmod 1755 dir     -> Activa sticky bit.
 *   chmod 6755 archivo -> Activa setuid y setgid.
 *   chmod 7755 archivo -> Activa los tres bits.
 *   chmod 0755 archivo -> Desactiva los tres bits.
 *
 * IMPORTANTE:
 *
 * En estos ejemplos, chmod establece el conjunto completo
 * de bits de permiso indicado, no solamente el bit especial.
 *
 * Para activar o desactivar un bit sin modificar los demas
 * permisos, se puede utilizar chmod simbolico:
 *
 *   chmod u+s archivo -> Activa setuid.
 *   chmod u-s archivo -> Desactiva setuid.
 *
 *   chmod g+s archivo -> Activa setgid.
 *   chmod g-s archivo -> Desactiva setgid.
 *
 *   chmod +t directorio -> Activa sticky bit.
 *   chmod -t directorio -> Desactiva sticky bit.
 *
 * Tambien pueden combinarse bits mediante operaciones:
 *
 *   mode & S_ISUID  -> Comprueba si setuid esta activado.
 *   mode | S_ISUID  -> Calcula un valor con setuid activado.
 *   mode & ~S_ISUID -> Calcula un valor con setuid desactivado.
 *
 * Estas operaciones sobre la variable mode NO modifican
 * los permisos reales del archivo.
 *
 *
 * 5. REPRESENTACION DE LOS BITS ESPECIALES EN ls -l
 *
 * Los bits especiales y los permisos de ejecucion son
 * bits independientes dentro de st_mode.
 *
 * Sin embargo, ls -l utiliza la misma posicion visual
 * para representar ambas informaciones.
 *
 *   Posicion        Bit especial       Caracteres
 *
 *   permissions[3]  S_ISUID (USR)       's' / 'S'
 *   permissions[6]  S_ISGID (GRP)       's' / 'S'
 *   permissions[9]  S_ISVTX (OTH)       't' / 'T'
 *
 *   's' -> setuid/setgid activo y permiso x presente.
 *   'S' -> setuid/setgid activo y permiso x ausente.
 *   't' -> sticky bit activo y permiso x presente.
 *   'T' -> sticky bit activo y permiso x ausente.
 *
 * Las minusculas indican que existe permiso de ejecucion
 * en esa categoria; las mayusculas indican que no existe.
 *
 * Los bits especiales NO conceden por si mismos ejecucion.
 * Los caracteres especiales sustituyen a 'x' o '-'
 * sin aumentar la longitud de la cadena de permisos.
 *
 * EJEMPLOS:
 *
 *   -rwxr-xr-x -> Sin bits especiales.
 *   -rwsr-xr-x -> Setuid activo, USR tiene x.
 *   -rwSr-xr-x -> Setuid activo, USR no tiene x.
 *   -rwxr-sr-x -> Setgid activo, GRP tiene x.
 *   -rwxr-Sr-x -> Setgid activo, GRP no tiene x.
 *   drwxrwxrwt -> Sticky activo, OTH tiene x.
 *   drwxrwxrwT -> Sticky activo, OTH no tiene x.
 *
 *
 * 6. ACLARACIONES IMPORTANTES
 *
 * SETUID:
 *
 * Que setuid aparezca en USR no significa que solo afecte
 * al propietario. Otro usuario con permiso de ejecucion
 * puede iniciar el programa con UID efectivo del propietario.
 *
 * SETGID:
 *
 * Que setgid aparezca en GRP no significa que solo afecte
 * a los miembros de ese grupo. Otro usuario con permiso
 * de ejecucion puede iniciar el programa con GID efectivo
 * del grupo propietario del archivo.
 *
 * STICKY BIT:
 *
 * Aunque se representa en OTH, no afecta exclusivamente
 * a esos usuarios. En directorios, restringe el borrado
 * y renombrado de entradas segun sus propietarios y
 * los privilegios del usuario.
 *
 * Ademas, en un directorio el permiso x significa
 * poder atravesarlo o acceder a sus entradas por nombre.
 *
 *
 * 7. IMPLEMENTACION EN format_file_mode()
 *
 * Cada posicion especial combina dos comprobaciones:
 *
 *   permissions[3] -> S_ISUID + S_IXUSR
 *   permissions[6] -> S_ISGID + S_IXGRP
 *   permissions[9] -> S_ISVTX + S_IXOTH
 *
 * Se utiliza AND bit a bit (&) para consultar cada mascara:
 *
 *   mode & S_IXUSR
 *   mode & S_ISUID
 *
 * Se utiliza AND logico (&&) para exigir dos condiciones:
 *
 *   (mode & S_IXUSR) && (mode & S_ISUID)
 *
 * Las cuatro combinaciones son:
 *
 *   Especial  Ejecucion   Resultado
 *      0         0          '-'
 *      0         1          'x'
 *      1         0          'S' / 'T'
 *      1         1          's' / 't'
 *
 * La funcion solo representa los bits de st_mode.
 * No comprueba que usuario puede ejecutar realmente
 * el archivo ni modifica sus permisos.
 *
 * La presencia de setuid o setgid tampoco garantiza
 * que el sistema aplique su efecto durante la ejecucion.
 * 
 * mode & S_IXUSR es una operación AND bit a bit
 * para comprobar si determinados bits están activos
 */
void	format_file_mode(mode_t mode, char permissions[11])
{
	/* identificar tipo de archivo */
	if (S_ISREG(mode))
		permissions[0] = '-';
	else if (S_ISDIR(mode))
		permissions[0] = 'd';
	else if (S_ISLNK(mode))
		permissions[0] = 'l';
	else if (S_ISCHR(mode))
		permissions[0] = 'c';
	else if (S_ISBLK(mode))
		permissions[0] = 'b';
	else if (S_ISFIFO(mode))
		permissions[0] = 'p';
	else if (S_ISSOCK(mode))
		permissions[0] = 's';
	else
		permissions[0] = '?';
	
	/* A.- Identificar permisos propietario USR (User) --------------------------- */
	/* 1.- permiso de lectura */
	if (mode & S_IRUSR)
		permissions[1] = 'r';
	else
		permissions[1] = '-';
	/* 2.- permiso de escritura */
	if (mode & S_IWUSR)
		permissions[2] = 'w';
	else
		permissions[2] = '-';
	/* 3.- permiso de ejecución del propietario y bit especial setuid */
	/* a.- ¿tiene ejecución y setuid activados? */
	if ((mode & S_IXUSR) && (mode & S_ISUID))
		permissions[3] = 's';
	/* b.- ¿tiene setuid activado, pero no ejecución? */
	else if (mode & S_ISUID)
		permissions[3] = 'S';
	/* c.- ¿tiene ejecución, pero no setuid? */
	else if (mode & S_IXUSR)
		permissions[3] = 'x';
	/* d.- no tiene ejecución ni setuid */
	else
		permissions[3] = '-';

	/* B.- Identificar permisos grupo GRP (Group) -------------------------------- */
	/* 1.- permiso de lectura */
	if (mode & S_IRGRP)
		permissions[4] = 'r';
	else
		permissions[4] = '-';
	/* 2.- permiso de escritura */
	if (mode & S_IWGRP)
		permissions[5] = 'w';
	else
		permissions[5] = '-';
	/* 3.- permiso de ejecución del grupo y bit especial setgid */
	/* a.- ¿tiene ejecución y setgid activados? */
	if ((mode & S_IXGRP) && (mode & S_ISGID))
		permissions[6] = 's';
	/* b.- ¿tiene setgid activado, pero no ejecución? */
	else if (mode & S_ISGID)
		permissions[6] = 'S';
	/* c.- ¿tiene ejecución, pero no setgid? */
	else if (mode & S_IXGRP)
		permissions[6] = 'x';
	/* d.- no tiene ejecución ni setgid */
	else
		permissions[6] = '-';

	/* C.- Identificar permisos otros OTH (Others) -------------------------------- */
	/* 1.- permiso de lectura */
	if (mode & S_IROTH)
		permissions[7] = 'r';
	else
		permissions[7] = '-';
	/* 2.- permiso de escritura */
	if (mode & S_IWOTH)
		permissions[8] = 'w';
	else
		permissions[8] = '-';
	/* 3.- permiso de ejecución para resto usuarios y stick bit */
	/* a.- ¿tiene ejecución y sticky bit activados? */
	if ((mode & S_IXOTH) && (mode & S_ISVTX))
		permissions[9] = 't';
	/* b.- ¿tiene sticky bit activado, pero no ejecución? */
	else if (mode & S_ISVTX)
		permissions[9] = 'T';
	/* c.- ¿tiene ejecución, pero no sticky bit activado? */
	else if (mode & S_IXOTH)
		permissions[9] = 'x';
	/* d.- no tiene ejecución ni sticky bit activado*/
	else
		permissions[9] = '-';

	permissions[10] = '\0';
}

void	print_file_owner(const struct stat *st, size_t width)
{
	struct passwd	*pw;

	pw = getpwuid(st->st_uid);
	if (pw)
		printf("%-*s ", (int)width, pw->pw_name);
	else
		printf("%-*ju ", (int)width, (uintmax_t)st->st_uid);
}

void	print_file_group(const struct stat *st, size_t width)
{
	struct group	*gr;

	gr = getgrgid(st->st_gid);
	if (gr)
		printf("%-*s ", (int)width, gr->gr_name);
	else
		printf("%-*ju ", (int)width, (uintmax_t)st->st_gid);
}

/*
 * 1 si la fecha es reciente
 * 0 si es antigua o futura
 * st->st_mtim.tv_sec	Segundos de modificación del archivo
 * st->st_mtim.tv_nsec 	Nanosegundos de modificación
 * now->tv_sec			Segundos de la hora actual
 * now->tv_nsec			Nanosegundos de la hora actual
 * ambos instantes se pueden comparar directamente porque utilizan la misma referencia temporal
 * Como now->tv_sec representa segundos desde el 1 de enero de 1970,
 * puedes restarle los segundos equivalentes a aproximadamente seis meses
 * GNU ls utiliza como referencia la mitad de un año de 365 días:
 * 1/2 x (365 x 24 x 60 x 60) = 15 768 000
 * comparamos los nanosegundos con now
 * limit = now->tv_sec - 15768000;
 * Solo restamos segundos. Los nanosegundos del límite siguen siendo los mismos que los de now.
 * 
 * touch -d	permite establecer manualmente la fecha de modificación (mtime) de un archivo
 * # Archivo con fecha futura (dentro de un año)
 * touch -d "1 year" t/d/fecha_futura
 * # Archivo reciente (hace 2 meses)
 * touch -d "2 months ago" t/d/fecha_reciente
 * # Archivo antiguo (hace 8 meses)
 * touch -d "8 months ago" t/d/fecha_antigua
 * touch -d modifica también la fecha de acceso (atime), pero solo nos interesa mtime.
 */
static int	is_recent_file_time(const struct timespec *mtime, const struct timespec *now)
{
	time_t	limit;

	/* identificar fechas futuras */
	if ((mtime->tv_sec > now->tv_sec) || ((mtime->tv_sec == now->tv_sec) && (mtime->tv_nsec > now->tv_nsec)))
		return (0);

	/* identificar fechas recientes - comprobar antigüedad de seis meses */
	limit = now->tv_sec - 15768000;
	if ((mtime->tv_sec > limit) || ((mtime->tv_sec == limit) && (mtime->tv_nsec >= now->tv_nsec)))
		return (1);

	/* fechas antiguas */
	return (0);
}

/*
 * struct tm *localtime_r(const time_t *timep, struct tm *result);
 * timep	puntero a los segundos que queremos convertir (time_t)
 * result	puntero a una estructura struct tm donde se guardará la fecha
 * retorno	puntero a result si funciona; NULL si falla
 * 
 * CAMPOS DE struct tm (fecha y hora local):
 *
 * Ejemplo: 4 de noviembre de 2026 a las 14:40:00
 *
 * Campo       Valor    Significado
 * ------------------------------------------------
 * tm_year      126     Años transcurridos desde 1900
 * tm_mon        10     Mes (0 = enero, 11 = diciembre)
 * tm_mday        4     Día del mes (1-31)				printf("Día: %d\n", tm_info.tm_mday);
 * tm_hour       14     Hora (0-23)						printf("Hora: %d\n", tm_info.tm_hour);		
 * tm_min        40     Minutos (0-59)					printf("Minutos: %d\n", tm_info.tm_min);
 * tm_sec         0     Segundos (0-60)					printf("Segundos: %d\n", tm_info.tm_sec);
 *
 * Año real = tm_year + 1900 							printf("Año: %d\n", tm_info.tm_year + 1900);
 * Mes real = tm_mon + 1								printf("Mes: %d\n", tm_info.tm_mon + 1);	
 * 
 * localtime_r() no produce la cadena "Nov  4 14:40".
 * localtime_r() Zona horaria (TZ)
 * Solo descompone la fecha en una estructura struct tm
 * Para convertirla en texto utilizaremos después strftime()
 * size_t strftime(char *s, size_t max, const char *format, const struct tm *tm);
 * s		buffer donde se guardará la fecha formateada
 * max		tamaño total del buffer, incluido '\0'
 * format	cadena que indica el formato de fecha
 * tm		puntero a la estructura struct tm
 *
 * CÓDIGOS DE FORMATO DE strftime():
 *
 * Código    Significado                         Ejemplo
 * -----------------------------------------------------
 * %b        Mes abreviado                       Nov
 * %e        Día del mes (relleno con espacios)   4
 * %H        Hora (00-23)                        14
 * %M        Minutos (00-59)                     40
 *
 * Formato: "%b %e %H:%M"
 * Resultado: "Nov  4 14:40"
 * 
 * FORMATO DE FECHAS EN GNU ls -l
 *
 * Fecha             Formato          Ejemplo
 * ------------------------------------------------
 * Reciente           %b %e %H:%M     Nov  4 14:40
 * Antigua            %b %e  %Y       Nov  4  2025
 * Demasiado futura   %b %e  %Y       Nov  4  2027
 *
 * REGLA PARA DETERMINAR SI UNA FECHA ES RECIENTE:
 *
 * - La fecha debe pertenecer aproximadamente a los últimos
 *   seis meses respecto a la hora actual.
 *
 * - La fecha no debe ser posterior a la hora actual.
 *   GNU ls considera las fechas futuras como no recientes.
 *
 * IMPORTANTE:
 *
 * - GNU ls utiliza un umbral aproximado de seis meses,
 *   calculado en segundos, no mediante meses del calendario.
 *
 * - Las fechas recientes muestran la hora y los minutos (HH:MM).
 *
 * - Las fechas antiguas o futuras muestran el año (YYYY).
 *
 * - La comparación se realiza entre st->st_mtim y ctx->now,
 *   utilizando segundos y nanosegundos cuando corresponda.
 */
void	format_file_time(const struct stat *st, const struct timespec *now, char date[32])
{
	struct tm	tm_info;
	const char	*format;

	if (!localtime_r(&st->st_mtim.tv_sec, &tm_info))
	{
		date[0] = '\0';																			/* provisional cadena vacia */
        return ;
	}
	if (is_recent_file_time(&st->st_mtim, now))
		format = "%b %e %H:%M";																	/* reciente: mostrar hora */
	else
		format = "%b %e  %Y";																	/* antigua o futura: mostrar año */
	
	if (!strftime(date, 32, format , &tm_info))
	{
		date[0] = '\0';																			/* provisional cadena vacia */
		return ;
	}
}

char	*read_link_target(t_ctx *ctx, const char *path, const struct stat *st)
{
	ssize_t	n;																					/* numero de bytes copiados en el buffer (target) */
	size_t	size;
	char	*target;
	char	*tmp;

	/*
	* 'st_size' indica normalmente la longitud en bytes del destino almacenado
	* En un enlace simbólico, sin embargo no es una garantia absoluta
	* Usamos 128 bytes como alternativa de inicialización si 'st->st_size' <= 0
	* ¡! (cast) 'st->st_size' es de tipo 'off_t', normalmente un entero con signo
	* mientras que 'size_t' es un entero sin signo
	*/
	if (st->st_size > 0)
		size = (size_t)st->st_size;
	else
		size = 128;
	if (size == SIZE_MAX)																		/* comprobar no haya desbordamiento de (size + 1) en el malloc */
	{
		ctx->fatal = FATAL_NOMEM;
		return (NULL);
	}
	target = malloc(sizeof(char) * (size + 1));													/* '+1' readlink() NO añade el terminador '\0' en el buffer (target) */											
	if (!target)
	{
		ctx->fatal = FATAL_NOMEM;
		return (NULL);
	}
	n = readlink(path, target, size);															/* readlink() lee la ruta del destino almacenada en ese enlace */
	if (n == -1)
	{
		fprintf(stderr, "%s: cannot read symbolic link '%s': %s\n",
			ctx->prog, path, strerror(errno));													/* error lectura */
		if (ctx->status == 0)
			ctx->status = 1;
		free(target);
		return (NULL);
	}
	/*
	 * Si readlink() llena el buffer, el destino puede estar
	 * truncado. Duplicamos la capacidad y repetimos la lectura.
	 */
	while ((size_t)n == size)																	/* 'n' son los bytes que ha copiado readlink() en el buffer (target) y 'size' es la capacidad disponible para guardar el destino. Si ambos son iguales no sabemos si el destino cabe justo o si se ha truncado, por eso ampliamos */
	{
		if (size > (SIZE_MAX - 1) / 2)
		{
			free(target);
			ctx->fatal = FATAL_NOMEM;
			return (NULL);
		}
		size *= 2;																				/* duplicar -> estrategia habitual de crecimiento dinámico, porque reduce el número de ampliaciones */
		tmp = realloc(target, size + 1);														/* usamos un puntero temporal (tmp) porque, si realloc() falla, el bloque original (target) sigue reservado y debemos liberarlo */
		if (!tmp)
		{
			free(target);
			ctx->fatal = FATAL_NOMEM;
			return (NULL);
		}
		target = tmp;																			/* copia de la dirección de memoria almacenada en tmp, no el contenido del bloque de memoria - target y tmp apuntan al mismo bloque de memoria - por tanto, bastará con liberar target */
		n = readlink(path, target, size);
		if (n == -1)
		{
			fprintf(stderr, "%s: cannot read symbolic link '%s': %s\n",
				ctx->prog, path, strerror(errno));												/* error lectura */
			if (ctx->status == 0)
				ctx->status = 1;
			free(target);
			return (NULL);
		}
	}
	target[n] = '\0';																			/* readlink() NO añade el terminador '\0' en el buffer (target) */
	return (target);
}
