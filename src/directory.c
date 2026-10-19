/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   directory.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 12:09:45 by gebravo           #+#    #+#             */
/*   Updated: 2026/10/19 17:10:30 by gebravo          ###   ########.fr       */
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
 * calcula la siguiente capacidad y comprueba posibles desbordamientos
 * SIZE_MAX: el mayor número de bytes que puede representar size_t
 * sizeof(t_directory_entry): los bytes que ocupa una entrada
 * SIZE_MAX / sizeof(t_directory_entry): el número máximo de entradas cuyo tamaño total podemos representar sin desbordamiento
 * new_capacity: el número de entradas para las que queremos reservar espacio
 */
static int calculate_capacity(size_t capacity, size_t *new_capacity)
{
	if (capacity == 0)																/* primera reserva */
		*new_capacity = 4;
	else
	{
		if (capacity > SIZE_MAX / 2)												/* evitar desbordamiento al duplicar capacity */
			return (1);
		*new_capacity = capacity * 2;
	}
	if (*new_capacity > (SIZE_MAX / sizeof(t_directory_entry)))						/* evitar desbordamiento al calcular los bytes para realloc() */
		return (1);

	return (0);
}

/*
 * 
 * lstat() u opendir() serán quienes se responsabilicen de comprobar que la ruta existe
 * 
 */
static char	*build_path(const char *name_directory, const char *name_entry)
{
	char	*path;
	size_t	len_directory;
	size_t	len_entry;
	size_t	size;

	len_directory = strlen(name_directory);
	len_entry = strlen(name_entry);

	if (len_entry > SIZE_MAX - 2
		|| len_directory > SIZE_MAX - len_entry - 2)								/* evitar desbordamiento al sumar longitudes, '/' y '\0' */
		return (NULL);

	size = len_directory + len_entry + 2;											/* +2 para caracteres '/' y '\0' */
	path = malloc(sizeof(char) * size);
	if (!path)
		return (NULL);
	snprintf(path, size, "%s/%s", name_directory, name_entry);

	return (path);
}

/*
 * Obtiene los metadatos de una entrada mediante lstat()
 * Solo se llamará cuando alguna opción necesite esos metadatos
 *
 * Devuelve:
 * 0: operación completada (aunque lstat haya fallado)
 * 1: error fatal de memoria
 */
static int	load_stat(t_ctx *ctx, const char *name_directory, t_directory_entry *entry)
{
	char	*path;

	path = build_path(name_directory, entry->name);									/* 'entry.name' contiene "clean.c", pero necesitamos "src/clean.c" para consultar el archivo correcto */
	if (!path)
	{
		ctx->fatal = FATAL_NOMEM;
		return (1);
	}
	if (lstat(path, &entry->st) == 0)
		entry->stat_ok = 1;
	else
	{
		fprintf(stderr, "%s: cannot access '%s': %s\n",
			ctx->prog, path, strerror(errno));
		ctx->status = 2;
	}
	free(path);
	return (0);
}

/*
 * Almacena una entrada del directorio en el array dinámico.
 *
 * - Amplía la capacidad cuando el array está lleno.
 * - Copia el nombre porque readdir() puede reutilizar su memoria.
 * - Obtiene metadatos solo si alguna opción los necesita.
 *
 * Devuelve:
 * 0: entrada almacenada.
 * 1: error fatal de memoria.
 */
static int	store_entry(t_ctx *ctx, const char *name_directory, const char *name_entry, t_directory_list *list) 
{
	t_directory_entry	*entries;
	size_t				new_capacity;

	if (list->count == list->capacity)												/* reservar o ampliar solo cuando el array esté vacio o lleno */
	{
		if (calculate_capacity(list->capacity, &new_capacity) == 1)
		{
			ctx->fatal = FATAL_NOMEM;
			return (1);
		}
		entries = realloc(list->items, sizeof(t_directory_entry) * new_capacity);
		if (!entries)
		{
			ctx->fatal = FATAL_NOMEM;
			return (1);
		}
		list->items = entries;
		list->capacity = new_capacity;
	}

	list->items[list->count].name = strdup(name_entry);								/* necesitamos copiar la cadena porque 'name' procede de readdir() y su memoria puede reutilizarse en la siguiente lectura */
	if (!list->items[list->count].name)
	{
		ctx->fatal = FATAL_NOMEM;
		return (1);
	}

	list->items[list->count].stat_ok = 0;											/* los metadatos no son válidos hasta que lstat() tenga éxito */

	if (ctx->opts.l || ctx->opts.t || ctx->opts.big_r)
	{
		if (load_stat(ctx, name_directory, &list->items[list->count]) == 1)
		{
			free(list->items[list->count].name);
			return (1);
		}
	}

	list->count++;
	return (0);
}

/*
 * Lee todas las entradas con readdir() y las guarda en list
 * Devuelve 0 si la lectura termina bien
 * Devuelve 1 si hay error de lectura (recuperable, ctx->status) o error de memoria (fatal, ctx->fatal)
 */
static int	read_directory(t_ctx *ctx, const char *name, DIR *dir, t_directory_list *list)
{
	struct dirent	*entry;

	errno = 0;
	entry = readdir(dir);
	while (entry)																	/* lee las entradas del directorio una a una hasta que readdir() devuelve NULL (no quedan entradas o hay un error) */
	{
		if (store_entry(ctx, name, entry->d_name, list) == 1)
			return (1);
		/* TODO eliminar el printf */
		printf("%s\n", entry->d_name);
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
 * LIST_DIRECTORY — Abrir y leer el contenido de un directorio
 *
 * 1. ABRIR EL DIRECTORIO: opendir()
 *
 *    #include <dirent.h>
 *    DIR *opendir(const char *name);
 *
 *    - name: ruta del directorio que queremos abrir
 *      En nuestro caso: operand->name
 *
 *    - DIR *: puntero al objeto que gestiona la lectura del directorio
 *      Lo guardamos en una variable:
 *
 *          DIR *dir;
 *          dir = opendir(operand->name);
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
 *    operand->name -> cadena con la ruta del directorio
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
 *    - Filtrar nombres ocultos según -a
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
void	list_directory(t_ctx *ctx, t_operand_info *operand)
{
	DIR					*dir;
	t_directory_list	list;
	int					flag_read;

	dir = open_directory(ctx, operand->name);
	if (!dir)
		return ;
	init_directory_list(&list);
	flag_read = read_directory(ctx, operand->name, dir, &list);
	if (flag_read == 0)
	{
		/* TODO: ordenar y mostrar list */
        /* TODO: recorrer subdirectorios si -R */
	}
	clean_directory_list(&list);
	if (closedir(dir) == -1)
	{
		fprintf(stderr, "%s: closing directory '%s': %s\n",
			ctx->prog, operand->name, strerror(errno));
		ctx->status = 2;
	}
}
