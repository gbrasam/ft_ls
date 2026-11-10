/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   load_directory_entries.c                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 12:09:45 by gebravo           #+#    #+#             */
/*   Updated: 2026/11/10 10:25:28 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

static DIR	*open_directory(t_ctx *ctx, const char *name, bool is_operand)
{
	DIR	*dir;

	dir = opendir(name);
	if (!dir)
	{
		report_error(ctx, is_operand, "cannot open directory", name, errno);
		return (NULL);
	}
	return (dir);
}

/*
 * Almacena una entrada del directorio en el array dinámico.
 *
 * - Amplía la capacidad cuando el array está lleno.
 * - Copia el nombre mediante strdup(), porque readdir()
 *   puede reutilizar su memoria.
 * - Guarda d_type para conservar el tipo indicado por readdir(),
 *   incluso si posteriormente falla lstat().
 * - Inicializa stat_ok a 0: los metadatos todavía no se han
 *   obtenido y puede que no sea necesario consultarlos.
 * - Obtiene los metadatos mediante lstat() cuando:
 *   - Está activada -l (formato largo).
 *   - Está activada -t (ordenación por fecha).
 *   - Está activada -R y d_type == DT_UNKNOWN,
 *     para determinar si la entrada es un directorio.
 * - Si d_type proporciona el tipo y solo está activada -R,
 *   no necesita llamar a lstat().
 * - Si d_type == DT_UNKNOWN y lstat() falla, la entrada
 *   no se recorrerá durante -R, porque no se puede
 *   confirmar que sea un directorio.
 * - Incrementa count únicamente si la entrada se ha almacenado.
 *
 * Retorno:
 *   0: entrada almacenada correctamente.
 *   1: error fatal de memoria.
 */
static int	store_entry(t_ctx *ctx, const char *directory_name, const char *entry_name, unsigned char entry_type, t_directory_list *directory_list) 
{
	t_directory_entry	*directory_entry;

	if (ensure_capacity(directory_list) == 1)																			/* reservar o ampliar solo cuando el array esté vacio o lleno */
	{
		ctx->fatal = FATAL_NOMEM;
		return (1);
	}

	directory_entry = &directory_list->items[directory_list->count];

	directory_entry->name = strdup(entry_name);																			/* necesitamos copiar la cadena porque 'name' procede de readdir() y su memoria puede reutilizarse en la siguiente lectura */
	if (!directory_entry->name)
	{
		ctx->fatal = FATAL_NOMEM;
		return (1);
	}

	directory_entry->stat_ok = 0;																						/* inicializamos stat_ok a 0 antes de llamar a lstat() (los metadatos no son válidos hasta que lstat() tenga éxito) */
	directory_entry->d_type = entry_type;																				/* conservar el tipo indicado por readdir() */
	if (ctx->opts.l || ctx->opts.t || (ctx->opts.big_r && directory_entry->d_type == DT_UNKNOWN))
	{
		if (load_stat(ctx, directory_name, directory_entry) == 1)
		{
			free(directory_entry->name);
			return (1);
		}
	}

	directory_list->count++;
	return (0);
}

/*
 * Lee las entradas de un directorio abierto mediante readdir().
 *
 * - Aplica el filtro de visibilidad según la opción -a.
 * - Almacena las entradas seleccionadas mediante store_entry().
 * - No ordena ni imprime las entradas.
 *
 * readdir() devuelve NULL cuando:
 * - Se alcanza el final del directorio (errno == 0).
 * - Se produce un error de lectura (errno != 0).
 *
 * errno se pone a 0 antes de cada llamada a readdir(),
 * porque otras funciones ejecutadas durante el bucle
 * pueden modificarlo.
 *
 * TRATAMIENTO DE ERRORES:
 *
 * 1. Si store_entry() devuelve 1:
 *    - Se ha producido un error fatal.
 *    - Se devuelve 1 para impedir la impresión.
 *
 * 2. Si readdir() falla:
 *    - Se informa del error.
 *    - is_operand determina su gravedad:
 *      · true: directorio recibido como operando (status = 2).
 *      · false: subdirectorio descubierto con -R (status = 1).
 *    - Nunca se rebaja un status = 2 anterior.
 *    - Se devuelve 0 para conservar e imprimir las entradas
 *      que ya se habían almacenado correctamente.
 *
 *    IMPORTANTE:
 *    El valor de retorno indica si podemos continuar utilizando
 *    las entradas almacenadas, no si toda la lectura tuvo éxito.
 *
 *    Ejemplo:
 *      readdir() -> archivo1 -> almacenado
 *      readdir() -> archivo2 -> almacenado
 *      readdir() -> ERROR
 *
 *    La función informa del error y actualiza ctx->status,
 *    pero devuelve 0 para que list_directory() pueda imprimir
 *    archivo1 y archivo2.
 *
 * RETORNO:
 * - 0: lectura completa o parcial aprovechable.
 * - 1: error fatal durante el almacenamiento.
 *
 * El cierre del directorio se realiza posteriormente
 * desde load_directory_entries().
 */
