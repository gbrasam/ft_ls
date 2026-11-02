/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   long_format.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/31 12:41:10 by gebravo           #+#    #+#             */
/*   Updated: 2026/11/02 16:55:25 by gebravo          ###   ########.fr       */
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
static size_t	count_digits(size_t value)
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
static void	update_max_width(size_t width, size_t *max_width)
{
	if (width > *max_width)
		*max_width = width;
}

/*
 * Actualiza los anchos máximos de las columnas de longitud variable
 * del formato largo (-l): enlaces, propietario, grupo y tamaño.
 *
 * Se llama una vez por cada entrada que se va a imprimir y cuyos
 * metadatos son válidos (struct stat).
 *
 * Los máximos se acumulan para todo el listado, de modo que las
 * columnas queden alineadas aunque sus valores tengan distintas
 * longitudes.
 *
 * No recorre el listado, no imprime ni calcula el total de bloques.
 * Estas tareas corresponden a la preparación del listado.
 *
 * IMPORTANTE:
 * El ancho representa el número de caracteres de la representación
 * impresa, no el tamaño del dato en memoria ni su valor numérico.
 *
 * Ejemplos:
 *   st_nlink = 125  -> 3 caracteres
 *   st_size  = -125 -> 4 caracteres (incluido el signo '-')
 *
 * Por eso los campos de t_print_widths son size_t: almacenan
 * cantidades de caracteres, no los valores originales.
 *
 * FUNCIONES AUXILIARES:
 *
 * count_digits(size_t value):
 *   Cuenta los dígitos decimales de un entero sin signo.
 *   El valor 0 también ocupa un carácter.
 *
 * update_max_width(size_t width, size_t *max_width):
 *   Conserva el mayor ancho acumulado.
 *   Recibe un puntero para modificar directamente el campo
 *   correspondiente de t_print_widths.
 *
 * ------------------------------------------------------------
 * ENLACES (nlink)
 * ------------------------------------------------------------
 *
 * st_nlink es de tipo nlink_t, habitualmente sin signo.
 * Representa el número de enlaces físicos (hard links).
 *
 * Su representación decimal determina el ancho de la columna.
 *
 * ------------------------------------------------------------
 * TAMAÑO (size)
 * ------------------------------------------------------------
 *
 * st_size es de tipo off_t, un entero con signo.
 *
 * Los tamaños negativos necesitan un carácter adicional para
 * representar el signo '-'.
 *
 * Ejemplo:
 *   -125 -> 3 dígitos + 1 signo = 4 caracteres
 *
 * PRECAUCIÓN CON LOS ENTEROS CON SIGNO:
 *
 * El valor mínimo de un entero con signo no puede negarse
 * directamente, porque su magnitud positiva no cabe en ese tipo.
 *
 * Por eso utilizamos:
 *   (size_t)(-(st->st_size + 1)) + 1
 *
 * Primero sumamos 1 para poder negar el valor sin desbordamiento.
 * Después convertimos a size_t y recuperamos la unidad.
 *
 * Esta operación supone que size_t puede representar la magnitud
 * de off_t, como ocurre en nuestro entorno Linux de 64 bits.
 *
 * ------------------------------------------------------------
 * PROPIETARIO (owner)
 * ------------------------------------------------------------
 *
 * st_uid es de tipo uid_t e identifica numéricamente al propietario.
 *
 * Sin embargo, ls -l muestra normalmente el nombre del usuario,
 * no su identificador numérico.
 *
 * getpwuid() obtiene la información de un usuario a partir de su UID.
 *
 * Firma:
 *   struct passwd *getpwuid(uid_t uid);
 *
 * Cabeceras:
 *   <sys/types.h>
 *   <pwd.h>
 *
 * Devuelve un puntero a struct passwd si encuentra el usuario,
 * o NULL si no puede resolverlo o se produce un error.
 *
 * struct passwd contiene, entre otros, el campo:
 *   char *pw_name;  // Nombre del usuario
 *
 * Ejemplo:
 *   UID = 0 -> usuario "root" -> ancho 4
 *
 * Si getpwuid() encuentra el usuario:
 *   ancho = strlen(user_info->pw_name)
 *
 * Si devuelve NULL:
 *   ancho = count_digits((size_t)st->st_uid)
 *
 * Un UID puede ser válido aunque el sistema no encuentre un nombre
 * asociado. En ese caso se utiliza su representación numérica
 * como alternativa, sin interrumpir el listado.
 *
 * La información puede proceder de /etc/passwd o de otras fuentes
 * configuradas en el sistema, como servicios de directorio.
 *
 * ------------------------------------------------------------
 * GRUPO (group)
 * ------------------------------------------------------------
 *
 * st_gid es de tipo gid_t e identifica numéricamente al grupo.
 *
 * Al igual que con el propietario, ls -l muestra normalmente
 * el nombre del grupo en lugar de su identificador numérico.
 *
 * getgrgid() obtiene la información de un grupo a partir de su GID.
 *
 * Firma:
 *   struct group *getgrgid(gid_t gid);
 *
 * Cabeceras:
 *   <sys/types.h>
 *   <grp.h>
 *
 * Devuelve un puntero a struct group si encuentra el grupo,
 * o NULL si no puede resolverlo o se produce un error.
 *
 * struct group contiene, entre otros, el campo:
 *   char *gr_name;  // Nombre del grupo
 *
 * Ejemplo:
 *   GID = 0 -> grupo "root" -> ancho 4
 *
 * Si getgrgid() encuentra el grupo:
 *   ancho = strlen(group_info->gr_name)
 *
 * Si devuelve NULL:
 *   ancho = count_digits((size_t)st->st_gid)
 *
 * Un GID puede ser válido aunque no tenga un nombre disponible.
 * En ese caso se utiliza su representación numérica y se
 * continúa el listado.
 *
 * La información puede proceder de /etc/group o de otras
 * fuentes configuradas en el sistema.
 *
 * ------------------------------------------------------------
 * COHERENCIA CON LA IMPRESIÓN
 * ------------------------------------------------------------
 *
 * El ancho calculado debe corresponder a la representación
 * que realmente se imprimirá: nombre o identificador numérico.
 *
 * Si la resolución de usuarios o grupos cambia entre la
 * preparación y la impresión, podría alterarse la alineación.
 * Esta coherencia deberá comprobarse al implementar la salida.
 *
 * ------------------------------------------------------------
 * PENDIENTE
 * ------------------------------------------------------------
 *
 * Los archivos especiales de dispositivo pueden mostrar
 * números major/minor en lugar del tamaño habitual.
 * Su representación requiere un tratamiento específico.
 */
