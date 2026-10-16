/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sort.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/15 16:37:54 by gebravo           #+#    #+#             */
/*   Updated: 2026/10/16 12:15:15 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

/*
 * strcmp("a.txt", "z.txt");  // Negativo
 * strcmp("z.txt", "a.txt");  // Positivo
 * strcmp("a.txt", "a.txt");  // Cero
 */
static int	compare_name(const char *a, const char *b)
{
	return (strcmp(a, b));
}

static void	sort_by_kind(t_operand_list *operand_list, t_operand_kind kind)
{
	int				i;
	int 			j;
	t_operand_info	tmp;

	i = 0;
	while (i < operand_list->count)
	{
		if (operand_list->items[i].kind == kind)
		{
			j = i + 1;
			while (j < operand_list->count)
			{
				if (operand_list->items[j].kind == kind)
				{
					if (compare_name(operand_list->items[i].name,
						operand_list->items[j].name) > 0)
					{
						tmp = operand_list->items[i];
						operand_list->items[i] = operand_list->items[j];
						operand_list->items[j] = tmp;
					}
				}
				j++;
			}
		}
		i++;
	}
}

/*
 * sort_operands():
 * - Recibe los operandos y las opciones mediante ctx.
 * - Ordena OPS_FILE y OPS_DIRECTORY por separado.
 * - Excluye OPS_ERROR de la ordenación.
 * - Aplica el criterio: nombre o fecha (-t).
 * - Invierte el orden si está activada -r.
 */
void	sort_operands(t_ctx *ctx, t_operand_list *operand_list)
{
	(void)ctx; /* TBD ¿si flags -r o -t activadas? */
	sort_by_kind(operand_list, OPS_FILE);
	sort_by_kind(operand_list, OPS_DIRECTORY);
	/*TODO
	Para completar la ordenación:
	1. -r: invertir el orden.
	2. -t: ordenar por fecha de modificación.
	3. Resolver empates de fecha.
	4. Revisar strcmp() frente a strcoll() para reproducir el orden de GNU ls.
	*/
}
