/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/20 13:39:11 by gebravo           #+#    #+#             */
/*   Updated: 2026/11/01 17:05:58 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

/*
 * Construye la ruta "directorio/entrada" en memoria dinámica.
 * ej.: ("src", "main.c") -> "src/main.c"
 *
 * Solo añade '/' si name_directory no termina ya en '/':
 *   ("src/", "main.c")    -> "src/main.c"
 *   ("src////", "main.c") -> "src////main.c"
 * Igual que GNU ls, solo se mira el último carácter
 * La ruta sigue siendo válida: el sistema trata varias '/'
 * seguidas como una sola al resolverla
 *
 * Devuelve la ruta o NULL si falla malloc()
 * o el tamaño desborda size_t.
 */
char	*build_path(const char *name_directory, const char *name_entry)
{
	char		*path;
	size_t		len_directory;
	size_t		len_entry;
	size_t		size;
	const char	*delimiter;

	len_directory = strlen(name_directory);
	len_entry = strlen(name_entry);

	if (len_entry > SIZE_MAX - 2 || len_directory > SIZE_MAX - len_entry - 2)			/* evitar desbordamiento al sumar longitudes, '/' y '\0' */
		return (NULL);
	
	if (len_directory > 0 && name_directory[len_directory - 1] == '/')
		delimiter = "";
	else
		delimiter = "/";

	size = len_directory + strlen(delimiter) + len_entry + 1;
	path = malloc(sizeof(char) * size);
	if (!path)
		return (NULL);
	snprintf(path, size, "%s%s%s", name_directory, delimiter, name_entry);
	return (path);
}
