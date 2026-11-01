/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   directory.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 12:09:45 by gebravo           #+#    #+#             */
/*   Updated: 2026/11/01 13:20:35 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

static DIR	*open_directory(t_ctx *ctx, const char *name)
{
	DIR	*dir;

	dir = opendir(name);
	if (!dir)
	{
		fprintf(stderr, "%s: cannot open directory '%s': %s\n",
			ctx->prog, name, strerror(errno));
		ctx->status = 2;
		return (NULL);
	}
	return (dir);
}

/*
 * Lee las entradas del directorio mediante readdir() y guarda
 * las seleccionadas en list
 *
 * Retorno:
 *   0: lectura completada correctamente
 *   1: error de lectura (recuperable, ctx->status) o
 *      error de memoria (fatal, ctx->fatal)
 *
 * struct dirent (<dirent.h>) representa una entrada del directorio
 * readdir() devuelve un puntero a esta estructura.
 *
 * Campos relevantes:
 *   d_name: nombre de la entrada (ej. "main.c")
 *   d_ino:  número de inodo
 *   d_type: tipo de entrada (DT_DIR, DT_REG, etc.)
 *
 * d_type puede valer DT_UNKNOWN, por lo que no siempre permite
 * determinar el tipo. Cuando sea necesario, se consulta mediante
 * stat() o lstat()
 *
 * readdir() devuelve NULL tanto al finalizar el directorio como
 * si se produce un error. Para distinguir ambos casos:
 *   - Se establece errno = 0 antes de llamar a readdir()
 *   - NULL y errno == 0: fin normal del directorio
 *   - NULL y errno != 0: error de lectura
 *
 * La memoria devuelta por readdir() no nos pertenece y puede
 * reutilizarse en llamadas posteriores. Por eso, store_entry()
 * copia el nombre con strdup() para conservarlo en list
 *
 * ctx->opts.a || entry->d_name[0] != '.'
 * ctx->opts.a vale 1 cuando el usuario ha indicado -a
 * entry->d_name es la cadena con el nombre devuelto por readdir()
 * entry->d_name[0] es el primer carácter de esa cadena
 *
 */
static int	read_directory(t_ctx *ctx, const char *name, DIR *dir, t_directory_list *directory_list)
{
	struct dirent	*entry;

	errno = 0;
	entry = readdir(dir);
	while (entry)																									/* lee las entradas del directorio una a una hasta que readdir() devuelve NULL (no quedan entradas o hay un error) */
	{
		if (ctx->opts.a || entry->d_name[0] != '.')																	/* aplicar filtro -a */
		{
			if (store_entry(ctx, name, entry->d_name, directory_list) == 1)											/* guardar entradas válidas */
				return (1);
		}
		errno = 0;
		entry = readdir(dir);
	}
	if (errno != 0)																									/* error de lectura */
	{
		fprintf(stderr, "%s: reading directory '%s': %s\n",
			ctx->prog, name, strerror(errno));
		ctx->status = 2;
		return (1);
	}
	return (0);
}

/* ------------------------------------------ handle_directory_entries ------------------------------------------ */

static void	prepare_entries_widths(t_directory_list *directory_list, t_print_widths *widths)
{
	(void)directory_list;
	(void)widths;
}


static void	print_entries(t_ctx *ctx, const t_directory_list *directory_list, const t_print_widths *widths)
{
	size_t	i;

	/*
 	 * Preparación del listado en formato largo (-l):
 	 * 1. Inicializar los anchos de columna para este directorio.
 	 * 2. Recorrer las entradas que se van a mostrar y cuyos metadatos
 	 *    sean válidos (stat_ok == 1), actualizando los anchos máximos.
 	 * 3. Calcular e imprimir el total de bloques del directorio.
 	 *
 	 * Las entradas con stat_ok == 0 no deben utilizarse para calcular
 	 * los anchos ni el total, aunque permanezcan en la lista.
 	 *
 	 * Cada directorio tiene sus propios anchos de columna y total de
 	 * bloques, que deben calcularse antes de imprimir sus entradas.
 	*/

	i = 0;
	while (i < directory_list->count)
	{
		if (ctx->opts.l)
		{
			if (directory_list->items[i].stat_ok)
				print_long_format(directory_list->items[i].name, &directory_list->items[i].st, widths);
		}
		else
			printf("%s\n", directory_list->items[i].name);
		i++;
	}
}

/*
 * TODO: Para -l, distinguir entre el nombre mostrado (clean.c)
 * y la ruta completa (src/clean.c), necesaria para readlink().
 * Inicializar los anchos.
 * Preparar los datos si está activado -l.
 * Imprimir el total de bloques.
 * Llamar a print_entries().
 */
