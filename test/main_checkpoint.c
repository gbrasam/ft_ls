/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main_checkpoint.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/11 09:21:08 by gebravo           #+#    #+#             */
/*   Updated: 2026/10/14 11:35:38 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "checkpoint.h"

/**
 * cc -Wall -Wextra -Werror -Iinclude \
	test/main_checkpoint.c \
	test/1_parse_args_checkpoint.c \
	src/parser.c \
	src/init.c \
	-o test/checkpoint
 * ./test/checkpoint
 * main(): recorre todos los casos de g_parse_cases
 *		   hasta encontrar el centinela {.id = NULL}
 *		   acumula esos resultados de la función run_case() en failed
 * run_case(): ejecuta y verifica un caso concreto g_parse_cases[i]
 * 			   (devuelve 0 si la prueba pasa y 1 si falla)
 */
int	main(void)
{
	int	i;
	int	failed;

	i = 0;
	failed = 0;
	while (g_parse_cases[i].id)
	{
		if (run_case(&g_parse_cases[i]))
		{
			printf("KO %s\n", g_parse_cases[i].id);
			failed++;
		}
		i++;
	}
	printf("%d casos, %d fallidos\n", i, failed);
	return (failed != 0);
}
