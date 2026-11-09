/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   long_format.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/31 12:41:10 by gebravo           #+#    #+#             */
/*   Updated: 2026/11/09 18:30:25 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

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


	/* st_size (tamaño) o major/minor de st_rdev (identificador del dispositivo) */
	/*
	 * En Linux, los archivos especiales de dispositivo permiten acceder
	 * a dispositivos o interfaces del kernel.
	 * S_ISCHR(): identifica dispositivos de caracteres.
	 * S_ISBLK(): identifica dispositivos de bloques.
	 * En ambos casos, ls -l muestra major, minor en lugar de st_size.
	 */
	if (S_ISCHR(st->st_mode) || S_ISBLK(st->st_mode))
	{
		/*
		 * st_rdev contiene el identificador del dispositivo (major y minor).
		 * major() y minor() extraen sus valores.
		 * No guardamos los valores, sino el número de dígitos que ocupan.
		 * widths->major y widths->minor conservan el ancho máximo encontrado
		 * para alinear correctamente la salida de ls -l.
		 */
		digits = count_digits((size_t)major(st->st_rdev));
		update_max_width(digits, &widths->major);
		digits = count_digits((size_t)minor(st->st_rdev));
		update_max_width(digits, &widths->minor);
	}
	else
	{
		if (st->st_size < 0)
		{
			digits = count_digits((size_t)(-(st->st_size + 1)) + 1);
			digits++;
		}
		else
			digits = count_digits((size_t)st->st_size);
		update_max_width(digits, &widths->size);
	}
	
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
 * Ajusta los anchos definitivos de la columna compartida
 * por el tamaño de los archivos y los identificadores de dispositivo.
 *
 * CONTEXTO:
 *
 * En ls -l, los archivos normales muestran st_size.
 * Los dispositivos de caracteres o bloques muestran major, minor.
 *
 * Ambos formatos ocupan la misma columna y deben alinearse.
 *
 * VALORES DE ENTRADA:
 *
 * update_print_widths() ha calculado previamente:
 *
 * - widths->size:  ancho máximo de st_size.
 * - widths->major: ancho máximo de major.
 * - widths->minor: ancho máximo de minor.
 *
 * CÁLCULO:
 *
 * column_width conserva el mayor ancho necesario entre:
 *
 * - El tamaño máximo de un archivo.
 * - La representación completa de "major, minor".
 *
 * El valor 2 corresponde a la coma y al espacio (", ").
 *
 * Solo se considera major/minor cuando existen dispositivos
 * en el listado, para evitar añadir dos caracteres innecesarios
 * cuando ambos anchos son cero.
 *
 * AJUSTE DE LOS ANCHOS:
 *
 * widths->size pasa a representar el ancho total compartido.
 *
 * widths->major pasa a representar el espacio reservado para
 * imprimir major, incluido el relleno a su izquierda.
 *
 * widths->minor conserva su ancho original, permitiendo
 * mantener la alineación independiente de minor.
 *
 * EJEMPLO:
 *
 * Anchos iniciales:
 *   size  = 8
 *   major = 2
 *   minor = 3
 *
 * Ancho compartido:
 *   column_width = max(8, 2 + 2 + 3) = 8
 *
 * Anchos definitivos:
 *   size  = 8
 *   major = 8 - 2 - 3 = 3
 *   minor = 3
 *
 * Representación:
 *   Tamaño:      "12345678"
 *   Dispositivo: " 11, 235"
 *
 * IMPORTANTE:
 *
 * Esta función modifica widths y debe ejecutarse una sola vez,
 * después de calcular todos los anchos individuales y antes
 * de comenzar la impresión.
 *
 * No debe volver a llamarse update_print_widths() sobre
 * estos anchos ya ajustados.
 *
 * RESPONSABILIDAD:
 *
 * Prepara los anchos definitivos de size y major/minor.
 * No recorre archivos, no consulta metadatos y no imprime.
 */
void	prepare_size_widths(t_print_widths *widths)
{
	size_t	column_width;

	column_width = widths->size;
	if (widths->major > 0 || widths->minor > 0)													/* solo incluir (major, minor) si existen dispositivos en el listado */
	{
		if (widths->major + 2 + widths->minor > column_width)									/* ancho total de la columna compartida por st_size y major, minor; el 2 corresponde a ", " */
			column_width = widths->major + 2 + widths->minor;
		widths->major = column_width - 2 - widths->minor;
	}
	widths->size = column_width;
}

