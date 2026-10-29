/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   directory.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 12:09:45 by gebravo           #+#    #+#             */
/*   Updated: 2026/10/29 13:10:59 by gebravo          ###   ########.fr       */
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
static int	read_directory(t_ctx *ctx, const char *name, DIR *dir, t_directory_list *list)
{
	struct dirent	*entry;

	errno = 0;
	entry = readdir(dir);
	while (entry)																	/* lee las entradas del directorio una a una hasta que readdir() devuelve NULL (no quedan entradas o hay un error) */
	{
		if (ctx->opts.a || entry->d_name[0] != '.')									/* aplicar filtro -a */
		{
			if (store_entry(ctx, name, entry->d_name, list) == 1)					/* guardar entradas válidas */
				return (1);
		}
		errno = 0;
		entry = readdir(dir);
	}
	if (errno != 0)																	/* error de lectura */
	{
		fprintf(stderr, "%s: reading directory '%s': %s\n",
			ctx->prog, name, strerror(errno));
		ctx->status = 2;
		return (1);
	}
	return (0);
}


/**
 * PROVISIONAL hasta Etapa 5
 */
static void	print_entries(const t_directory_list *list)
{
	size_t	i;

	i = 0;
	while (i < list->count)
	{
		printf("%s\n", list->items[i].name);
		i++;
	}
}

/**
 * LIST_DIRECTORY — Abrir y leer el contenido de un directorio
 *
 * 1. ABRIR EL DIRECTORIO: opendir()
 *
 *    #include <dirent.h>
 *    DIR *opendir(const char *name);
 *
 *    - name: ruta del directorio que queremos abrir
 *      En nuestro caso: path
 *
 *    - DIR *: puntero al objeto que gestiona la lectura del directorio
 *      Lo guardamos en una variable:
 *
 *          DIR *dir;
 *          dir = opendir(path);
 *
 *    - Si tiene éxito, devuelve un puntero DIR *
 *    - Si falla, devuelve NULL y establece errno
 *      Ejemplos: directorio inexistente o permisos insuficientes
 *
 *    IMPORTANTE:
 *    opendir() NO devuelve los nombres de los archivos
 *    Solo abre el directorio y prepara su lectura
 *
 *    Diferencia:
 *    path -> cadena con la ruta del directorio
 *    dir           -> puntero para leer ese directorio
 *
 *
 * 2. LEER LAS ENTRADAS: readdir()
 *
 *    struct dirent *readdir(DIR *dirp);
 *
 *    - dirp: puntero DIR * obtenido con opendir()
 *    - Devuelve un puntero struct dirent * a la siguiente entrada
 *    - Devuelve NULL cuando no quedan entradas o hay un error
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
 *    Comprobar que entry != NULL antes de acceder a d_name
 *
 *    Un directorio puede contener muchas entradas
 *    Debemos llamar repetidamente a readdir() hasta recibir NULL
 *
 *    Para distinguir fin de lectura de error:
 *    - errno == 0: fin del directorio
 *    - errno != 0: error de lectura
 *
 *    Debemos poner errno = 0 antes de la llamada cuyo
 *    resultado NULL vamos a comprobar
 *
 *    ATENCIÓN:
 *    Guardar entry NO significa conservar todas las entradas.
 *    readdir() puede reutilizar su memoria en llamadas posteriores.
 *    Para conservar nombres, necesitaremos copiarlos y almacenarlos
 *    en una colección propia -> "t_directory_list	list;"
 *
 *
 * 3. PROCESAMIENTO DE LAS ENTRADAS
 *
 *    IMPLEMENTADO:
 *    - Almacenar las entradas en t_directory_list mediante store_entry()
 *    - Obtener metadatos mediante store_entry() -> load_stat(),
 *      solo cuando están activadas las opciones -l, -t o -R
 *
 *    PENDIENTE:
 *    - Ordenar por nombre o fecha (-t), considerando -r
 *    - Mostrar las entradas (formato normal o -l)
 *    - Descender a subdirectorios si está activado -R
 *
 *    IMPORTANTE PARA -R:
 *    Primero procesamos y mostramos el directorio actual
 *    Después descendemos a sus subdirectorios
 *    Nunca debemos descender a "." ni a ".."
 *
 * 4. CERRAR Y LIBERAR RECURSOS
 *
 *    int closedir(DIR *dirp);
 *
 *    - Cierra el directorio abierto con opendir()
 *    - Devuelve 0 si tiene éxito y -1 si falla (establece errno)
 *    - Las entradas se liberan con clean_directory_list() antes de closedir()
 *
 *    Un fallo de apertura o lectura suele ser recuperable:
 *    informar por stderr, actualizar ctx->status y continuar
 *    con otros directorios cuando sea posible
 *
 *
 *	ctx->status = 2; error recuperable -> el programa informa del error, pero puede seguir procesando otros directorios
 */
void	list_directory(t_ctx *ctx, const char *path)
{
	DIR					*dir;
	t_directory_list	list;
	int					flag_read;

	dir = open_directory(ctx, path);
	if (!dir)
		return ;
	init_directory_list(&list);
	flag_read = read_directory(ctx, path, dir, &list);
	/* ¡! ETAPA 8: distinguir error de readdir() (recuperable:
	 * imprimir entradas ya leídas) de error de memoria (fatal:
	 * no imprimir). Actualmente flag_read == 1 agrupa ambos
	 */
	if (flag_read == 0)
	{
		print_entries(&list);
		/* TODO: ordenar Etapa 4 */
        /* TODO: recorrer subdirectorios si -R */
	}
	clean_directory_list(&list);
	if (closedir(dir) == -1)
	{
		fprintf(stderr, "%s: closing directory '%s': %s\n",
			ctx->prog, path, strerror(errno));
		ctx->status = 2;
	}
}