static void	handle_directory_entries(t_ctx *ctx, t_directory_list *directory_list)
{
	(void)ctx;
	(void)directory_list;
}

/* -------------------------------------------------------------------------------------------------------------- */

/**
 * LIST_DIRECTORY — Abrir, leer y procesar el contenido de un directorio
 *
 * 1. ABRIR EL DIRECTORIO: opendir()
 *
 *    #include <dirent.h>
 *    DIR *opendir(const char *name);
 *
 *    - name: ruta del directorio que queremos abrir.
 *      En nuestro caso: path.
 *
 *    - DIR *: puntero al objeto que gestiona la lectura del directorio.
 *      Lo guardamos en una variable:
 *
 *          DIR *dir;
 *          dir = opendir(path);
 *
 *    - Si tiene éxito, devuelve un puntero DIR *.
 *    - Si falla, devuelve NULL y establece errno.
 *      Ejemplos: directorio inexistente o permisos insuficientes.
 *
 *    IMPORTANTE:
 *    opendir() NO devuelve los nombres de los archivos.
 *    Solo abre el directorio y prepara su lectura.
 *
 *    Diferencia:
 *    - path: cadena con la ruta del directorio.
 *    - dir: puntero que permite leer ese directorio.
 *
 *    En nuestro programa, la apertura se realiza mediante
 *    open_directory(), que también gestiona los errores.
 *
 *
 * 2. LEER LAS ENTRADAS: readdir()
 *
 *    #include <dirent.h>
 *    struct dirent *readdir(DIR *dirp);
 *
 *    - dirp: puntero DIR * obtenido mediante opendir().
 *    - Devuelve un puntero struct dirent * a la siguiente entrada.
 *    - Devuelve NULL cuando no quedan entradas o hay un error.
 *
 *    Para guardar el resultado:
 *
 *        struct dirent *entry;
 *        entry = readdir(dir);
 *
 *    Para consultar el nombre:
 *
 *        entry->d_name
 *
 *    IMPORTANTE:
 *    Comprobar que entry != NULL antes de acceder a d_name.
 *
 *    Un directorio puede contener muchas entradas.
 *    Debemos llamar repetidamente a readdir() hasta recibir NULL.
 *
 *    Para distinguir fin de lectura de error:
 *    - errno == 0: fin del directorio.
 *    - errno != 0: error de lectura.
 *
 *    Debemos poner errno = 0 antes de la llamada a readdir()
 *    cuyo resultado NULL vamos a comprobar.
 *
 *    ATENCIÓN:
 *    Guardar entry NO significa conservar todas las entradas.
 *    readdir() puede reutilizar su memoria en llamadas posteriores.
 *
 *    Para conservar los nombres, debemos copiarlos mediante
 *    strdup() y almacenarlos en una colección propia:
 *
 *        t_directory_list list;
 *
 *    En nuestro programa, read_directory() realiza la lectura
 *    y utiliza store_entry() para almacenar cada entrada.
 *
 *
 * 3. PROCESAR Y ALMACENAR LAS ENTRADAS
 *
 *    IMPLEMENTADO:
 *    - Filtrar nombres ocultos según la opción -a.
 *    - Almacenar las entradas en t_directory_list mediante
 *      store_entry().
 *    - Copiar los nombres con strdup(), porque la memoria
 *      devuelta por readdir() no debe conservarse directamente.
 *    - Obtener metadatos mediante store_entry() -> load_stat(),
 *      cuando están activadas las opciones -l, -t o -R.
 *    - Guardar los metadatos en cada t_directory_entry.
 *    - Utilizar stat_ok para indicar si son válidos.
 *
 *    PENDIENTE:
 *    - ETAPA 4: ordenar las entradas por nombre o fecha (-t),
 *      considerando también la opción -r.
 *    - ETAPA 5: implementar la presentación detallada con -l.
 *    - ETAPA 7: descender a subdirectorios cuando se active -R.
 *
 *    IMPORTANTE PARA -R:
 *    Primero procesamos y mostramos el directorio actual.
 *    Después descendemos a sus subdirectorios.
 *
 *    Nunca debemos descender a "." ni a "..".
 *
 *    Los metadatos obtenidos mediante lstat() permiten
 *    distinguir un directorio real de un enlace simbólico
 *    que apunta a un directorio.
 *
 *
 * 4. CERRAR EL DIRECTORIO: closedir()
 *
 *    #include <dirent.h>
 *    int closedir(DIR *dirp);
 *
 *    - Cierra el directorio abierto mediante opendir().
 *    - Devuelve 0 si tiene éxito y -1 si falla (establece errno).
 *    - Después de cerrarlo, no debemos volver a utilizar dir.
 *
 *    ORDEN ACTUAL:
 *
 *    1. open_directory()       -> abrir el directorio.
 *    2. init_directory_list()  -> inicializar la lista.
 *    3. read_directory()       -> leer y almacenar entradas.
 *    4. closedir()             -> cerrar el directorio.
 *    5. sort_directory_entries() -> ordenar las entradas.
 *    6. print_entries()        -> imprimir las entradas.
 *    7. Recursividad -R        -> pendiente.
 *    8. clean_directory_list() -> liberar la memoria.
 *
 *    IMPORTANTE:
 *    Cerramos el directorio ANTES de ordenar e imprimir.
 *
 *    Esto es posible porque ya hemos copiado los nombres
 *    y, cuando es necesario, los metadatos de las entradas.
 *
 *    Con -R evitaremos mantener abiertos simultáneamente
 *    los directorios de los distintos niveles del recorrido.
 *
 *    closedir() también debe ejecutarse cuando read_directory()
 *    falla, siempre que opendir() haya funcionado.
 *
 *
 * 5. LIBERAR LA MEMORIA: clean_directory_list()
 *
 *    Después de procesar las entradas, liberamos la memoria
 *    reservada para nuestra lista.
 *
 *    clean_directory_list() debe liberar:
 *    - Cada nombre copiado mediante strdup().
 *    - El array dinámico de t_directory_entry.
 *
 *    No debemos confundir dos recursos diferentes:
 *
 *    - DIR *dir:
 *      Recurso gestionado mediante opendir() y closedir().
 *
 *    - t_directory_list list:
 *      Memoria dinámica gestionada mediante store_entry()
 *      y clean_directory_list().
 *
 *    Cerrar dir NO libera automáticamente nuestra lista.
 *    Liberar la lista NO cierra el directorio.
 *
 *    Si open_directory() falla, no se ha inicializado la lista
 *    y la función retorna directamente.
 *
 *    Si read_directory() falla después de inicializarla,
 *    debemos cerrar dir y liberar la memoria almacenada.
 *
 *
 * 6. TRATAMIENTO DE ERRORES
 *
 *    ctx->status:
 *    Representa el código de salida global del programa.
 *
 *    - 0: sin errores.
 *    - 1: errores menores durante el procesamiento.
 *    - 2: errores de mayor gravedad.
 *
 *    Un error recuperable no implica necesariamente
 *    detener el programa: podemos informar por stderr
 *    y continuar con otros operandos o directorios.
 *
 *    ctx->fatal:
 *    Indica si se ha producido un error que impide
 *    continuar con el procesamiento normal.
 *
 *    Por ejemplo:
 *    - FATAL_NOMEM: fallo de reserva de memoria.
 *
 *    IMPORTANTE:
 *    ctx->status y ctx->fatal tienen responsabilidades
 *    diferentes y no deben confundirse.
 *
 *    PENDIENTE ETAPA 7 (-R):
 *    Distinguir los errores de directorios recibidos como
 *    operandos de argv de los errores de subdirectorios
 *    descubiertos durante el recorrido recursivo.
 *
 *    PENDIENTE ETAPA 8:
 *    - Distinguir un fallo recuperable de readdir()
 *      de un fallo fatal de memoria.
 *    - Aprovechar ctx->fatal para identificar el fallo
 *      de memoria sin añadir otro código de retorno.
 *    - Verificar frente a GNU ls el código de salida
 *      correspondiente a errores de closedir().
 *    - Revisar la gestión de errores de escritura.
 */
