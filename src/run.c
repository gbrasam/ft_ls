/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   run.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/14 13:21:21 by gebravo           #+#    #+#             */
/*   Updated: 2026/10/20 12:15:58 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

/*
 * Coordina la ejecución del listado según las opciones y operandos
 *
 * 1. PREPARAR Y CLASIFICAR OPERANDOS
 *    - Inicializar operand_list
 *    - Reservar memoria para todos los operandos
 *    - Si no hay operandos, reservar una entrada para "."
 *    - Clasificar todas las rutas antes de procesarlas
 *
 * 2. PROCESAR OPERANDOS
 *    - Informar de los operandos con error
 *    - Mostrar los archivos y otros objetos no directorio
 *    - Recorrer y mostrar los directorios
 *
 * 3. LISTAR DIRECTORIOS: delegado en handle_operands() -> list_directory() (directory.c)
 *
 * 4. CONTROLAR LA PRESENTACIÓN
 *    - Aplicar las opciones -l, -R, -a, -r y -t
 *    - Gestionar cabeceras y separadores entre bloques
 *    - Mostrar los archivos antes de los directorios
 *
 * 5. GESTIONAR ERRORES Y MEMORIA
 *    - Registrar los errores recuperables en ctx->status
 *    - Registrar los errores fatales en ctx->fatal
 *    - Ante un error fatal, detener el procesamiento
 *    - Liberar siempre los recursos locales reservados
 *
 *    main() llama a report_fatal(), que informa del error
 *    fatal y establece ctx->status = 2
 *
 * No realiza el parseo de argumentos
 * No imprime directamente los errores fatales
 */
void	run_ls(t_ctx *ctx, t_operands *ops)
{
	t_operand_list	operand_list;
	size_t			count;

	init_operand_list(&operand_list);
	count = ops->count;
	if (count == 0)
		count = 1;															/* necesitamos reservar espacio para un operando implícito: "." directorio actual */
	operand_list.items = malloc(sizeof(t_operand_info) * count);			/* ¡! array de estructuras; reservamos 'count' estructuras */
	if (!operand_list.items)
	{
		ctx->fatal = FATAL_NOMEM;
		return ;
	}
	operand_list.count = count;
	classify_operands(ctx, ops, &operand_list);
	handle_operands(ctx, &operand_list);
	/* handle_operands() puede producir un error fatal: no añadir ningún return entre malloc() y este free() */
	free(operand_list.items);
}
