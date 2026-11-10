/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   directory_entries.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 12:09:45 by gebravo           #+#    #+#             */
/*   Updated: 2026/11/10 09:30:51 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

static void	handle_subdirectories(t_ctx *ctx, t_directory_list *directory_list)
{
	char	*path;
	size_t	i;

	i = 0;
	while (i < directory_list->count)
	{
		if (ctx->fatal != FATAL_NONE)
			return ;
		if (((directory_list->items[i].stat_ok && S_ISDIR(directory_list->items[i].st.st_mode))
			|| (!directory_list->items[i].stat_ok && directory_list->items[i].d_type == DT_DIR))
			&& strcmp(directory_list->items[i].name, ".") != 0
			&& strcmp(directory_list->items[i].name, "..") != 0)
		{
			path = build_path(directory_list->directory_name, directory_list->items[i].name);
			if (!path)
			{
				ctx->fatal = FATAL_NOMEM;
				return ;
			}
			list_directory(ctx, path, false);
			free(path);
		}
		i++;
	}
}

/*
 * Función coordinadora del procesamiento de un directorio.
 *
 * list_directory() gestiona el procesamiento de un directorio
 * concreto y coordina su impresión y la recursividad con -R.
 *
 * 1. init_directory_list()
 *    - Inicializa la estructura donde se almacenarán las entradas.
 *
 * 2. load_directory_entries()
 *    - Abre el directorio mediante opendir()
 *    - Si la apertura funciona:
 *      · Imprime el separador de bloques si 'ctx->printed'
 *      · Imprime la cabecera si 'ctx->headers'
 *      · Lee y almacena las entradas mediante 'readdir()'
 *      · Cierra el directorio mediante 'closedir()'
 *    - Si opendir() falla, no imprime separador ni cabecera
 *
 * 3. Si la carga devuelve 0:
 *    - handle_directory_entries():
 *      · Ordena las entradas según las opciones activas.
 *      · Calcula los anchos y los bloques para -l.
 *      · Imprime las entradas.
 *
 *    - Marca ctx->printed = true para indicar que el bloque
 *      del directorio ya se ha procesado.
 *
 *    - Si -R está activada y no hay error fatal:
 *      · Recorre las entradas en el orden establecido.
 *      · Excluye siempre "." y "..", incluso con -a.
 *      · Respeta la visibilidad de -a.
 *      · No sigue enlaces simbólicos a directorios.
 *      · Construye la ruta completa de cada subdirectorio.
 *      · Llama recursivamente a list_directory().
 *      · Libera la ruta reservada.
 *
 *    Primero se imprimen las entradas del directorio actual.
 *    Después se procesan sus subdirectorios.
 *
 * 4. clean_directory_list()
 *    - Libera las entradas almacenadas, incluso si falla
 *      la carga del directorio.
 *
 * IMPORTANTE:
 * - El separador y la cabecera se imprimen ANTES de readdir().
 * - Los errores de lstat() de las entradas se producen
 *   después de la cabecera del directorio correspondiente.
 */
void	list_directory(t_ctx *ctx, const char *directory_name, bool is_operand)
{
	t_directory_list	directory_list;

	init_directory_list(directory_name, &directory_list);
	if (load_directory_entries(ctx, directory_name, &directory_list, is_operand) == 0)
	{
		handle_directory_entries(ctx, &directory_list);
		ctx->printed = true;
		if (ctx->opts.big_r && ctx->fatal == FATAL_NONE)
			handle_subdirectories(ctx, &directory_list);
	}
	clean_directory_list(&directory_list);
}
