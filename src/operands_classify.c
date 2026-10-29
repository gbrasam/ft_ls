/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   operands_classify.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/20 12:10:27 by gebravo           #+#    #+#             */
/*   Updated: 2026/10/29 13:05:18 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

/*
 * Consulta los metadatos de una ruta y clasifica el operando.
 * → utiliza stat() y guarda el tipo, error y metadatos en t_operand_info.
 *
 * Utiliza stat(), que sigue los enlaces simbólicos y consulta
 * los metadatos del objeto al que apuntan.
 *
 * stat(const char *path, struct stat *buf):
 * - path: ruta que se quiere consultar (info->name).
 * - buf: puntero a la estructura donde se guardan los metadatos (&info->st).
 * - Devuelve 0 si tiene éxito y -1 si falla (establece errno).
 *
 * struct stat info->st:
 * - Los metadatos se guardan directamente en la estructura del operando.
 * - info->st.st_mode contiene el tipo de archivo y sus permisos.
 * - S_ISDIR(info->st.st_mode) devuelve un valor distinto de 0
 *   si es un directorio, y 0 en caso contrario.
 * - info->stat_ok indica si los metadatos son válidos (1) o no (0).
 *
 * Resultado:
 * - OPS_ERROR: stat() falla; guarda errno en info->err y stat_ok = 0.
 * - OPS_DIRECTORY: es un directorio; stat_ok = 1 y err = 0.
 * - OPS_FILE: cualquier otro tipo de objeto válido; stat_ok = 1 y err = 0.
 *
 * No devuelve ningún valor: modifica directamente la estructura recibida.
 * No imprime errores ni modifica ctx->status.
 */
static void	get_operand_info(t_operand_info	*info)
{

	if (stat(info->name, &info->st) == -1)													/* la idea es que stat guarde los metadatos directamente en la estructura del operando */
	{
		info->stat_ok = 0;
		info->err = errno;
		info->kind = OPS_ERROR;
	}
	else
	{
		info->stat_ok = 1;
		info->err = 0;
		if (S_ISDIR(info->st.st_mode) != 0)
			info->kind = OPS_DIRECTORY;
		else
			info->kind = OPS_FILE;
	}
}

/*
 * Clasifica todos los operandos recibidos por parse_args().
 * → guarda el nombre, tipo, error y metadatos de cada operando.
 *
 * La memoria de operand_list->items ya está reservada por run_ls().
 *
 * 1. Recorre las entradas de operand_list.
 * 2. Determina el nombre de cada operando:
 *    - Si no hay operandos explícitos, utiliza ".".
 *    - En caso contrario, conserva la ruta original de argv.
 * 3. Llama a get_operand_info() para consultar cada ruta mediante stat().
 * 4. Rellena los campos de cada t_operand_info:
 *    - name: nombre o ruta original del operando.
 *    - kind: OPS_FILE, OPS_DIRECTORY u OPS_ERROR.
 *    - err: errno si stat() falla; 0 si tiene éxito.
 *    - st: metadatos obtenidos mediante stat() (válidos si stat_ok = 1).
 *    - stat_ok: 1 si stat() tiene éxito; 0 si falla.
 *
 * Modifica operand_list a través de su puntero.
 * No devuelve una colección nueva ni reserva memoria adicional.
 *
 * No imprime ni ordena: conserva el orden original de los operandos.
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
		get_operand_info(&operand_list->items[i]);
		i++;
	}
}
