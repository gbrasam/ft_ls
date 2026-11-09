/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   directory_entries.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 12:09:45 by gebravo           #+#    #+#             */
/*   Updated: 2026/11/09 17:50:25 by gebravo          ###   ########.fr       */
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
 * 'list_directory' se ocupa del ciclo de vida de un directorio concreto:
 *  |
 *  +-- init_directory_list()
 *  |
 *  +-- load_directory_entries()
 *  |       1. opendir()
 *  |       2. readdir() -> guardar entradas
 *  |       3. closedir()
 *  |
 *  +-- Si la carga ha tenido éxito:
 *  |       4. Imprimir separador de bloque (si ctx->printed)
 *  |       5. Imprimir cabecera (si ctx->headers)
 *  |       6. handle_directory_entries()
 *  |          -> ordenar, calcular anchos e imprimir entradas
 *  |
 *  |       7. Coordinar la recursividad -R:
 *  |          |
 *  |          +-- Si -R está activada:
 *  |              Recorrer las entradas ya ordenadas.
 *  |              |
 *  |              +-- Para cada subdirectorio válido:
 *  |                  - stat_ok == 1 y S_ISDIR().
 *  |                  - Excluir siempre "." y "..", incluso con -a.
 *  |                  - Respetar la visibilidad de -a.
 *  |                  - No seguir enlaces simbólicos:
 *  |                    lstat() + S_ISDIR() los excluye.
 *  |                  |
 *  |                  +-- Construir la ruta completa.
 *  |                  +-- Llamar a list_directory(ruta).
 *  |                  +-- Liberar la ruta reservada.
 *  |
 *  |          Primero se imprime todo el directorio actual.
 *  |          Después se recorren sus subdirectorios en el
 *  |          orden establecido por la ordenación activa (-t, -r).
 *  |
 *  +-- clean_directory_list()
 */
void	list_directory(t_ctx *ctx, const char *directory_name, bool is_operand)
{
	t_directory_list	directory_list;

	init_directory_list(directory_name, &directory_list);
	if (load_directory_entries(ctx, directory_name, &directory_list, is_operand) == 0)
	{
		if (ctx->printed)
			printf("\n");
		if (ctx->headers)
			printf("%s:\n", directory_name);
		handle_directory_entries(ctx, &directory_list);
		ctx->printed = true;
		if (ctx->opts.big_r && ctx->fatal == FATAL_NONE)
			handle_subdirectories(ctx, &directory_list);
	}
	clean_directory_list(&directory_list);
}
