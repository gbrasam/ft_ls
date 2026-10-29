/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   operands_classify.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/20 12:10:27 by gebravo           #+#    #+#             */
/*   Updated: 2026/10/29 17:25:08 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

/*
 * Consulta los metadatos de una ruta y clasifica el operando.
 * Guarda el tipo, el error y los metadatos en t_operand_info.
 *
 * 1. PRIMERA COMPROBACIÓN: stat()
 *
 * Utiliza stat(), que sigue los enlaces simbólicos y consulta
 * los metadatos del objeto al que apuntan.
 *
 * stat(const char *path, struct stat *buf):
 * - path: ruta que se quiere consultar (info->name).
 * - buf: puntero a la estructura donde se guardan los metadatos (&info->st).
 * - Devuelve 0 si tiene éxito y -1 si falla (establece errno).
 *
 * Si stat() funciona:
 * - Guarda los metadatos directamente en info->st.
 * - Si el objeto es un directorio, lo clasifica como OPS_DIRECTORY.
 * - En cualquier otro caso, lo clasifica como OPS_FILE.
 *
 * 2. SEGUNDA COMPROBACIÓN: lstat() (solo en determinados errores)
 *
 * Si stat() falla, guarda primero errno en saved_errno, porque
 * una llamada posterior a lstat() podría modificar su valor.
 *
 * No todos los fallos de stat() significan que el operando no exista:
 *
 * - ENOENT: no se encuentra la ruta o el destino del enlace.
 *   Ejemplo: roto -> inexistente.txt.
 *   stat("roto") falla porque el destino no existe, pero el propio
 *   enlace simbólico "roto" sí puede existir.
 *
 * - ELOOP: se han encontrado demasiados enlaces simbólicos al
 *   resolver la ruta, por ejemplo, porque forman un ciclo.
 *   El propio enlace puede seguir existiendo.
 *
 * - EACCES: permiso denegado al acceder a algún componente
 *   de la ruta. No debemos tratarlo como un enlace roto.
 *
 * Solo si stat() falla con ENOENT o ELOOP se intenta lstat().
 *
 * lstat() consulta los metadatos del propio archivo.
 * Si es un enlace simbólico, consulta los metadatos del enlace,
 * sin seguirlo hasta su destino.
 *
 * - Si lstat() funciona, el operando existe y se clasifica
 *   como OPS_FILE, conservando sus metadatos en info->st.
 * - Si lstat() también falla, se clasifica como OPS_ERROR.
 * - Para otros errores de stat(), como EACCES, no se llama
 *   a lstat() y se clasifica directamente como OPS_ERROR.
 *
 * En caso de OPS_ERROR, info->err conserva saved_errno,
 * es decir, el error original de stat().
 *
 * 3. METADATOS: struct stat (info->st)
 *
 * - Los metadatos se guardan directamente en la estructura
 *   del operando, sin utilizar una estructura temporal.
 * - info->st.st_mode contiene el tipo de archivo y sus permisos.
 * - S_ISDIR(info->st.st_mode) devuelve un valor distinto de 0
 *   si es un directorio, y 0 en caso contrario.
 * - info->stat_ok indica si los metadatos son válidos (1) o no (0).
 *
 * IMPORTANTE:
 * - Si stat() funciona sobre un enlace simbólico, info->st contiene
 *   los metadatos del destino.
 * - Si stat() falla y lstat() funciona, info->st contiene
 *   los metadatos del propio enlace.
 * - Nunca debemos consultar info->st si stat_ok = 0.
 *
 * 4. RESULTADO DE LA CLASIFICACIÓN
 *
 * - OPS_DIRECTORY:
 *   stat() funciona y el objeto consultado es un directorio.
 *   stat_ok = 1 y err = 0.
 *
 * - OPS_FILE:
 *   stat() funciona y el objeto no es un directorio, o bien
 *   stat() falla con ENOENT/ELOOP y lstat() funciona.
 *   Incluye los enlaces simbólicos rotos.
 *   stat_ok = 1 y err = 0.
 *
 * - OPS_ERROR:
 *   stat() falla con un error distinto de ENOENT/ELOOP,
 *   o bien falla con ENOENT/ELOOP y también falla lstat().
 *   stat_ok = 0 y err = saved_errno.
 *
 * 5. RESPONSABILIDAD DE LA FUNCIÓN
 *
 * No devuelve ningún valor: modifica directamente la estructura
 * t_operand_info recibida mediante su puntero.
 *
 * No imprime mensajes de error ni modifica ctx->status.
 * El tratamiento y la impresión de errores se realizan después,
 * durante el procesamiento de los operandos.
 *
 * PENDIENTE ETAPA 5 (-l):
 * Revisar la obtención de metadatos de operandos que sean enlaces
 * simbólicos, porque con -l se debe mostrar el propio enlace
 * y no los metadatos de su destino.
 */
static void	get_operand_info(t_operand_info	*info)
{
	int	error;

	if (stat(info->name, &info->st) == 0)
	{
		info->stat_ok = 1;
		info->err = 0;
		if (S_ISDIR(info->st.st_mode))
			info->kind = OPS_DIRECTORY;
		else
			info->kind = OPS_FILE;
	}
	else
	{
		error = errno;
		if ((error == ENOENT || error == ELOOP) && lstat(info->name, &info->st) == 0)
		{
			info->stat_ok = 1;
			info->err = 0;
			info->kind = OPS_FILE;
		}
		else
		{
			info->stat_ok = 0;
			info->err = error;
			info->kind = OPS_ERROR;
		}
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
