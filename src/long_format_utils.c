/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   long_format_utils.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/31 12:41:10 by gebravo           #+#    #+#             */
/*   Updated: 2026/11/04 18:30:00 by gebravo          ###   ########.fr       */
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
 * Macro             Tipo de archivo                  Carácter ls -l
 * -----------------------------------------------------------------
 * S_ISREG(mode)     Fichero regular                  '-'
 * S_ISDIR(mode)     Directorio                       'd'
 * S_ISLNK(mode)     Enlace simbólico                 'l'
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
	
	/* identificar permisos propietario USR (User) */
	if (mode & S_IRUSR)
		permissions[1] = 'r';
	else
		permissions[1] = '-';
	if (mode & S_IWUSR)
		permissions[2] = 'w';
	else
		permissions[2] = '-';
	if (mode & S_IXUSR)
		permissions[3] = 'x';
	else
		permissions[3] = '-';

	/* identificar permisos grupo GRP (Group) */
	if (mode & S_IRGRP)
		permissions[4] = 'r';
	else
		permissions[4] = '-';
	if (mode & S_IWGRP)
		permissions[5] = 'w';
	else
		permissions[5] = '-';
	if (mode & S_IXGRP)
		permissions[6] = 'x';
	else
		permissions[6] = '-';

	/* identificar permisos otros OTH (Others)*/
	if (mode & S_IROTH)
		permissions[7] = 'r';
	else
		permissions[7] = '-';
	if (mode & S_IWOTH)
		permissions[8] = 'w';
	else
		permissions[8] = '-';
	if (mode & S_IXOTH)
		permissions[9] = 'x';
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
		date[0] = '\0';									/* provisional cadena vacia */
        return ;
	}
	if (is_recent_file_time(&st->st_mtim, now))
		format = "%b %e %H:%M";							/* reciente: mostrar hora */
	else
		format = "%b %e  %Y";							/* antigua o futura: mostrar año */
	
	if (!strftime(date, 32, format , &tm_info))
	{
		date[0] = '\0';									/* provisional cadena vacia */
		return ;
	}
}
