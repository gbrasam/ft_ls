/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sort_directory.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/15 16:37:54 by gebravo           #+#    #+#             */
/*   Updated: 2026/10/30 14:15:05 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

/*
 * Compara dos entradas de directorio por su nombre.
 *
 * qsort() entrega punteros genéricos (const void *)
 * Los convertimos a punteros t_directory_entry para
 * poder acceder al campo name.
 *
 * strcmp() devuelve:
 * - Un valor negativo si a->name va antes que b->name.
 * - Cero si los nombres son iguales.
 * - Un valor positivo si a->name va después que b->name.
 *
 * No modifica las entradas: únicamente las compara.
 */
static int	compare_directory_name(const void *a, const void *b)
{
	const t_directory_entry	*entry_a;
	const t_directory_entry	*entry_b;

	entry_a = (const t_directory_entry *)a;
	entry_b = (const t_directory_entry *)b;
	return (strcmp(entry_a->name, entry_b->name));
}

/*
 * El inodo almacena tres marcas de tiempo:
 * - atime: último acceso.
 * - mtime: última modificación del contenido.
 * - ctime: último cambio de estado del inodo (no es la creación).
 *
 * ls -t ordena por 'mtime'.
 * chmod modifica ctime, pero no mtime.
 *
 * st_mtim es una estructura timespec:
 * - tv_sec: segundos desde la época Unix.
 * - tv_nsec: nanosegundos adicionales dentro del segundo.
 *
 * Criterio de ordenación:
 * 1. Comparar tv_sec (más reciente primero).
 * 2. Si coinciden, comparar tv_nsec (más reciente primero).
 * 3. Si coinciden, desempatar por nombre con strcmp().
 *
 * El comparador devuelve un valor negativo si 'a' debe ir antes
 * que 'b', positivo si debe ir después y 0 si son equivalentes.
 */
static int compare_directory_time(const void *a, const void *b)
{
	
	
}

/*
 * Invierte in-place el orden de las entradas de directorio.
 *
 * Utiliza dos índices que avanzan desde los extremos
 * hacia el centro del array.
 *
 * En cada iteración intercambia dos estructuras completas,
 * conservando asociados el nombre y sus metadatos.
 *
 * No reserva memoria adicional: tmp es una variable local.
 *
 * Si count < 2, no hay nada que invertir.
 */
static void	reverse_directory_entries(t_directory_list *directory_list)
{
	t_directory_entry	tmp;
	size_t				left_index;
	size_t				right_index;

	if (directory_list->count < 2)
		return ;
	left_index = 0;
	right_index = directory_list->count - 1;
	while (left_index < right_index)
	{
		tmp = directory_list->items[left_index];
		directory_list->items[left_index] = directory_list->items[right_index];
		directory_list->items[right_index] = tmp;
		left_index++;
		right_index--;
	}
}

/*
 * Ordena las entradas de un directorio alfabéticamente.
 *
 * qsort() reorganiza las estructuras completas del array:
 * nombre, metadatos y stat_ok permanecen asociados.
 *
 * qsort() se encarga de reorganizar el array,
 * pero no sabe qué criterio de ordenación queremos utilizar;
 * por eso necesita una función comparadora.
 *
 * DECLARACIÓN:
 *
 * void qsort(void *base,
 *            size_t nmemb,
 *            size_t size,
 *            int (*compar)(const void *, const void *));
 *
 * Parámetros:
 * - base: dirección del primer elemento del array.
 * - nmemb: número de elementos del array.
 * - size: tamaño en bytes de cada elemento.
 * - compar: función que compara dos elementos y devuelve:
 *   < 0 si el primero debe ir antes.
 *   = 0 si ambos son equivalentes para la ordenación.
 *   > 0 si el primero debe ir después.
 *
 * qsort() recibe un array de estructuras, pero su comparador
 * trabaja con const void * porque es una función genérica:
 * puede ordenar enteros, cadenas, estructuras, etc.
 *
 * IMPORTANTE:
 * No estamos ordenando únicamente los nombres.
 * qsort() mueve las estructuras completas, de modo que los
 * metadatos siguen asociados al archivo correspondiente.
 *
 * ESTABILIDAD:
 * qsort() no garantiza que dos elementos cuyo comparador
 * devuelve 0 mantengan su orden original.
 *
 * En las entradas de un mismo directorio no supone un problema:
 * - Los nombres son únicos dentro de ese directorio.
 * - La comparación por nombre distingue entradas diferentes.
 * - Con -t, utilizaremos el nombre para desempatar cuando
 *   dos entradas tengan la misma fecha de modificación.
 *
 * Por tanto, el comparador no devolverá 0 para dos entradas
 * diferentes, siempre que utilicemos esos criterios.
 *
 * ESTADO ETAPA 4:
 * - Ordenación por nombre: IMPLEMENTADA.
 * - Orden inverso (-r): IMPLEMENTADO, pendiente de pruebas.
 * - Ordenación por fecha (-t): PENDIENTE.
 *
 * Primero se ordena mediante qsort().
 * Si la opción -r está activada, se invierte el array
 * mediante reverse_directory_entries().
 */
void	sort_directory_entries(t_ctx *ctx, t_directory_list *directory_list)
{
	if (directory_list->count < 2)
		return ;
	qsort(directory_list->items,
		directory_list->count,
		sizeof(t_directory_entry),
		compare_directory_name);
	if (ctx->opts.r == 1)
		reverse_directory_entries(directory_list);
}
