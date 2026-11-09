/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   directory_entries_utils.c                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/20 13:33:40 by gebravo           #+#    #+#             */
/*   Updated: 2026/11/09 17:50:30 by gebravo          ###   ########.fr       */
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
int calculate_capacity(size_t capacity, size_t *new_capacity)
{
	if (capacity == 0)																									/* primera reserva */
		*new_capacity = 4;
	else
	{
		if (capacity > SIZE_MAX / 2)																					/* evitar desbordamiento al duplicar capacity */
			return (1);
		*new_capacity = capacity * 2;
	}
	if (*new_capacity > (SIZE_MAX / sizeof(t_directory_entry)))															/* evitar desbordamiento al calcular los bytes para realloc() */
		return (1);

	return (0);
}

/*
 * Obtiene mediante lstat() los metadatos de cada entrada descubierta
 * con readdir() dentro de un directorio
 * 
 * Solo se llama cuando alguna opción requiere esos metadatos (-l, -t, -R)
 *
 * Devuelve:
 * 0: operación no fatal (aunque lstat() haya fallado)
 * 1: error fatal por fallo de memoria al construir la ruta
 * 
 * cuando falla lstat(), la función informa del error y actualiza ctx->status,
 * pero no interrumpe el recorrido del directorio
 */
int	load_stat(t_ctx *ctx, const char *directory_name, t_directory_entry *directory_entry)
{
	char	*path;

	path = build_path(directory_name, directory_entry->name);															/* entry->name contiene "clean.c"; construimos "src/clean.c" */
	if (!path)
	{
		ctx->fatal = FATAL_NOMEM;
		return (1);
	}
	if (lstat(path, &directory_entry->st) == 0)
		directory_entry->stat_ok = 1;
	else
	{
		fprintf(stderr, "%s: cannot access '%s': %s\n", ctx->prog, path, strerror(errno));
		if (ctx->status == 0)																							/* solo eleva el estado de 0 a 1, sin reducir un estado 2 anterior */
			ctx->status = 1;
	}
	free(path);
	return (0);
}

/*
 * garantizar que hay espacio para guardar una nueva entrada
 * solo se amplia la memoria cuando es necesario, no en todas las llamadas
 * - comprobar si queda espacio
 * - calcular la nueva capacidad
 * - reservar o ampliar memoria con realloc()
 * - actualizar list->items y list->capacity solo cuando realloc() tiene éxito
 *
 * ¡! si realloc() falla, list->items sigue apuntando a la memoria anterior,
 *    porque hemos utilizado el puntero temporal entries
 *    no se pierden las entradas que ya se habían guardado
 */
int	ensure_capacity(t_directory_list *directory_list)
{
	t_directory_entry	*entries;
	size_t				new_capacity;

	if (directory_list->count < directory_list->capacity)																/* no es necesario ampliar memoria */
		return (0);

	if (calculate_capacity(directory_list->capacity, &new_capacity) == 1)												/* desbordamiento */
		return (1);

	entries = realloc(directory_list->items, sizeof(t_directory_entry) * new_capacity);
	if (!entries)
		return (1);

	directory_list->items = entries;
	directory_list->capacity = new_capacity;
	return (0);
}