static int	read_directory(t_ctx *ctx, const char *directory_name, DIR *dir, t_directory_list *directory_list, bool is_operand)
{
	struct dirent	*entry;

	errno = 0;
	entry = readdir(dir);
	while (entry)																										/* lee las entradas del directorio una a una hasta que readdir() devuelve NULL (no quedan entradas o hay un error) */
	{
		if (ctx->opts.a || entry->d_name[0] != '.')																		/* aplicar filtro -a */
		{
			if (store_entry(ctx, directory_name, entry->d_name, entry->d_type, directory_list) == 1)					/* almacenar la entrada seleccionada ¡! una entrada puede almacenarse aunque lstat() falle y stat_ok quede a 0 */
				return (1);
		}
		errno = 0;
		entry = readdir(dir);
	}
	if (errno != 0)																										/* error de lectura */
		report_error(ctx, is_operand, "reading directory", directory_name, errno);
	return (0);
}

/*
 * Gestiona la apertura, lectura y cierre de un directorio.
 *
 * También imprime el separador y la cabecera del directorio
 * antes de leer sus entradas, pero no ordena ni imprime
 * las entradas almacenadas.
 *
 * 1. open_directory() -> opendir().
 *    - Si falla, informa del error y devuelve 1.
 *    - is_operand identifica el origen del directorio:
 *      · true: operando original de argv.
 *      · false: subdirectorio descubierto durante -R.
 *    - Gravedad del error:
 *      · Operando: status = 2.
 *      · Subdirectorio: status = 1.
 *      · Nunca reducir un status = 2 anterior.
 *
 * 2. Separador y cabecera.
 *    - Solo se imprimen si opendir() ha funcionado.
 *    - Si ctx->printed, imprime un salto de línea para
 *      separar este bloque del anterior.
 *    - Si ctx->headers, imprime "directory_name:".
 *    - Se imprimen ANTES de llamar a read_directory().
 *    - Así, los errores de lstat() de las entradas se generan
 *      después de la cabecera correspondiente.
 *
 * 3. read_directory() -> readdir() y store_entry().
 *    - Lee y almacena las entradas visibles según -a.
 *    - Si readdir() falla, informa del error y actualiza status,
 *      pero devuelve 0 para conservar las entradas ya leídas.
 *    - Si se produce un error fatal, devuelve 1.
 *
 * 4. closedir().
 *    - Se ejecuta siempre que opendir() haya tenido éxito,
 *      incluso si read_directory() devuelve un error fatal.
 *    - Evita mantener descriptores abiertos durante -R.
 *    - Si falla, informa del error y actualiza status
 *      según is_operand, sin rebajar un status = 2.
 *    - Un error de cierre no descarta las entradas leídas.
 *
 * RETORNO:
 * - 0: lectura completa o parcial aprovechable.
 * - 1: error de apertura o error fatal durante la lectura.
 *
 * Un error parcial de readdir() se registra en ctx->status,
 * pero no impide imprimir las entradas almacenadas.
 *
 * La ordenación, impresión de entradas y recursividad
 * se gestionan posteriormente desde list_directory().
 */
int	load_directory_entries(t_ctx *ctx, const char *directory_name, t_directory_list *directory_list, bool is_operand)
{
	DIR	*dir;
	int	flag_read;

	dir = open_directory(ctx, directory_name, is_operand);
	if (!dir)
		return (1);

	if (ctx->printed)
		printf("\n");
	if (ctx->headers)
		printf("%s:\n", directory_name);

	flag_read = read_directory(ctx, directory_name, dir, directory_list, is_operand);
	
	if (closedir(dir) == -1)																							/* closedir() se ejecuta después de cualquier lectura, incluso si read_directory() falla */
		report_error(ctx, is_operand, "closing directory", directory_name, errno);
	return (flag_read);
}