void	list_directory(t_ctx *ctx, const char *path)
{
	DIR					*dir;
	t_directory_list	directory_list;
	int					flag_read;

	/*
	 * workflow:
	 * 1. opendir()
	 * 2. readdir()  → guardar entradas
	 * 3. closedir() → se ejecuta antes de ordenar e imprimir, lo que evitará acumular descriptores abiertos durante la futura recursividad
	 * 4. ordenar entradas
	 * 5. imprimir entradas
	 * 6. recorrer subdirectorios con -R TODO
	 * 7. liberar memoria
	*/
	
	/*
 	 * ¡! ETAPA 7 (-R): distinguir el origen del directorio
 	 *
 	 * PROBLEMA ACTUAL:
 	 * list_directory() solo recibe directorios procedentes de
 	 * los operandos de argv.
 	 *
 	 * Cuando implementemos -R, esta misma función también
 	 * recibirá subdirectorios descubiertos durante el recorrido.
 	 *
 	 * Es necesario distinguir ambos casos porque GNU ls puede
 	 * asignar distinta gravedad a los errores según el origen
 	 * del directorio.
 	 *
 	 * PROPUESTA: añadir un parámetro is_operand:
 	 *
 	 * - is_operand = 1:
 	 *   El directorio procede directamente de argv.
 	 *   Ejemplo: ./ft_ls directorio
 	 *
 	 * - is_operand = 0:
 	 *   El directorio se ha descubierto durante el recorrido -R.
 	 *   Ejemplo: ./ft_ls -R directorio
 	 *   y posteriormente se procesa directorio/subdirectorio.
 	 *
 	 * CRITERIO PREVISTO:
 	 * - Error de opendir() en un operando: status = 2.
 	 * - Error de opendir() en un subdirectorio: status = 1.
 	 * - No sobrescribir un status = 2 anterior con status = 1.
 	 *
 	 * PENDIENTE DE VERIFICACIÓN:
 	 * Comprobar frente a GNU ls los códigos correspondientes
 	 * a los errores de opendir() y readdir() en ambos casos.
 	 * No asumir que ambos tipos de error tienen necesariamente
 	 * la misma gravedad sin comprobarlo.
 	 *
 	 * IMPLEMENTACIÓN FUTURA:
 	 * - Añadir is_operand como parámetro de list_directory().
 	 * - Pasar is_operand a open_directory() para que pueda
 	 *   determinar la gravedad de los errores de apertura.
 	 * - Utilizar también esta información al tratar los
 	 *   errores de lectura de read_directory().
 	 * - Actualizar las llamadas desde handle_directories()
 	 *   y desde el futuro recorrido recursivo.
 	 *
 	 * IMPORTANTE:
 	 * is_operand pertenece a cada llamada de list_directory(),
 	 * no al contexto global t_ctx.
 	 *
 	 * Durante una misma ejecución pueden procesarse tanto
 	 * operandos originales como subdirectorios descubiertos.
 	 * El origen debe conservarse individualmente.
 	 *
 	 * Este cambio queda pendiente hasta implementar -R.
  	 */
	dir = open_directory(ctx, path);
	if (!dir)
		return ;
	init_directory_list(&directory_list);
	flag_read = read_directory(ctx, path, dir, &directory_list);
	/*
	 * ¡! ETAPA 8: distinguir errores de lectura y errores de memoria
	 *
	 * PROBLEMA ACTUAL:
	 * read_directory() devuelve 0 si termina correctamente y 1 si falla,
	 * pero no distingue la causa del fallo:
	 *
	 * - flag_read = 0: lectura completa. Ordenar e imprimir las entradas
	 * - flag_read = 1: puede significar dos situaciones diferentes:
	 *
	 *   1. Error de readdir() (recuperable):
	 *      No se ha podido completar la lectura del directorio, pero
	 *      podemos ordenar e imprimir las entradas ya almacenadas
	 *      Debemos informar del error y actualizar ctx->status
	 *
	 *   2. Error de memoria (fatal):
	 *      Ha fallado una reserva de memoria (realloc(), strdup() o
	 *      build_path()). No debemos continuar el procesamiento normal
	 *      ctx->fatal = FATAL_NOMEM y debemos liberar la memoria reservada
	 *
	 * SOLUCIÓN PENDIENTE:
	 * Modificar read_directory() para distinguir ambos errores:
	 *
	 * - return (0): lectura completada correctamente.
	 * - return (1): error de lectura recuperable (readdir())
	 * - return (2): error fatal de memoria
	 *
	 * Estos valores son códigos internos de read_directory(), no deben
	 * confundirse con ctx->status, que representa el código de salida
	 * global del programa
	 *
	 * En list_directory():
	 * - Resultado 0: ordenar e imprimir
	 * - Resultado 1: informar del error, ordenar e imprimir lo ya leído
	 * - Resultado 2: no imprimir; liberar memoria y detener el
	 *   procesamiento normal
	 *
	 * TBD ctx->fatal, también podríamos utilizarlo para distinguir los errores
	 * sin introducir tres valores de retorno [ELEGIDO]
	 *
	 * En todos los casos debemos cerrar el directorio y liberar
	 * la memoria de la lista.
	*/
	if (closedir(dir) == -1)
	{
		fprintf(stderr, "%s: closing directory '%s': %s\n",
			ctx->prog, path, strerror(errno));
		/*
     		* ¡! ETAPA 8: verificar frente a GNU ls qué código de salida
     		* corresponde a un fallo de closedir()
     		*
     		* Actualmente se establece status = 2
     		* Comprobar si la gravedad depende de que el directorio sea
     		* un operando de argv o un subdirectorio encontrado con -R
     		* No sobrescribir un status = 2 anterior con uno menor
     	*/
		ctx->status = 2;
	}
	if (flag_read == 0)
	{
		sort_directory_entries(ctx, &directory_list);
		print_entries(ctx, &directory_list);
        /* TODO: Recursividad -R (pendiente) */
	}
	clean_directory_list(&directory_list);
}