/*
 * Imprime una entrada en el formato largo de ls (-l) cuando sus metadatos (st) son válidos
 * Imprime utilizando los metadatos de lstat()
 *
 * Cada entrada ocupa una línea y muestra sus metadatos además del nombre.
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
 * Para fechas antiguas o futuras:
 *   Nov  2  2025
 *
 * GNU ls considera reciente una fecha posterior al límite
 * aproximado de seis meses y no posterior a la hora actual.
 * Cualquier fecha futura, aunque sea por segundos,
 * se considera no reciente y muestra el año.
 *
 * Las fechas fuera de ese intervalo muestran el año en lugar de la hora.
 *
 * El criterio se evalúa respecto a ctx->now, obtenido
 * una sola vez para evitar diferencias durante el listado.
 *
 * Los nombres de los meses dependen del locale.
 * En nuestras pruebas utilizamos LC_ALL=C.
 *
 * * ------------------------------------------------------------
 * ENLACES SIMBOLICOS: readlink()
 * ------------------------------------------------------------
 *
 * Un enlace simbolico almacena una ruta hacia otro archivo
 * o directorio. Es diferente de un enlace fisico (hard link).
 *
 * En ls -l se muestra:
 *   enlace -> destino
 *
 * readlink() lee la ruta almacenada en el enlace simbolico
 * sin seguirlo y sin necesitar que exista su destino.
 *
 * Firma:
 *   ssize_t readlink(const char *path, char *buf, size_t bufsiz);
 *
 * Cabecera:
 *   <unistd.h>
 *
 * Parametros:
 *   path   -> Ruta del enlace simbolico que queremos consultar.
 *   buf    -> Buffer donde se copia el destino.
 *   bufsiz -> Numero maximo de bytes que pueden copiarse.
 *
 * VALOR DE RETORNO:
 *
 *   n >= 0 -> Exito. Numero de bytes copiados en el buffer.
 *   n == -1 -> Error. errno indica la causa.
 *
 * POSIBLES ERRORES:
 *
 *   ENOENT -> El enlace ha desaparecido o falta algun
 *             componente de la ruta.
 *
 *   EACCES -> No tenemos permiso para atravesar algun
 *             directorio de la ruta.
 *
 *   EINVAL -> La ruta no corresponde a un enlace simbolico.
 *
 * Aunque lstat() haya identificado previamente un enlace,
 * readlink() puede fallar si el sistema de archivos cambia
 * entre ambas llamadas.
 *
 * IMPORTANTE:
 *
 *   readlink() NO añade el terminador '\0'.
 *
 *   Para utilizar el resultado como cadena C debemos
 *   reservar un byte adicional y añadirlo nosotros
 *   despues de una lectura correcta:
 *
 *      buffer[n] = '\0';
 *
 * IMPORTANTE: TRUNCAMIENTO
 *
 *   Si el buffer es demasiado pequeño, readlink() copia
 *   solamente los bytes que caben, sin devolver un error.
 *
 *   Si n == bufsiz, el destino puede estar truncado.
 *   Debemos ampliar el buffer y repetir la lectura.
 *
 *   Si n < bufsiz, el destino cabe completo.
 *
 * OTRAS CONSIDERACIONES:
 *
 *   - Un enlace roto no provoca por si mismo un error.
 *   - El destino puede ser una ruta relativa o absoluta.
 *   - readlink() devuelve el texto almacenado en el enlace,
 *     no la ruta canonica del archivo de destino.
 *   - Si readlink() devuelve -1, no debemos utilizar
 *     el contenido del buffer como un destino valido.
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
 * TIPOS DE DATOS DE STRUCT STAT Y FORMATOS DE PRINTF
 * ------------------------------------------------------------
 *
 * Tipo             Campo       Significado                  Formato  Conversión
 * -----------------------------------------------------------------------------------
 * mode_t           st_mode     Tipo de archivo y permisos   %jo      (uintmax_t)
 * off_t            st_size     Tamaño lógico en bytes       %jd      (intmax_t)
 * struct timespec  st_mtim     Última modificación          N/A      localtime_r()
 * uid_t            st_uid      ID del propietario           %ju      (uintmax_t)
 * gid_t            st_gid      ID del grupo                 %ju      (uintmax_t)
 * nlink_t          st_nlink    Número de enlaces duros      %ju      (uintmax_t)
 * blkcnt_t         st_blocks   Bloques de 512 B (Linux)     %jd      (intmax_t)
 *
 * FORMATOS:
 *
 * %jd  -> Entero decimal con signo (intmax_t).
 * %ju  -> Entero decimal sin signo (uintmax_t).
 * %jo  -> Entero octal sin signo (uintmax_t).
 *
 * NOTAS:
 *
 * - <inttypes.h> proporciona los tipos intmax_t y uintmax_t,
 *   y los formatos correspondientes.
 *
 * - st_mode: %jo muestra el valor numérico en octal.
 *   Para ls -l hay que interpretar sus bits y generar una
 *   cadena de tipo y permisos (ejemplo: -rw-r--r--).
 *
 * - st_mtim: no tiene un formato directo en printf().
 *   Se utiliza st_mtim.tv_sec para convertir la fecha mediante
 *   localtime_r() y después formatearla con strftime().
 *
 * - st_uid y st_gid: ls -l muestra normalmente nombres obtenidos
 *   con getpwuid() y getgrgid(), que se imprimen con %s.
 *   Los formatos %ju sirven para mostrar los ID numéricos.
 *
 * - st_blocks: se utiliza para calcular "total N", no se imprime
 *   como una columna individual del formato largo.
 *
 * - Para alinear columnas, printf() permite anchos dinámicos:
 *     %*ju  -> Número alineado a la derecha.
 *     %*jd  -> Número alineado a la derecha.
 *     %-*s  -> Texto alineado a la izquierda.
 *
 *   El argumento correspondiente al ancho debe ser de tipo int.
 * 
 * ANCHOS DE CAMPO EN PRINTF:
 *
 * %5d   -> Ancho fijo de 5 caracteres, alineado a la derecha.
 *          Ejemplo: printf("%5d", 12);
 *          Resultado: "   12" (3 espacios + 2 dígitos).
 *
 * %*d   -> Ancho dinámico, alineado a la derecha.
 *          El '*' indica que el ancho se obtiene del siguiente
 *          argumento de printf(), que debe ser de tipo int.
 *          Ejemplo: printf("%*d", 5, 12);
 *          Resultado: "   12".
 *
 * %-5d  -> Ancho fijo de 5 caracteres, alineado a la izquierda.
 *          Ejemplo: printf("%-5d", 12);
 *          Resultado: "12   ".
 *
 * %-*d  -> Ancho dinámico, alineado a la izquierda.
 *          Ejemplo: printf("%-*d", 5, 12);
 *          Resultado: "12   ".
 * 
 * * ALINEACIÓN DE COLUMNAS EN LS -L:
 *
 * Campo             Ejemplo          Alineación
 * ---------------------------------------------------------------
 * Tipo y permisos   -rw-rw-r--       Se representan mediante 10 caracteres de longitud fija.
 *                                    El especificador %jo imprime un número en octal (base 8)
 * Enlaces duros     1                Alineados a la derecha mediante %*ju
 * Propietario       gebravo          Alineados a la izquierda mediante %-*s
 * Grupo             gebravo          Alineados a la izquierda mediante %-*s
 * Tamaño            1                Alineados a la derecha mediante %*jd
 * Fecha             Nov  4 14:40     Formato de fecha específico. No utiliza los anchos calculados en t_print_widths
 * Nombre            archivo1         Sin relleno. Se imprime tal cual, sin ancho ni relleno adicional
 * 
 * gebravo@debian-hetzner:~/42_ft_ls$ ls -l t/d
 * total 12
 * -rw-rw-r-- 1 gebravo gebravo    1 Nov  4 14:40 archivo1
 * -rw-rw-r-- 1 gebravo gebravo 5000 Nov  4 14:40 archivo2
 */
