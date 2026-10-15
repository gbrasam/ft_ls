/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sort.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/15 16:37:54 by gebravo           #+#    #+#             */
/*   Updated: 2026/10/15 16:45:50 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

/*
 * SORT.C — ORDENACIÓN DE OPERANDOS
 *
 * OBJETIVO
 *
 * Ordenar los operandos antes de mostrarlos, respetando las
 * opciones activadas y la clasificación realizada previamente.
 *
 * 1. CRITERIOS DE ORDENACIÓN
 *
 * - Por defecto: ordenar por nombre, ascendente.
 * - Con -t: ordenar por fecha de modificación, más reciente primero.
 * - Con -r: invertir el orden correspondiente.
 * - Con -tr: ordenar por fecha, más antigua primero.
 *
 * 2. FUNCIONES PREVISTAS
 *
 * sort_operands():
 *   - Recibe el conjunto de operandos y las opciones.
 *   - Selecciona el criterio de ordenación.
 *   - Ordena los OPS_FILE entre sí.
 *   - Ordena los OPS_DIRECTORY entre sí.
 *   - Excluye los OPS_ERROR de la ordenación.
 *
 * compare_name():
 *   - Compara dos nombres.
 *   - Sirve tanto para archivos como para directorios.
 *
 * compare_time():
 *   - Compara las fechas de modificación de dos objetos.
 *   - Se utilizará con la opción -t.
 *
 * Las funciones de comparación podrán ser static si solo
 * se utilizan dentro de sort.c.
 *
 * 3. EJEMPLO
 *
 * Comando:
 * ./ft_ls dirZ z.txt dirA a.txt
 *
 * Array inicial:
 *
 *   0  dirZ    OPS_DIRECTORY
 *   1  z.txt   OPS_FILE
 *   2  dirA    OPS_DIRECTORY
 *   3  a.txt   OPS_FILE
 *
 * Objetivo: ordenar cada tipo sin mezclar archivos y directorios.
 *
 * Una posible disposición después de ordenar:
 *
 *   0  dirA    OPS_DIRECTORY
 *   1  a.txt   OPS_FILE
 *   2  dirZ    OPS_DIRECTORY
 *   3  z.txt   OPS_FILE
 *
 * Los tipos siguen intercalados.
 *
 * handle_non_directories() mostrará:
 *   a.txt
 *   z.txt
 *
 * handle_directories() procesará:
 *   dirA
 *   dirZ
 *
 * 4. INTEGRACIÓN
 *
 * handle_operands():
 *   1. sort_operands()
 *   2. handle_non_directories()
 *   3. handle_directories()
 *
 * La ordenación prepara los datos.
 * Las funciones handle_* recorren y procesan los resultados.
 *
 * 5. DECISIONES PENDIENTES
 *
 * - Elegir el algoritmo de ordenación.
 * - Decidir cómo ordenar elementos del mismo tipo cuando
 *   están intercalados con elementos de otros tipos.
 * - Determinar cómo obtener o almacenar st_mtim para -t.
 * - Resolver los empates de fecha mediante el nombre.
 * - Aplicar -r al criterio seleccionado.
 * - Reutilizar los criterios de comparación al ordenar
 *   las entradas obtenidas mediante readdir().
 *
 * IMPORTANTE
 *
 * No confundir:
 * - Clasificar: determinar OPS_FILE, OPS_DIRECTORY u OPS_ERROR.
 * - Ordenar: establecer el orden dentro de cada grupo.
 * - Procesar: mostrar archivos, informar errores o leer directorios.
 *
 * No implementar todavía.
 */

