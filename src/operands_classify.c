/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   operands_classify.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/20 12:10:27 by gebravo           #+#    #+#             */
/*   Updated: 2026/11/09 18:30:20 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

/*
 * Consulta los metadatos de una ruta y clasifica el operando.
 * Guarda el tipo, el error y los metadatos en t_operand_info.
 *
 * 1. CASO ESPECIAL: ENLACE SIMBÓLICO CON -l
 *
 * - stat() sigue los enlaces simbólicos y consulta su destino.
 * - lstat() consulta el propio enlace, sin seguirlo.
 *
 * Si -l está activada:
 * - Se intenta lstat() antes de stat().
 * - Si lstat() funciona y S_ISLNK() confirma que es un enlace,
 *   se clasifica como OPS_FILE y se retorna sin llamar a stat().
 * - Se conservan los metadatos del propio enlace.
 * - Si no es un enlace o lstat() falla, continúa la clasificación.
 *
 * Ejemplo: enlace -> carpeta/
 * - Sin -l: se sigue el enlace y se lista la carpeta.
 * - Con -l: se muestra "enlace -> carpeta/".
 * - Con -l y enlace/: la barra final obliga a resolver el enlace
 *   y se lista el directorio de destino.
 *
 * 2. CLASIFICACIÓN GENERAL: stat()
 *
 * stat(const char *path, struct stat *buf):
 * - path: ruta que se consulta (info->name).
 * - buf: dirección donde se guardan los metadatos (&info->st).
 * - Devuelve 0 si funciona y -1 si falla (establece errno).
 *
 * Si stat() funciona:
 * - Guarda inicialmente los metadatos en info->st.
 * - Si es un directorio, lo clasifica como OPS_DIRECTORY
 *   y conserva los metadatos obtenidos con stat().
 * - Si no es un directorio, llama a lstat() para obtener los
 *   metadatos del propio operando y clasificarlo como OPS_FILE.
 * - Si lstat() falla, se clasifica como OPS_ERROR.
 *
 * 3. TRATAMIENTO DE ERRORES DE stat()
 *
 * Si stat() falla, se guarda inmediatamente errno en error,
 * porque una llamada posterior a lstat() podría modificarlo.
 *
 * No todos los fallos significan que el operando no exista:
 *
 * - ENOENT: no se encuentra la ruta o el destino del enlace.
 *   Ejemplo: roto -> inexistente.txt.
 *   stat("roto") falla, pero el propio enlace puede existir.
 *
 * - ELOOP: demasiados enlaces simbólicos al resolver la ruta,
 *   por ejemplo, porque forman un ciclo.
 *   El propio enlace puede seguir existiendo.
 *
 * - EACCES: permiso denegado al acceder a algún componente
 *   de la ruta. No se trata como un enlace roto.
 *
 * Si stat() falla con ENOENT o ELOOP:
 * - Se intenta lstat() para comprobar si el operando existe.
 * - Si funciona, se clasifica como OPS_FILE y se conservan
 *   los metadatos del propio operando.
 * - Si falla, se clasifica como OPS_ERROR.
 *
 * Si stat() falla por otro motivo, como EACCES:
 * - No se llama a lstat().
 * - Se clasifica directamente como OPS_ERROR.
 *
 * Si stat() falla y no se recupera con lstat(), info->err
 * conserva el error original de stat().
 * Si stat() funciona pero lstat() falla posteriormente,
 * info->err conserva el error de ese último lstat().
 *
 * 4. METADATOS: struct stat (info->st)
 *
 * - Los metadatos se guardan directamente en info->st.
 * - st_mode contiene el tipo de archivo y sus permisos.
 * - S_ISDIR(st_mode) identifica si es un directorio.
 * - S_ISLNK(st_mode) identifica si es un enlace simbólico.
 * - stat_ok indica si los metadatos son válidos (1) o no (0).
 *
 * IMPORTANTE:
 * - OPS_DIRECTORY conserva los metadatos de stat().
 * - OPS_FILE conserva los metadatos de lstat().
 * - Nunca consultar info->st si stat_ok == 0.
 *
 * 5. RESULTADO DE LA CLASIFICACIÓN
 *
 * OPS_DIRECTORY:
 * - stat() funciona y reconoce un directorio.
 * - Incluye enlaces a directorios que se siguen sin -l.
 * - stat_ok = 1 y err = 0.
 *
 * OPS_FILE:
 * - Con -l, lstat() reconoce un enlace simbólico.
 * - O stat() funciona, no es directorio y lstat() funciona.
 * - O stat() falla con ENOENT/ELOOP y lstat() funciona.
 * - Incluye archivos normales y enlaces simbólicos rotos.
 * - stat_ok = 1 y err = 0.
 *
 * OPS_ERROR:
 * - stat() falla con un error distinto de ENOENT/ELOOP.
 * - O stat() falla con ENOENT/ELOOP y también falla lstat().
 * - O stat() funciona pero el posterior lstat() falla.
 * - stat_ok = 0 y err = error correspondiente.
 *
 * 6. RELACIÓN CON LA ORDENACIÓN -t
 *
 * - OPS_DIRECTORY conserva los metadatos de stat(),
 *   que corresponden al directorio de destino si es un enlace.
 * - OPS_FILE conserva los metadatos de lstat(),
 *   que corresponden al propio archivo o enlace.
 *
 * Así, la ordenación -t utiliza los metadatos almacenados
 * según la clasificación del operando.
 *
 * El uso de -l modifica la clasificación de los enlaces
 * simbólicos a directorios: se muestran como OPS_FILE
 * en lugar de recorrer su destino.
 */
