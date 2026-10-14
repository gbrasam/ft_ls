/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   run.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/14 13:21:21 by gebravo           #+#    #+#             */
/*   Updated: 2026/10/14 16:50:50 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

/*
 * Consulta el tipo de objeto al que corresponde una ruta.
 *
 * - Si la consulta falla:
 *     Guarda errno en *error y devuelve OPS_ERROR.
 * - Si es un directorio:
 *     Devuelve OPS_DIRECTORY.
 * - En cualquier otro caso:
 *     Devuelve OPS_FILE.
 *
 * Utiliza stat() o lstat() según el tratamiento de enlaces simbólicos.
 * stat() sigue el enlace; lstat() examina el propio enlace.
 *
 * No imprime errores ni modifica ctx->status.
 */
static t_operand_kind	get_operand_kind(const char *name, int *error)
{
	/*la recomendación de copiar errno inmediatamente
	después de detectar el fallo es correcta y está bien fundamentada.*/

}

/*
 * Clasifica todos los operandos recibidos por parse_args().
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
static void	classify_operands(t_ctx *ctx, t_operands *ops, t_operand_list *operand_list)
{
	int	i;

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

/*
 * Coordina la ejecución del listado según las opciones y operandos.
 *
 * 1. PREPARAR Y CLASIFICAR OPERANDOS
 *    - Inicializar operand_list.
 *    - Reservar memoria para todos los operandos.
 *    - Si no hay operandos, reservar una entrada para ".".
 *    - Clasificar todas las rutas antes de procesarlas.
 *
 * 2. PROCESAR OPERANDOS
 *    - Informar de los operandos con error.
 *    - Mostrar los archivos y otros objetos no directorio.
 *    - Recorrer y mostrar los directorios.
 *
 * 3. LEER DIRECTORIOS
 *    - Abrir con opendir().
 *    - Leer entradas con readdir().
 *    - Aplicar el filtro de ocultos según -a.
 *    - Cerrar con closedir().
 *    - Ordenar y mostrar las entradas.
 *
 *    Antes de cada readdir(), establecer errno = 0.
 *    Si devuelve NULL:
 *      errno == 0: fin del directorio.
 *      errno != 0: error de lectura.
 *
 * 4. CONTROLAR LA PRESENTACIÓN
 *    - Aplicar las opciones -l, -R, -a, -r y -t.
 *    - Gestionar cabeceras y separadores entre bloques.
 *    - Mostrar los archivos antes de los directorios.
 *
 * 5. GESTIONAR ERRORES Y MEMORIA
 *    - Registrar los errores recuperables en ctx->status.
 *    - Registrar los errores fatales en ctx->fatal.
 *    - Ante un error fatal, detener el procesamiento.
 *    - Liberar siempre los recursos locales reservados.
 *
 *    main() llama a report_fatal(), que informa del error
 *    fatal y establece ctx->status = 2.
 *
 * No realiza el parseo de argumentos.
 * No imprime directamente los errores fatales.
 */
void	run_ls(t_ctx *ctx, t_operands *ops)
{
	t_operand_list	operand_list;
	int				count;

	init_operand_list(&operand_list);
	count = ops->count;
	if (count == 0)
		count = 1;	/* necesitamos reservar espacio para un operando implícito: "." */
	operand_list.items = malloc(sizeof(t_operand_info) * count);
	if (!operand_list.items)
	{
		ctx->fatal = FATAL_NOMEM;
		return ;
	}
	operand_list.count = count;
	classify_operands(ctx, ops, &operand_list);
	/* read_directory() */
	free(operand_list.items); 
	/* Cuando run_ls() tenga otras operaciones después de classify_operands(),
	alguna podría producir un error fatal.
	En ese caso, no debes hacer un return que se salte este free  */
}
