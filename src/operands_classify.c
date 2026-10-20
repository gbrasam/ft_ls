/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   operands_classify.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/20 12:10:27 by gebravo           #+#    #+#             */
/*   Updated: 2026/10/20 12:20:50 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

/*
 * Consulta los metadatos de una ruta y clasifica el operando
 * → consulta stat() y devuelve tipo/error
 *
 * Utiliza stat(), que sigue los enlaces simbólicos y consulta
 * los metadatos del objeto al que apuntan
 *
 * stat(const char *path, struct stat *buf):
 * - path: ruta que se quiere consultar
 * - buf: puntero a la estructura donde se guardan los metadatos
 * - Devuelve 0 si tiene éxito y -1 si falla (establece errno)
 *
 * struct stat st:
 * - st es la variable local que recibe los metadatos
 * - st.st_mode contiene el tipo de archivo y sus permisos
 * - S_ISDIR(st.st_mode) devuelve un valor distinto de 0
 *   si es un directorio, y 0 en caso contrario
 *
 * Resultado:
 * - OPS_ERROR: stat() falla; guarda errno en *error
 * - OPS_DIRECTORY: el objeto es un directorio
 * - OPS_FILE: cualquier otro tipo de objeto válido
 *
 * No imprime errores ni modifica ctx->status.
 */
static t_operand_kind	get_operand_kind(const char *name, int *error)
{
	struct stat st;

	if (stat(name, &st) == -1)
	{
		*error = errno;
		return (OPS_ERROR);	
	}
	if (S_ISDIR(st.st_mode) != 0)
		return (OPS_DIRECTORY);
	return (OPS_FILE);
}

/*
 * Clasifica todos los operandos recibidos por parse_args()
 * → guarda los resultados de todos los operandos
 *
 * La memoria de operand_list->items ya está reservada por run_ls().
 *
 * 1. Recorre las entradas de operand_list.
 * 2. Determina el nombre de cada operando:
 *    - Si no hay operandos explícitos, utiliza ".".
 *    - En caso contrario, conserva la ruta original de argv.
 * 3. Consulta cada ruta mediante get_operand_kind().
 * 4. Rellena name, kind y err de cada entrada.
 *
 * Modifica operand_list a través de su puntero.
 * No devuelve una colección nueva ni reserva memoria.
 *
 * No imprime ni ordena: conserva el orden original.
 */
void	classify_operands(t_ctx *ctx, t_operands *ops, t_operand_list *operand_list)
{
	size_t	i;

	(void)ctx;
	i = 0;
	while (i < operand_list->count)
	{
		if (ops->count == 0)
			operand_list->items[i].name = ".";
		else
			operand_list->items[i].name = ops->items[i];
		operand_list->items[i].err = 0;
		operand_list->items[i].kind = get_operand_kind(operand_list->items[i].name, &operand_list->items[i].err);
		i++;
	}
}
