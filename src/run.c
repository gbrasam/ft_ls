/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   run.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/14 13:21:21 by gebravo           #+#    #+#             */
/*   Updated: 2026/10/15 16:15:15 by gebravo          ###   ########.fr       */
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
		count = 1;															/* necesitamos reservar espacio para un operando implícito: "." */
	operand_list.items = malloc(sizeof(t_operand_info) * count);			/* ¡! array de estructuras, NO de caracteres; reservamos 'count' estructuras */
	if (!operand_list.items)
	{
		ctx->fatal = FATAL_NOMEM;
		return ;
	}
	operand_list.count = count;
	classify_operands(ctx, ops, &operand_list);
	handle_operands(ctx, &operand_list);
	free(operand_list.items); 
	/* Cuando run_ls() tenga otras operaciones después de classify_operands(),
	alguna podría producir un error fatal.
	En ese caso, no debes hacer un return que se salte este free  */
}
