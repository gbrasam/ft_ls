/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   display_directory_entries.c                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 12:09:45 by gebravo           #+#    #+#             */
/*   Updated: 2026/11/04 12:35:25 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

/*
 * Prepara los anchos máximos del formato largo (-l) y acumula
 * los bloques de almacenamiento de las entradas de un directorio.
 *
 * Ambas operaciones se realizan en un único recorrido.
 *
 * FILTRADO DE ENTRADAS:
 *
 * directory_list ya contiene únicamente las entradas admitidas
 * por el filtro -a, aplicado en read_directory().
 *
 * Sin -a, se excluyen las entradas cuyo nombre empieza por '.'.
 * Con -a, se incluyen también las ocultas, '.' y '..'.
 *
 * No es necesario volver a comprobar su visibilidad.
 *
 * METADATOS:
 *
 * Solo se consideran entradas con stat_ok == 1, porque ambas
 * operaciones necesitan acceder a los campos de struct stat.
 *
 * Las entradas sin metadatos válidos no contribuyen
 * a los anchos ni al total de bloques.
 *
 * CÁLCULO DE ANCHOS:
 *
 * update_print_widths() calcula el espacio necesario para
 * representar el número de enlaces, el propietario, el grupo
 * y el tamaño, conservando el máximo de cada columna.
 *
 * Los anchos representan caracteres de la salida impresa,
 * no el tamaño de los datos en memoria.
 *
 * CÁLCULO DEL TOTAL:
 *
 * st_blocks es un campo de struct stat, de tipo blkcnt_t.
 * Indica el espacio de almacenamiento asignado al archivo.
 *
 * En Linux, se expresa en bloques de 512 bytes,
 * independientemente del tamaño lógico (st_size).
 *
 * Ejemplo:
 *   Archivo A: st_blocks = 8
 *   Archivo B: st_blocks = 7
 *   Archivo C: st_blocks = 1
 *
 *   Suma acumulada: 16 bloques de 512 bytes.
 *
 * Se acumulan los valores originales de st_blocks.
 * No se convierte ni redondea cada entrada individualmente.
 *
 * La conversión a las unidades mostradas por ls debe
 * realizarse una sola vez, después de completar la suma.
 *
 * IMPORTANTE:
 *
 * El total corresponde a las entradas del directorio,
 * no a los archivos pasados individualmente como operandos.
 * Por eso no se calcula en prepare_files_widths().
 *
 * INICIALIZACIÓN:
 *
 * display_directory_entries() inicializa previamente:
 *
 * - Los anchos mediante init_print_widths().
 * - El acumulador total a cero.
 *
 * Esta función recibe ambos resultados por puntero
 * y actualiza sus valores durante el recorrido.
 *
 * Cada directorio mantiene sus propios anchos y total,
 * sin acumular resultados de directorios anteriores.
 *
 * RESPONSABILIDAD:
 *
 * Prepara la información necesaria para alinear las columnas
 * y mostrar el total de bloques.
 *
 * No imprime entradas, no convierte las unidades del total,
 * no aplica filtros de visibilidad ni ordena el listado.
 */
static void	prepare_entries_info(const t_directory_list *directory_list, t_print_widths *widths, blkcnt_t *total)
{
	size_t	i;

	i = 0;
	while (i < directory_list->count)
	{
		if (directory_list->items[i].stat_ok)
		{
			/* widths */
			update_print_widths(&directory_list->items[i].st, widths);
			/* total */
			*total += directory_list->items[i].st.st_blocks;
		}
		i++;
	}
}

static void	print_directory_entries(t_ctx *ctx, const t_directory_list *directory_list, const t_print_widths *widths)
{
	size_t	i;

	i = 0;
	while (i < directory_list->count)
	{
		if (ctx->opts.l)
		{
			/*
			* TODO paso 8: imprimir las entradas con stat_ok == 0
			*/
			if (directory_list->items[i].stat_ok)
			{
				/*
				* TODO paso 7: construir la ruta completa de los enlaces
				* simbólicos para readlink() y liberar la memoria reservada
				*/
				print_long_format(ctx, directory_list->items[i].name, NULL, &directory_list->items[i].st, widths);
			}
		}
		else
			printf("%s\n", directory_list->items[i].name);
		i++;
	}
}

/*
 * Coordina la preparación e impresión de las entradas
 * de un directorio previamente cargado y ordenado.
 *
 * ORDENACIÓN:
 *
 * Recibe directory_list ya ordenada por
 * sort_directory_entries(), llamada previamente desde
 * handle_directory_entries().
 *
 * Esta función no modifica el orden de las entradas.
 *
 * CON LA OPCIÓN -l:
 *
 * 1. Inicializa los anchos mediante init_print_widths()
 *    y el acumulador total a cero.
 *
 * 2. prepare_entries_info() calcula los anchos máximos
 *    y acumula st_blocks de las entradas con stat_ok == 1.
 *
 * 3. TODO: convertir e imprimir "total N".
 *
 * 4. print_directory_entries() imprime el formato largo.
 *
 * SIN LA OPCIÓN -l:
 *
 * Imprime únicamente los nombres de las entradas,
 * sin calcular los anchos ni el total de bloques.
 *
 * INDEPENDENCIA ENTRE DIRECTORIOS:
 *
 * Cada directorio tiene sus propios anchos y total.
 * Se inicializan de nuevo en cada llamada para evitar
 * que los resultados de un directorio afecten a otro.
 *
 * TOTAL DE BLOQUES:
 *
 * total es de tipo blkcnt_t, igual que st_blocks.
 * Representa el espacio de almacenamiento asignado,
 * no el tamaño lógico del archivo (st_size).
 *
 * La conversión a las unidades mostradas por ls se realiza
 * después de acumular los bloques de todas las entradas.
 *
 * RESPONSABILIDAD:
 *
 * Prepara los datos necesarios para la impresión
 * y muestra las entradas en el orden recibido.
 *
 * No abre directorios, no obtiene metadatos adicionales,
 * no ordena ni libera directory_list.
 */
static void	display_directory_entries(t_ctx *ctx, t_directory_list *directory_list)
{
	t_print_widths	widths;
	blkcnt_t		total;

	
	init_print_widths(&widths);
	total = 0;
	if (ctx->opts.l)
	{
		prepare_entries_info(directory_list, &widths, &total);
		/* TODO: convertir e imprimir "total N" */
	}
	print_directory_entries(ctx, directory_list, &widths);
}

void	handle_directory_entries(t_ctx *ctx, t_directory_list *directory_list)
{
	sort_directory_entries(ctx, directory_list);
	display_directory_entries(ctx, directory_list);
}

/*pendiente convertir e imprimir total N*/
/*pendiente construir la ruta completa para readlink().*/
