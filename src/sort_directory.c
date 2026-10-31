/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sort_directory.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/15 16:37:54 by gebravo           #+#    #+#             */
/*   Updated: 2026/10/30 15:55:50 by gebravo          ###   ########.fr       */
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
 * 
 * st					→ struct stat
 * └── st_mtim			→ struct timespec
 *    ├── tv_sec		→ time_t			Segundos desde el 01/01/1970
 *    └── tv_nsec		→ long				Nanosegundos adicionales
 * 
 * tv_sec es de tipo time_t (normalmente 64 bits en tu Debian) → por tanto, puede representar valores mucho mayores que un int de 32 bits
 * El comparador de qsort() devuelve un int (normalmente 32 bits)
 * Si restas dos fechas muy alejadas, el resultado puede superar el rango de un int
 * Al convertirlo a int, podrías perder información e incluso obtener un signo incorrecto, provocando una ordenación errónea
 * 
 * tv_nsec es de tipo long (64 bits en tu Debian x86-64), pero sus valores válidos están comprendidos entre 0 y 999.999.999
 * por tanto, la resta de dos valores válidos de tv_nsec sí cabe en un int de 32 bits
 * sin embargo, es más claro y coherente utilizar comparaciones (< y >) para ambos campos
 * 
 * Si stat_ok == 0, significa que no tienes una fecha válida para comparar, pero la entrada sigue existiendo
 * Ambas válidas: comparar sus fechas normalmente
 * Una válida y otra no: colocar primero la válida
 * Ambas inválidas: ordenarlas por nombre
 * No se ignora ninguna entrada, simplemente cambias el criterio de comparación según stat_ok
 * ¡! No debes acceder a st_mtim si alguna entrada tiene stat_ok == 0
 */
static int compare_directory_time(const void *a, const void *b)
{
	const t_directory_entry	*entry_a;
	const t_directory_entry	*entry_b;

	entry_a = (const t_directory_entry *)a;
	entry_b = (const t_directory_entry *)b;
	if (entry_a->stat_ok == 1 && entry_b->stat_ok == 0)
		return (-1);
	if (entry_a->stat_ok == 0 && entry_b->stat_ok == 1)
		return (1);
	if (entry_a->stat_ok == 1 && entry_b->stat_ok == 1)
	{
		if (entry_a->st.st_mtim.tv_sec > entry_b->st.st_mtim.tv_sec)
			return (-1);
		if (entry_a->st.st_mtim.tv_sec < entry_b->st.st_mtim.tv_sec)
			return (1);
		if (entry_a->st.st_mtim.tv_nsec > entry_b->st.st_mtim.tv_nsec)
			return (-1);
		if (entry_a->st.st_mtim.tv_nsec < entry_b->st.st_mtim.tv_nsec)
			return (1);
	}
	return (compare_directory_name(a, b));
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
 * Ordena las entradas de un directorio según las opciones activadas.
 *
 * CRITERIOS DE ORDENACIÓN:
 * - Por defecto: orden alfabético por nombre.
 * - Con -t: orden por fecha de modificación (mtime), más reciente primero.
 * - Con -r: invierte el resultado de la ordenación elegida.
 *
 * COMBINACIONES:
 * - Sin opciones: nombre ascendente.
 * - -r: nombre descendente.
 * - -t: fecha descendente (más reciente primero).
 * - -tr: fecha ascendente (más antigua primero).
 *
 * Con -t, si dos entradas tienen exactamente el mismo mtime
 * (tv_sec y tv_nsec), se desempata por nombre mediante strcmp().
 *
 * FUNCIONAMIENTO DE qsort():
 *
 * qsort() reorganiza las estructuras completas del array:
 * nombre, metadatos y stat_ok permanecen asociados.
 *
 * qsort() se encarga de reorganizar el array, pero no sabe
 * qué criterio de ordenación queremos utilizar;
 * por eso necesita una función comparadora.
 *
 * DECLARACIÓN:
 *
 * void qsort(void *base,
 *            size_t nmemb,
 *            size_t size,
 *            int (*compar)(const void *, const void *));
 *
 * PARÁMETROS:
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
 * - Con -t, se utiliza el nombre para desempatar cuando
 *   dos entradas tienen la misma fecha de modificación.
 *
 * Por tanto, el comparador no devolverá 0 para dos entradas
 * diferentes, siempre que utilicemos esos criterios.
 *
 * ORDEN DE EJECUCIÓN:
 * 1. Si count < 2, no es necesario ordenar.
 * 2. Si -t está activada, qsort() utiliza compare_directory_time().
 * 3. En caso contrario, utiliza compare_directory_name().
 * 4. Si -r está activada, reverse_directory_entries()
 *    invierte el array después de qsort().
 *
 * Es importante aplicar -r después de qsort(), porque
 * una ordenación posterior podría deshacer la inversión.
 *
 * ESTADO ETAPA 4:
 * - Ordenación por nombre: IMPLEMENTADA Y PROBADA.
 * - Orden inverso (-r): IMPLEMENTADO Y PROBADO.
 * - Ordenación por fecha (-t): IMPLEMENTADA Y PROBADA EN DIRECTORIOS.
 * - Combinaciones -t, -tr, -ta y -tra: PROBADAS EN DIRECTORIOS.
 * - Ordenación por fecha de operandos: PENDIENTE.
 */
void	sort_directory_entries(t_ctx *ctx, t_directory_list *directory_list)
{
	if (directory_list->count < 2)
		return ;

	if (ctx->opts.t == 1)
		qsort(directory_list->items,
			directory_list->count,
			sizeof(t_directory_entry),
			compare_directory_time);
	else
		qsort(directory_list->items,
			directory_list->count,
			sizeof(t_directory_entry),
			compare_directory_name);

	if (ctx->opts.r == 1)
		reverse_directory_entries(directory_list);
}
