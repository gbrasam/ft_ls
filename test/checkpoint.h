/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   checkpoint.h                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/11 09:35:12 by gebravo           #+#    #+#             */
/*   Updated: 2026/10/14 11:35:25 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHECKPOINT_H
#define CHECKPOINT_H

#include "ft_ls.h"

#define MAX_ARGS	8

/*
 * t_case: entrada y resultados esperados de una prueba de parse_args().
 *
 * id       Identificador de la prueba (P01, E01...).
 * args     Argumentos simulados, sin argv[0]. Terminan en NULL.
 *          Máximo: MAX_ARGS - 1 argumentos reales.
 * ret      Indica si parse_args() ha terminado correctamente o con error. 0 = correctamente; -1 = fallo.
 * fatal    Identifica el tipo de error esperado. FATAL_OPTION = Se espera que el fallo sea por una opción inválida; FATAL_NONE = No se espera ningún error
 * flags    Opciones que deben estar activadas (ej.: "la").
 *          NULL indica que no se comprueban.
 * bad_char Carácter inválido esperado en una opción corta.
 *			'\0' si no se comprueba.
 * 			Por ejemplo, './ft_ls -lz'
 *			{"E02", {"-lz", NULL}, -1, FATAL_OPTION, NULL, 'z', 0, 0, {-1}},
 * 			'z' reconoce l, pero no reconoce z
 * bad_long 1 si el error esperado es ARG_LONG; 0 si es ARG_SHORT.
 *			Por ejemplo, './ft_ls --foo
 *			{"E08", {"--foo", NULL}, -1, FATAL_OPTION, NULL, '\0', 1, 1, {-1}},
 * 			bad_char = '\0'	cuando no corresponde comprobarlo.
 *			bad_long = 1	'--foo' esperamos ARG_LONG
 * ops      Posiciones de argv que deben identificarse como operandos.
 *          Terminan en -1.
 *          {-1} indica que no hay operandos esperados.
 *
 *          Ejemplo sin operandos:
 *          {"E02", {"-lz", NULL}, -1, FATAL_OPTION, NULL, 'z', 0, 0, {-1}},
 *
 *          Ejemplo con un operando:
 *          ./ft_ls d1 -l
 *          argv[0] = "./ft_ls", argv[1] = "d1", argv[2] = "-l"
 *          args = {"d1", "-l", NULL}
 *          flags = "l"
 *          ops = {1, -1} -> argv[1] es el único operando.
 *
 *          Ejemplo con tres operandos:
 *          ./ft_ls a b c
 *          ops = {1, 2, 3, -1} -> argv[1], argv[2] y argv[3].
 */ 
typedef struct s_case
{
	const char	*id;
	const char	*args[MAX_ARGS];
	int			ret;
	t_fatal		fatal;
	const char	*flags;
	char		bad_char;
	int			bad_long;
	int			bad_long_pos;
	int			ops[MAX_ARGS];
}	t_case;

/*
 * extern -> declara un array definido en otro archivo .c
 * g_     -> variable global
 * parse  -> pruebas de parse_args()
 * cases  -> conjunto de casos de prueba
 */
extern const t_case	g_parse_cases[];

int	run_case(const t_case *test);

#endif
