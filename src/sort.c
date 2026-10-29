/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sort.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/15 16:37:54 by gebravo           #+#    #+#             */
/*   Updated: 2026/10/29 17:42:54 by gebravo          ###   ########.fr       */
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
 * PENDIENTE ETAPA 4:
 * - Ordenar por fecha de modificación con -t.
 * - Invertir el orden con -r.
 */
void	sort_directory_entries(t_ctx *ctx, t_directory_list *directory_list)
{
	(void)ctx;
	qsort(directory_list->items,
		directory_list->count,
		sizeof(t_directory_entry),
		compare_directory_name);
}

/*
 * ETAPA 4: ordenar los operandos recibidos por argv.
 *
 * OBJETIVO:
 * Ordenar los operandos válidos por nombre o fecha de modificación,
 * teniendo en cuenta las opciones -t y -r.
 *
 * DIFERENCIA RESPECTO A sort_directory_entries():
 *
 * El array t_operand_list puede contener tres tipos:
 *
 * - OPS_FILE: archivo u otro objeto válido.
 * - OPS_DIRECTORY: directorio válido.
 * - OPS_ERROR: operando cuyo acceso ha fallado.
 *
 * No necesitamos agrupar archivos y directorios en el array,
 * porque handle_files() y handle_directories() ya recorren
 * los elementos filtrando por kind.
 *
 * ORDEN DE LOS ERRORES:
 *
 * GNU ls informa de los errores de operandos en el orden
 * original de argv, sin ordenarlos.
 *
 * qsort() no es estable: no garantiza conservar el orden
 * original de los elementos que el comparador considera iguales.
 *
 * SOLUCIÓN ADOPTADA:
 *
 * En handle_operands() llamamos primero a handle_error()
 * y después a sort_operands().
 *
 * De este modo, los errores se muestran en el orden original
 * de argv antes de que qsort() reorganice el array.
 *
 * IMPORTANTE: OPS_ERROR Y LOS METADATOS
 *
 * Aunque los errores ya se hayan mostrado, sus estructuras
 * siguen presentes en operand_list->items.
 *
 * Un operando OPS_ERROR tiene stat_ok = 0.
 * Sus metadatos info->st no son válidos y no deben consultarse.
 *
 * Comparar su nombre mediante strcmp() es seguro, porque
 * info->name sigue siendo válido.
 *
 * El riesgo aparece con -t: el comparador NO debe acceder
 * a st_mtim de un operando cuyo stat_ok sea 0.
 *
 * TRATAMIENTO DE OPS_ERROR:
 *
 * Propuesta: situar los errores al final del array.
 *
 * El comparador debe contemplar todas las combinaciones:
 *
 * - Error frente a válido:
 *   El error va después (resultado positivo).
 *
 * - Válido frente a error:
 *   El válido va antes (resultado negativo).
 *
 * - Error frente a error:
 *   Puede devolver 0 o comparar los nombres mediante strcmp().
 *   Su orden interno ya no afecta a la impresión de errores.
 *
 * - Válido frente a válido:
 *   Comparar por nombre o fecha, según las opciones activadas.
 *
 * IMPORTANTE:
 * Si se utiliza -r invirtiendo los argumentos del comparador,
 * también puede invertirse la posición de los errores.
 *
 * Esto no afecta a la salida porque handle_error() ya los
 * habrá mostrado antes de ejecutar sort_operands().
 *
 * COHERENCIA DEL COMPARADOR:
 *
 * qsort() necesita que el comparador mantenga un criterio
 * coherente para cualquier pareja de elementos.
 *
 * Por ejemplo:
 * - Si A va antes que B y B antes que C,
 *   A también debe ir antes que C.
 * - Si A va antes que B, B debe ir después que A.
 * - Si A y B se consideran equivalentes, el comparador
 *   debe tratar esa equivalencia de forma consistente.
 *
 * No debemos utilizar metadatos inválidos ni criterios
 * contradictorios que puedan romper estas propiedades.
 *
 * ORDENACIÓN CON qsort():
 *
 * qsort() reorganiza las estructuras t_operand_info completas.
 * Por tanto, cada nombre conserva asociados su kind, err,
 * stat_ok y struct stat.
 *
 * No debemos ordenar únicamente los punteros name ni
 * separar los metadatos de sus respectivos operandos.
 *
 * PLAN DE IMPLEMENTACIÓN:
 *
 * 1. Implementar el comparador por nombre mediante strcmp(),
 *    tratando explícitamente OPS_ERROR.
 *
 * 2. Utilizar qsort() para ordenar operand_list->items.
 *
 * 3. Comprobar que los errores se imprimen en orden de argv
 *    y que los archivos válidos se muestran ordenados,
 *    incluso cuando aparecen mezclados.
 *
 * 4. Incorporar -t:
 *    - Comparar las fechas de modificación (st_mtim).
 *    - Comparar primero los segundos y después los nanosegundos.
 *    - Desempatar por nombre cuando coincidan las fechas.
 *    - No consultar struct stat si stat_ok = 0.
 *
 * 5. Incorporar -r:
 *    - Invertir el criterio de ordenación.
 *    - Comprobar su combinación con -t.
 *
 * PRUEBAS PREVISTAS:
 *
 * ./ft_ls zz aa demo/b.txt demo/A.txt
 * ./ft_ls demo/b.txt zz demo/A.txt aa
 *
 * Comparar ambas órdenes con GNU ls:
 * - stdout.
 * - stderr.
 * - Código de salida.
 *
 * Utilizar LC_ALL=C para comparar el orden por nombre.
 * Las rutas de prueba deben existir cuando corresponda.
 *
 * ESTADO ACTUAL:
 * Pendiente de implementar la ordenación de operandos.
 */
void	sort_operands(t_ctx *ctx, t_operand_list *operand_list)
{
	(void)ctx;
	(void)operand_list;
	/* TODO: implementar ordenación de operandos */
	/* No agrupará archivos y directorios.
	Su comparador deberá tratar los errores sin acceder a metadatos inválidos */
	/* no hace falta que sort_operands() ordene los errores entre sí,
	porque ya estarán impresos. Solo necesitamos que el comparador mantenga un criterio coherente */
}