void	update_print_widths(const struct stat *st, t_print_widths *widths)
{
	size_t			digits;
	struct passwd	*user_info;
	struct group	*group_info;

	/* st->st_nlink */
	digits = count_digits(st->st_nlink);
	update_max_width(digits, &widths->nlink);

	/* st->st_size */
	if (st->st_size < 0)
	{
		digits = count_digits((size_t)(-(st->st_size + 1)) + 1);
		digits++;
	}
	else
		digits = count_digits((size_t)st->st_size);
	update_max_width(digits, &widths->size);

	/* st->st_uid */
	user_info = getpwuid(st->st_uid);
	if (!user_info)
		digits = count_digits((size_t)st->st_uid);
	else
		digits = strlen(user_info->pw_name);
	update_max_width(digits, &widths->owner);

	/* st->st_gid */
	group_info = getgrgid(st->st_gid);
	if (!group_info)
		digits = count_digits((size_t)st->st_gid);
	else
		digits = strlen(group_info->gr_name);
	update_max_width(digits, &widths->group);
}

/*
 * Imprime una entrada en el formato largo de ls (-l).
 *
 * Cada entrada ocupa una línea y muestra sus metadatos
 * además del nombre.
 *
 * Ejemplo con LC_ALL=C:
 *   -rw-r--r-- 1 root root 125 Nov  2 17:30 archivo.txt
 *
 * CAMPOS DEL FORMATO LARGO:
 *
 * Tipo y permisos:
 *   El primer carácter identifica el tipo de archivo.
 *   Los nueve siguientes representan los permisos de
 *   propietario, grupo y otros usuarios.
 *   También deben contemplarse los bits especiales:
 *   setuid, setgid y sticky bit.
 *
 * Número de enlaces:
 *   Cantidad de enlaces físicos (hard links) del archivo.
 *
 * Propietario y grupo:
 *   Normalmente se muestran sus nombres.
 *   Si no pueden resolverse, se utilizan los identificadores
 *   numéricos UID y GID.
 *
 * Tamaño:
 *   Para archivos normales se expresa en bytes.
 *   Los dispositivos especiales requieren otro tratamiento.
 *
 * Fecha:
 *   Se utiliza la fecha de última modificación (mtime).
 *   Su representación depende de la antigüedad de la fecha.
 *
 * Nombre:
 *   Identifica la entrada que se está mostrando.
 *   Los enlaces simbólicos muestran también su destino.
 *
 * ------------------------------------------------------------
 * PARÁMETROS
 * ------------------------------------------------------------
 *
 * ctx:
 *   Contiene las opciones de ejecución y ctx->now, la hora
 *   obtenida una sola vez para mantener un criterio temporal
 *   uniforme durante todo el listado.
 *
 * name:
 *   Nombre que debe aparecer en el listado.
 *
 * path:
 *   Ruta utilizada para acceder a la entrada.
 *   Puede ser distinta del nombre mostrado.
 *
 *   Es especialmente importante para readlink(), porque
 *   el nombre por sí solo puede no identificar correctamente
 *   un enlace situado dentro de otro directorio.
 *
 * st:
 *   Metadatos válidos obtenidos previamente mediante
 *   stat() o lstat(), según corresponda.
 *
 *   No es necesario repetir esas consultas para imprimir
 *   la información que ya tenemos.
 *
 * widths:
 *   Anchos máximos calculados antes de imprimir el listado.
 *   Permiten alinear las columnas de longitud variable.
 *
 *   Son específicos del conjunto de entradas que se imprime.
 *   Esta función los consulta, pero no los modifica.
 *
 * ------------------------------------------------------------
 * TIPO DE ARCHIVO Y PERMISOS
 * ------------------------------------------------------------
 *
 * st_mode contiene tanto el tipo de archivo como sus permisos.
 *
 * Los permisos se representan mediante los caracteres:
 *   r  lectura
 *   w  escritura
 *   x  ejecución
 *   -  permiso ausente
 *
 * Los bits especiales pueden modificar la representación
 * de los permisos de ejecución mediante s, S, t o T.
 *
 * El primer carácter depende del tipo de archivo:
 *   -  archivo regular
 *   d  directorio
 *   l  enlace simbólico
 *   c  dispositivo de caracteres
 *   b  dispositivo de bloques
 *   p  tubería con nombre (FIFO)
 *   s  socket
 *
 * ------------------------------------------------------------
 * FECHAS
 * ------------------------------------------------------------
 *
 * Se utiliza mtime (última modificación del contenido),
 * no atime (último acceso) ni ctime (cambio de metadatos).
 *
 * Para fechas recientes:
 *   Nov  2 17:30
 *
 * Para fechas antiguas:
 *   Nov  2 2025
 *
 * GNU ls considera reciente una fecha posterior al límite
 * aproximado de seis meses y que no esté más de una hora
 * en el futuro respecto a la hora actual.
 *
 * Las fechas fuera de ese intervalo muestran el año
 * en lugar de la hora.
 *
 * El criterio se evalúa respecto a ctx->now, obtenido
 * una sola vez para evitar diferencias durante el listado.
 *
 * Los nombres de los meses dependen del locale.
 * En nuestras pruebas utilizamos LC_ALL=C.
 *
 * ------------------------------------------------------------
 * ENLACES SIMBÓLICOS
 * ------------------------------------------------------------
 *
 * Un enlace simbólico almacena una ruta hacia otro archivo
 * o directorio. No es un enlace físico (hard link).
 *
 * En formato largo se muestra:
 *   enlace -> destino
 *
 * readlink() permite obtener el destino del enlace.
 *
 * Firma:
 *   ssize_t readlink(const char *path, char *buf, size_t bufsiz);
 *
 * Cabecera:
 *   <unistd.h>
 *
 * Parámetros:
 *   path   Ruta del enlace simbólico.
 *   buf    Buffer donde se escribe el destino.
 *   bufsiz Capacidad del buffer en bytes.
 *
 * Valor de retorno:
 *   Número de bytes escritos en el buffer.
 *   -1 si se produce un error.
 *
 * IMPORTANTE:
 *   readlink() NO añade el terminador '\0'.
 *
 *   Para utilizar el resultado como cadena C debemos
 *   reservar espacio para el terminador y añadirlo
 *   nosotros después de una lectura correcta.
 *
 *   Si el buffer es demasiado pequeño, el destino puede
 *   quedar truncado. Hay que comprobar su capacidad.
 *
 *   El destino devuelto es el texto almacenado en el enlace;
 *   no tiene por qué ser una ruta absoluta ni existir.
 *
 * ------------------------------------------------------------
 * DISPOSITIVOS ESPECIALES
 * ------------------------------------------------------------
 *
 * En Unix/Linux, algunos archivos representan dispositivos
 * o interfaces del sistema en lugar de datos ordinarios.
 * Habitualmente se encuentran en /dev.
 *
 * Ejemplos:
 *   /dev/null  Descarta los datos escritos en él.
 *   /dev/zero  Proporciona bytes con valor cero.
 *
 * Los dispositivos pueden ser:
 *   De caracteres: acceso como flujo de bytes.
 *   De bloques: acceso mediante bloques de datos.
 *
 * En ls -l no muestran el tamaño habitual, sino dos números:
 *   major y minor.
 *
 * Ejemplo habitual en Linux:
 *   crw-rw-rw- 1 root root 1, 3 Nov  2 17:30 /dev/null
 *
 * En este ejemplo:
 *   1 (major) identifica la clase/controlador del dispositivo.
 *   3 (minor) identifica el dispositivo dentro de esa clase.
 *
 * Los identificadores se obtienen de st_rdev mediante:
 *   major() y minor()
 *
 * Cabecera en Linux:
 *   <sys/sysmacros.h>
 *
 * No debe confundirse:
 *   st_dev  Dispositivo que contiene el sistema de archivos.
 *   st_rdev Dispositivo representado por el archivo especial.
 *
 * S_ISCHR() y S_ISBLK() permiten reconocer estos tipos
 * a partir de st_mode.
 *
 * IMPORTANTE PARA LA ALINEACIÓN:
 *   Los dispositivos necesitan un cálculo de ancho diferente
 *   al de st_size. La preparación de anchos y la impresión
 *   deben utilizar la misma representación.
 *
 * ------------------------------------------------------------
 * COHERENCIA CON LA PREPARACIÓN
 * ------------------------------------------------------------
 *
 * La representación impresa debe coincidir con los anchos
 * calculados previamente mediante update_print_widths().
 *
 * Esto afecta especialmente a:
 *   - Nombres de propietarios y grupos frente a UID/GID.
 *   - Tamaños normales frente a números major/minor.
 *
 * La resolución de nombres puede cambiar entre la preparación
 * y la impresión. Debemos evitar que esas diferencias alteren
 * la alineación de las columnas.
 *
 * ------------------------------------------------------------
 * RESPONSABILIDAD
 * ------------------------------------------------------------
 *
 * Esta función imprime una única entrada.
 *
 * No recorre directorios, no ordena entradas, no calcula
 * anchos máximos ni obtiene la hora actual.
 *
 * La preparación del listado y la impresión son
 * responsabilidades separadas.
 *
 * PENDIENTE:
 *   Implementar la representación de cada campo y verificar
 *   que coincide con los anchos calculados previamente.
 */
void	print_long_format(t_ctx *ctx, const char *name, const char *path, const struct stat *st, const t_print_widths *widths)
{
	(void)ctx;
	(void)name;
	(void)path;
	(void)st;
	(void)widths;
}