static void	get_operand_info(t_ctx *ctx, t_operand_info	*info)
{
	int	error;

	if (ctx->opts.l && lstat(info->name, &info->st) == 0 && S_ISLNK(info->st.st_mode))
	{
		info->stat_ok = 1;
		info->err = 0;
		info->kind = OPS_FILE;
		return ;
	}

	if (stat(info->name, &info->st) == 0)
	{
		info->stat_ok = 1;
		info->err = 0;
		if (S_ISDIR(info->st.st_mode))
		{
			info->kind = OPS_DIRECTORY;
			return ;
		}
		if (lstat(info->name, &info->st) == 0)
		{
			info->kind = OPS_FILE;
			return ;
		}
		info->stat_ok = 0;
		info->err = errno;
		info->kind = OPS_ERROR;
		return ;
	}
	error = errno;
	if ((error == ENOENT || error == ELOOP) && lstat(info->name, &info->st) == 0)
	{
		info->stat_ok = 1;
		info->err = 0;
		info->kind = OPS_FILE;
		return ;
	}
	info->stat_ok = 0;
	info->err = error;
	info->kind = OPS_ERROR;
}

/*
 * Clasifica todos los operandos recibidos por parse_args()
 * Guarda el nombre, tipo, error y metadatos de cada operando
 *
 * La memoria de operand_list->items ya está reservada por run_ls()
 *
 * 1. Recorre las entradas de operand_list
 * 2. Determina el nombre de cada operando:
 *    - Si no hay operandos explícitos, utiliza "."
 *    - En caso contrario, conserva la ruta original de argv
 * 3. Llama a get_operand_info() para consultar cada ruta
 *    mediante stat() y, cuando corresponde, lstat()
 * 4. Rellena los campos de cada t_operand_info:
 *    - name: nombre o ruta original del operando
 *    - kind: OPS_FILE, OPS_DIRECTORY u OPS_ERROR
 *    - err: código de error si falla la consulta; 0 si es válida
 *    - st: metadatos obtenidos mediante stat() o lstat()
 *    - stat_ok: 1 si los metadatos son válidos; 0 si no lo son
 *
 * METADATOS:
 * - OPS_DIRECTORY: conserva los obtenidos mediante stat()
 * - OPS_FILE: conserva los obtenidos mediante lstat()
 * - OPS_ERROR: no dispone de metadatos válidos
 *
 * Modifica operand_list a través de su puntero
 * No devuelve una colección nueva ni realiza reservas de memoria
 *
 * No imprime ni ordena: conserva el orden original de los operandos
 */
void	classify_operands(t_ctx *ctx, t_operands *ops, t_operand_list *operand_list)
{
	size_t	i;

	i = 0;
	while (i < operand_list->count)
	{
		if (ops->count == 0)
			operand_list->items[i].name = ".";
		else
			operand_list->items[i].name = ops->items[i];
		get_operand_info(ctx, &operand_list->items[i]);
		i++;
	}
}
