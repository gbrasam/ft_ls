/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   directory_entries.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 12:09:45 by gebravo           #+#    #+#             */
/*   Updated: 2026/11/04 12:35:15 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

/*
 * list_directory() — nivel de directorio individual
 * Se ocupa del ciclo de vida de un directorio concreto:
 * 1. Inicializar su lista de entradas.
 * 2. Cargar sus entradas (fase 1).
 * 3. Ordenar, preparar y mostrar el listado (fase 2).
 * 4. Gestionar posteriormente la recursividad -R.
 * 5. Liberar la memoria.
 */
void	list_directory(t_ctx *ctx, const char *name)
{
	t_directory_list	directory_list;

	init_directory_list(&directory_list);
	if (load_directory_entries(ctx, name, &directory_list) == 0)
		handle_directory_entries(ctx, &directory_list);
	clean_directory_list(&directory_list);
}

/* TODO coordinar la recursividad -R */
