/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/20 13:39:11 by gebravo           #+#    #+#             */
/*   Updated: 2026/10/20 13:40:48 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

char	*build_path(const char *name_directory, const char *name_entry)
{
	char	*path;
	size_t	len_directory;
	size_t	len_entry;
	size_t	size;

	len_directory = strlen(name_directory);
	len_entry = strlen(name_entry);

	if (len_entry > SIZE_MAX - 2
		|| len_directory > SIZE_MAX - len_entry - 2)								/* evitar desbordamiento al sumar longitudes, '/' y '\0' */
		return (NULL);

	size = len_directory + len_entry + 2;											/* +2 para caracteres '/' y '\0' */
	path = malloc(sizeof(char) * size);
	if (!path)
		return (NULL);
	snprintf(path, size, "%s/%s", name_directory, name_entry);

	return (path);
}
