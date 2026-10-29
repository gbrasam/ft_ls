/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   directory_entries.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/20 13:33:40 by gebravo           #+#    #+#             */
/*   Updated: 2026/10/20 13:40:45 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

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
int	store_entry(t_ctx *ctx, const char *name_directory, const char *name_entry, t_directory_list *list) 
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
