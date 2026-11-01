/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sort_operand.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/15 16:37:54 by gebravo           #+#    #+#             */
/*   Updated: 2026/11/01 12:55:38 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

/*
 * Compara dos operandos por su nombre.
 *
 * qsort() entrega punteros genéricos (const void *).
 * Los convertimos a t_operand_info para acceder a name.
 *
 * strcmp() es seguro también para OPS_ERROR porque
 * el nombre del operando sigue siendo válido.
 *
 * No consulta struct stat ni necesita comprobar stat_ok.
 */
static int	compare_operand_name(const void *a, const void *b)
{
	const t_operand_info	*operand_a;
	const t_operand_info	*operand_b;

	operand_a = (const t_operand_info *)a;
	operand_b = (const t_operand_info *)b;
	return (strcmp(operand_a->name, operand_b->name));
}

/*
 * Compara dos operandos para ordenarlos por fecha de modificación (-t).
 *
 * qsort() entrega dos punteros genéricos (const void *).
 * Se convierten a const t_operand_info * para acceder
 * a sus metadatos y a su nombre.
 *
 * CRITERIO DE ORDENACIÓN:
 *
 * 1. Comprobar stat_ok antes de acceder a struct stat.
 *
 *    - Ambos válidos: comparar sus fechas.
 *    - Uno válido y otro inválido: colocar primero el válido.
 *    - Ambos inválidos: ordenar por nombre.
 *
 * 2. Si ambos tienen metadatos válidos:
 *
 *    - Comparar st_mtim.tv_sec (segundos).
 *    - Si coinciden, comparar st_mtim.tv_nsec (nanosegundos).
 *    - La fecha más reciente debe aparecer primero.
 *
 * 3. Si las fechas son idénticas, desempatar por nombre
 *    mediante compare_operand_name().
 *
 * RELACIÓN ENTRE stat_ok Y OPS_ERROR:
 *
 * En la implementación actual de get_operand_info():
 *
 * - stat_ok == 1: OPS_FILE u OPS_DIRECTORY.
 * - stat_ok == 0: OPS_ERROR.
 *
 * stat_ok indica si los metadatos pueden consultarse.
 * kind indica cómo debe procesarse el operando.
 *
 * No es necesario comprobar kind en este comparador,
 * porque stat_ok ya determina si st_mtim es válido.
 *
 * Los errores ya se han mostrado mediante handle_error()
 * antes de ejecutar sort_operands().
 *
 * VALORES DEVUELTOS:
 *
 * - Negativo: el primer operando debe aparecer antes.
 * - Positivo: el primer operando debe aparecer después.
 * - Cero: ambos son equivalentes según el comparador.
 *
 * No se restan las fechas para evitar problemas de
 * rango al convertir time_t a int.
 *
 * La opción -r se aplica después de qsort(), por lo que
 * este comparador siempre establece el orden normal de -t.
 */
static int compare_operand_time(const void *a, const void *b)
{
	const t_operand_info	*operand_a;
	const t_operand_info	*operand_b;

	operand_a = (const t_operand_info *)a;
	operand_b = (const t_operand_info *)b;
	if (operand_a->stat_ok == 1 && operand_b->stat_ok == 0)
		return (-1);
	if (operand_a->stat_ok == 0 && operand_b->stat_ok == 1)
		return (1);
	if (operand_a->stat_ok == 1 && operand_b->stat_ok == 1)
	{
		if (operand_a->st.st_mtim.tv_sec > operand_b->st.st_mtim.tv_sec)
			return (-1);
		if (operand_a->st.st_mtim.tv_sec < operand_b->st.st_mtim.tv_sec)
			return (1);
		if (operand_a->st.st_mtim.tv_nsec > operand_b->st.st_mtim.tv_nsec)
			return (-1);
		if (operand_a->st.st_mtim.tv_nsec < operand_b->st.st_mtim.tv_nsec)
			return (1);
	}
	return (compare_operand_name(a, b));
}