void	print_long_format(t_ctx *ctx, const char *entry_name, const char *path, const struct stat *st, const t_print_widths *widths)
{
	char	permissions[11];
	char	date[32];
	char	*target;
	

	/* leer el destino antes de imprimir para evitar líneas incompletas */
	target = NULL;
	if (S_ISLNK(st->st_mode))
	{
		target = read_link_target(ctx, path, st);
		if (!target)
			return ;
	}
	
	/* 0.- print tipo y permisos */
	format_file_mode(st->st_mode, permissions);
	printf("%s ", permissions);

	/* 1.- print número de enlaces duros */
	printf("%*ju ", (int)widths->nlink, (uintmax_t)st->st_nlink);

	/* 2.- print propietario */
	print_file_owner(st, widths->owner);

	/* 3.- print grupo */
	print_file_group(st, widths->group);

	/* 4.- print tamaño del archivo o identificador del dispositivo */
	if (S_ISCHR(st->st_mode) || S_ISBLK(st->st_mode))
		printf("%*ju, %*ju ",
			(int)widths->major, (uintmax_t)major(st->st_rdev),
			(int)widths->minor, (uintmax_t)minor(st->st_rdev));
	else
		printf("%*jd ", (int)widths->size, (intmax_t)st->st_size);

	/* 5.- print fecha de modificación */
	format_file_time(st, &ctx->now, date);
	printf("%s ", date);
	
	/* 6.- print nombre */
	printf("%s", entry_name);

	/* 7.- print impresión del destino del enlace simbólico */
	if (target)
	{
		printf(" -> %s", target);
		free(target);
	}
	printf("\n");
}

/*
 * La función recibe el valor numérico de d_type
 * y devuelve el carácter que debemos imprimir en la primera posición del formato largo (-l)
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
 */
