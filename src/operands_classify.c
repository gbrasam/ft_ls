/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   operands_classify.c                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/20 12:10:27 by gebravo           #+#    #+#             */
/*   Updated: 2026/10/31 12:10:50 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

/*
 * Consulta los metadatos de una ruta y clasifica el operando.
 * Guarda el tipo, el error y los metadatos en t_operand_info.
 *
 * 1. PRIMERA COMPROBACIÓN: stat()
 *
 * stat() sigue los enlaces simbólicos y consulta los metadatos
 * del objeto al que apuntan.
 *
 * stat(const char *path, struct stat *buf):
 * - path: ruta que se quiere consultar (info->name).
 * - buf: dirección donde se guardan los metadatos (&info->st).
 * - Devuelve 0 si tiene éxito y -1 si falla (establece errno).
 *
 * Si stat() funciona:
 * - Guarda inicialmente los metadatos en info->st.
 * - Si es un directorio, lo clasifica como OPS_DIRECTORY
 *   y conserva esos metadatos.
 * - En otro caso, utiliza lstat() para obtener los metadatos
 *   del propio operando y clasificarlo como OPS_FILE.
 * - Si lstat() falla, lo clasifica como OPS_ERROR.
 *
 * 2. SEGUNDA COMPROBACIÓN: lstat()
 *
 * lstat() consulta los metadatos del propio operando.
 * Si es un enlace simbólico, no sigue el enlace hasta su destino.
 *
 * Se utiliza en dos situaciones:
 * - Si stat() funciona y el operando no es un directorio.
 * - Si stat() falla con ENOENT o ELOOP.
 *
 * Si stat() falla, se guarda primero errno en error, porque
 * una llamada posterior a lstat() podría modificar su valor.
 *
 * No todos los fallos de stat() significan que el operando no exista:
 *
 * - ENOENT: no se encuentra la ruta o el destino del enlace.
 *   Ejemplo: roto -> inexistente.txt.
 *   stat("roto") falla porque el destino no existe, pero el
 *   propio enlace simbólico "roto" sí puede existir.
 *
 * - ELOOP: se han encontrado demasiados enlaces simbólicos al
 *   resolver la ruta, por ejemplo, porque forman un ciclo.
 *   El propio enlace puede seguir existiendo.
 *
 * - EACCES: permiso denegado al acceder a algún componente
 *   de la ruta. No debemos tratarlo como un enlace roto.
 *
 * Si stat() falla, solo se intenta lstat() cuando el error
 * es ENOENT o ELOOP.
 *
 * - Si lstat() funciona, se clasifica como OPS_FILE y
 *   conserva los metadatos del propio operando en info->st.
 * - Si lstat() falla, se clasifica como OPS_ERROR.
 * - Para otros errores de stat(), como EACCES, no se llama
 *   a lstat() y se clasifica directamente como OPS_ERROR.
 *
 * Si stat() había fallado, info->err conserva su error original.
 * Si stat() funcionó pero lstat() falla, se conserva errno
 * correspondiente al fallo de lstat().
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
 * - OPS_DIRECTORY conserva los metadatos obtenidos con stat().
 * - OPS_FILE guarda los metadatos obtenidos con lstat().
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
 *   En ambos casos, lstat() debe funcionar.
 *   Incluye los enlaces simbólicos rotos.
 *   stat_ok = 1 y err = 0.
 *
 * - OPS_ERROR:
 *   stat() falla con un error distinto de ENOENT/ELOOP,
 *   o falla con ENOENT/ELOOP y también falla lstat(),
 *   o stat() funciona pero el posterior lstat() falla.
 *   stat_ok = 0 y err = error correspondiente.
 *
 * 5. TRATAMIENTO DE ENLACES SIMBÓLICOS (-t)
 *
 * Para los operandos, GNU ls distingue según el destino:
 *
 * - Enlace a fichero: utiliza los metadatos del propio enlace.
 * - Enlace a directorio: utiliza los metadatos del destino.
 *
 * Por tanto:
 * - stat() permite clasificar el operando siguiendo el enlace.
 * - OPS_DIRECTORY: conserva los metadatos obtenidos con stat().
 * - OPS_FILE: utiliza lstat() para guardar los metadatos
 *   del propio operando, sin seguir enlaces simbólicos.
 *
 * lstat() también funciona con archivos normales y devuelve
 * los mismos metadatos que stat() cuando no hay enlaces simbólicos.
 *
 * Podemos sustituir los metadatos obtenidos con stat() por
 * los de lstat() para todos los OPS_FILE, sin necesidad de
 * identificar expresamente los enlaces simbólicos.
 *
 * 6. PENDIENTE ETAPA 5: FORMATO LARGO (-l)
 *
 * Actualmente, los enlaces simbólicos a directorios se siguen
 * y se clasifican como OPS_DIRECTORY.
 *
 * Con -l, sin opciones que fuercen el seguimiento de enlaces,
 * GNU ls muestra el propio enlace como operando, sin recorrer
 * el directorio de destino.
 *
 * Habrá que adaptar la clasificación según las opciones
 * y utilizar los metadatos del propio enlace.
 */
static void	get_operand_info(t_operand_info	*info)
{
	int	error;

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
 * No devuelve una colección nueva ni reserva memoria adicional
 *
 * No imprime ni ordena: conserva el orden original de los operandos
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