/*
 * Invierte in-place el orden de los operandos.
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
static void	reverse_operands(t_operand_list *operand_list)
{
	t_operand_info	tmp;
	size_t			left_index;
	size_t			right_index;

	if (operand_list->count < 2)
		return ;
	left_index = 0;
	right_index = operand_list->count - 1;
	while (left_index < right_index)
	{
		tmp = operand_list->items[left_index];
		operand_list->items[left_index] = operand_list->items[right_index];
		operand_list->items[right_index] = tmp;
		left_index++;
		right_index--;
	}
}

/*
 * Ordena los operandos recibidos por argv.
 *
 * FUNCIONAMIENTO:
 *
 * 1. Si hay menos de dos operandos, no es necesario ordenar.
 *
 * 2. Utiliza qsort() con compare_operand_name() para ordenar
 *    los operandos por nombre mediante strcmp().
 *
 * 3. Si la opción -r está activada, llama a reverse_operands()
 *    para invertir el orden obtenido.
 *
 * TIPOS DE OPERANDOS:
 *
 * El array operand_list->items puede contener:
 *
 * - OPS_FILE: archivo u otro objeto válido.
 * - OPS_DIRECTORY: directorio válido.
 * - OPS_ERROR: operando cuyo acceso ha fallado.
 *
 * No es necesario agrupar archivos y directorios dentro del
 * array, porque handle_files() y handle_directories() recorren
 * los elementos y seleccionan los correspondientes según kind.
 *
 * ORDEN DE LOS ERRORES:
 *
 * GNU ls muestra los errores de los operandos en el orden
 * original de argv, independientemente de la ordenación.
 *
 * Para conservar este comportamiento, handle_operands()
 * llama a handle_error() antes de sort_operands().
 *
 * Así, los errores se imprimen antes de que qsort() o
 * reverse_operands() modifiquen el orden del array.
 *
 * Las estructuras OPS_ERROR permanecen en el array, pero
 * su posición posterior ya no afecta al orden de los errores
 * mostrados por pantalla.
 *
 * METADATOS DE LOS OPERANDOS:
 *
 * qsort() y reverse_operands() reorganizan estructuras
 * t_operand_info completas, no únicamente sus nombres.
 *
 * Por tanto, cada operando conserva asociados:
 * - name: nombre o ruta del operando.
 * - kind: tipo de operando.
 * - err: código de error, si corresponde.
 * - st: metadatos obtenidos mediante stat() o lstat().
 * - stat_ok: indica si los metadatos son válidos.
 *
 * En los operandos OPS_ERROR, stat_ok = 0 y los metadatos
 * de struct stat no deben consultarse.
 *
 * La ordenación actual por nombre es segura para estos
 * operandos porque strcmp() únicamente utiliza name,
 * que sigue siendo válido.
 *
 * ORDENACIÓN CON qsort():
 *
 * qsort() recibe:
 * - La dirección del primer elemento del array.
 * - El número de elementos.
 * - El tamaño de cada estructura t_operand_info.
 * - La función comparadora compare_operand_name().
 *
 * El comparador determina qué elemento debe ir antes,
 * mientras que qsort() se encarga de reorganizar el array.
 *
 * qsort() no garantiza estabilidad: dos elementos para los
 * que el comparador devuelve 0 podrían cambiar de posición.
 *
 * Esto no afecta al orden de los errores mostrados, porque
 * ya se han procesado antes de realizar la ordenación.
 *
 * OPCIÓN -r:
 *
 * La opción -r no modifica el criterio del comparador.
 *
 * Primero se obtiene el orden normal mediante qsort().
 * Después, reverse_operands() invierte el array in-place,
 * intercambiando estructuras completas desde los extremos
 * hacia el centro.
 *
 * De esta manera, la inversión es independiente del criterio
 * utilizado para ordenar y podrá reutilizarse con -t.
 *
 * OPCIÓN -t:
 *
 * Actualmente no está implementada en esta función.
 *
 * Cuando se incorpore, será necesario comparar las fechas
 * de modificación de los operandos válidos, teniendo en
 * cuenta segundos y nanosegundos.
 *
 * También deberá evitarse consultar struct stat cuando
 * stat_ok = 0 y establecer un criterio de comparación
 * coherente para los operandos OPS_ERROR.
 *
 * La inversión mediante -r seguirá realizándose después
 * de la ordenación, sin necesidad de modificar
 * reverse_operands().
 */
void	sort_operands(t_ctx *ctx, t_operand_list *operand_list)
{
	if (operand_list->count < 2)
		return ;
	if (ctx->opts.t)
		qsort(operand_list->items,
			operand_list->count,
			sizeof(t_operand_info),
			compare_operand_time);
	else
		qsort(operand_list->items,
			operand_list->count,
			sizeof(t_operand_info),
			compare_operand_name);
	if (ctx->opts.r)
		reverse_operands(operand_list);
}