static char	get_dirent_type_char(unsigned char d_type)
{
	if (d_type == DT_REG)
		return ('-');
	if (d_type == DT_DIR)
		return ('d');
	if (d_type == DT_LNK)
		return ('l');
	if (d_type == DT_CHR)
		return ('c');
	if (d_type == DT_BLK)
		return ('b');
	if (d_type == DT_FIFO)
		return ('p');
	if (d_type == DT_SOCK)
		return ('s');
	return ('?');																				/* Si d_type == DT_UNKNOWN o recibimos un valor no reconocido, devolvemos '?' porque no podemos determinar el tipo y esta función se utiliza precisamente cuando lstat() ha fallado y, por tanto, no disponemos de metadatos válidos */
}

/*
 * Imprime una entrada en formato largo (-l) cuando lstat()
 * no ha proporcionado metadatos válidos (stat_ok == 0).
 *
 * Ejemplo de salida:
 *
 * l????????? ? ? ? ?            ? enlace
 *
 * - Utiliza d_type para representar el tipo de archivo,
 *   si se conoce. Si es desconocido, imprime '?'.
 * - Imprime '?' para los permisos y demás metadatos desconocidos.
 * - Imprime el nombre conservado desde readdir().
 * - No intenta consultar ni imprimir el destino de un enlace
 *   simbólico (-> destino).
 *
 * No accede a struct stat porque sus metadatos no son válidos.
 * 
 * El ejemplo de salida es ilustrativo: los espacios
 * dependen de los anchos calculados para cada listado.
 *
 * ALINEACIÓN DE LOS CAMPOS DESCONOCIDOS
 *
 * Los anchos almacenados en t_print_widths no corresponden
 * a una entrada concreta, sino a los anchos máximos de las
 * columnas para todo el listado del directorio.
 *
 * Se calculan previamente recorriendo las entradas cuyos
 * metadatos son válidos (stat_ok == 1).
 *
 * Por tanto, aunque lstat() haya fallado para esta entrada,
 * los anchos pueden ser mayores que 0 si existen otras
 * entradas con metadatos válidos en el mismo directorio.
 *
 * Utilizamos esos anchos para alinear los interrogantes (?)
 * con los valores de las demás entradas del listado.
 *
 * Ejemplo para la columna de enlaces duros:
 *
 * - widths->nlink == 3: '?' alineado a la derecha en 3 posiciones.
 * - widths->nlink == 0: '?' ocupa igualmente 1 posición.
 *
 * Si todas las entradas tienen stat_ok == 0, los anchos
 * conservan su valor inicial (0).
 * printf("%*c", 0, '?') sigue imprimiendo un carácter '?'.
 *
 * No es necesario modificar t_print_widths ni recalcular
 * los anchos: la propia impresión contempla el ancho mínimo
 * necesario para mostrar cada interrogante.
 *
 * La fecha utiliza un ancho fijo de 12 caracteres,
 * conforme al formato actual de fechas (LC_ALL=C).
 */
void	print_unknown_long_format(const char *entry_name, unsigned char d_type, const t_print_widths *widths)
{
	char	dirent_type;

	/* 0.- print tipo y permisos */
	dirent_type = get_dirent_type_char(d_type);
	printf("%c????????? ", dirent_type);														/* 1 carácter de tipo obtenido de d_type + 9 permisos desconocidos */

	/* 1.- print número de enlaces duros */
	/* ? -> st_nlink desconocido */
	printf("%*c ", (int)widths->nlink, '?');

	/* 2.- print propietario */
	/* ? -> st_uid desconocido (nombre o UID) */
	/* ¡! '-*' alineación a la izquierda */
	printf("%-*c ", (int)widths->owner, '?');

	/* 3.- print grupo */
	/* ? -> st_gid desconocido (nombre o GID) */
	/* ¡! '-*' alineación a la izquierda */
	printf("%-*c ", (int)widths->group, '?');

	/* 4.- print tamaño del archivo o identificador del dispositivo */
	/*
 	 * prepare_size_widths() ya ha ajustado 'widths->size' al ancho
 	 * necesario para tamaños normales e identificadores de dispositivos.
 	 *
 	 * Como no disponemos de st_size ni de major/minor, imprimimos
 	 * un único '?' alineado a la derecha en esa columna,
 	 * aunque d_type indique DT_CHR o DT_BLK.
 	*/
	printf("%*c ", (int)widths->size, '?');

	/* 5.- print fecha de modificación */
	/* ? -> st_mtim desconocido */
	printf("%12c ", '?');																/* la fecha utiliza un ancho fijo de 12 caracteres, conforme al formato actual de fechas (LC_ALL=C) */

	/* 6.- print nombre */
	/* entry_name -> nombre conservado desde readdir() */
	printf("%s", entry_name);
	
	/* 7.- print impresión del destino del enlace simbólico */
	/* No se imprime "-> destino": no disponemos de un destino consultado */

	printf("\n");
}
