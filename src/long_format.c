/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   long_format.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/31 12:41:10 by gebravo           #+#    #+#             */
/*   Updated: 2026/11/01 18:30:45 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

/*
 * Actualiza los anchos máximos de las columnas de longitud variable
 * del formato largo (-l): enlaces, propietario, grupo y tamaño.
 *
 * Se llama una vez por cada entrada que se va a imprimir y cuyos
 * metadatos son válidos (struct stat).
 *
 * Compara el ancho necesario para cada columna con el máximo
 * acumulado y conserva el mayor.
 *
 * No recorre el listado, no imprime ni calcula el total de bloques.
 * Estas tareas corresponden a la preparación del listado.
 * 
 * IMPORTANTE: El ancho se refiere al número de caracteres de la
 * representación impresa, no al tamaño del dato en memoria.
 *
 * Por ejemplo, st_nlink puede imprimirse como 1, 12 o 125
 * (1, 2 o 3 caracteres), aunque su tipo ocupe un tamaño fijo
 * en memoria. Lo mismo ocurre con st_size.
 *
 * Por eso utilizamos size_t: cada campo almacena una cantidad
 * de caracteres, no el valor original del metadato.
 */
void	update_print_widths(const struct stat *st, t_print_widths *widths)
{
	(void)st;
	(void)widths;
}

/*
 TBD la suma de st_blocks de las entradas que se van a mostrar.
 */



/*
 * TODO: Definir la interfaz definitiva para la impresión en formato largo.
 *
 * - Enlaces simbólicos: distinguir entre el nombre mostrado (name)
 *   y la ruta completa (path), necesaria para readlink().
 *
 * - Fecha: utilizar una referencia temporal común (ctx->now),
 *   inicializada una sola vez, para aplicar la regla de los 6 meses.
 *
 * - Alineación: calcular previamente los anchos máximos de las
 *   columnas del listado y pasarlos a la función de impresión.
 *
 * Separar la preparación del listado de la impresión de cada entrada:
 * 1.- La preparación del listado debe hacerse antes de empezar a imprimir,
 *	   tanto para los archivos recibidos como argumentos
 *	   como para las entradas de un directorio
 *	   En directory.c → print_entries() antes del bucle
 *	   En operands_handle.c → handle_files() antes del bucle
 */
void	print_long_format(t_ctx *ctx, const char *name, const char *path, const struct stat *st, const t_print_widths *widths)
{
	(void)ctx;
	(void)name;
	(void)path;
	(void)st;
	(void)widths;
}
