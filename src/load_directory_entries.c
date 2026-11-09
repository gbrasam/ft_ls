/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   load_directory_entries.c                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 12:09:45 by gebravo           #+#    #+#             */
/*   Updated: 2026/11/09 17:50:28 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

static DIR	*open_directory(t_ctx *ctx, const char *name, bool is_operand)
{
	DIR	*dir;

	dir = opendir(name);
	if (!dir)
	{
		fprintf(stderr, "%s: cannot open directory '%s': %s\n", ctx->prog, name, strerror(errno));
		if (is_operand)
			ctx->status = 2;
		else if (ctx->status != 2)
			ctx->status = 1;
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
 * Lee las entradas del directorio mediante readdir() y guarda
 * las seleccionadas en list
 *
 * Retorno:
 *   0: lectura completada correctamente
 *   1: error de lectura (recuperable, ctx->status) o
 *      error de memoria (fatal, ctx->fatal)
 *
 * struct dirent (<dirent.h>) representa una entrada del directorio
 * readdir() devuelve un puntero a esta estructura.
 *
 * Campos relevantes:
 *   d_name: nombre de la entrada (ej. "main.c")
 *   d_ino:  número de inodo
 *   d_type: tipo de entrada (DT_DIR, DT_REG, etc.)
 *
 * d_type puede valer DT_UNKNOWN, por lo que no siempre permite
 * determinar el tipo de entrada.
 *
 * Guardamos d_type en nuestra estructura porque lstat() puede
 * fallar después de que readdir() haya encontrado la entrada.
 * Así conservamos el tipo indicado por readdir(), si se conoce.
 *
 * Para obtener los metadatos de las entradas utilizamos lstat().
 *
 * readdir() devuelve NULL tanto al finalizar el directorio como
 * si se produce un error. Para distinguir ambos casos:
 *   - Se establece errno = 0 antes de llamar a readdir()
 *   - NULL y errno == 0: fin normal del directorio
 *   - NULL y errno != 0: error de lectura
 *
 * La memoria devuelta por readdir() no nos pertenece y puede
 * reutilizarse en llamadas posteriores. Por eso, store_entry()
 * copia el nombre con strdup() para conservarlo en list
 * y guarda el valor de d_type en nuestra estructura.
 *
 * ctx->opts.a || entry->d_name[0] != '.'
 * ctx->opts.a vale 1 cuando el usuario ha indicado -a
 * entry->d_name es la cadena con el nombre devuelto por readdir()
 * entry->d_name[0] es el primer carácter de esa cadena
 *
 */
static int	read_directory(t_ctx *ctx, const char *directory_name, DIR *dir, t_directory_list *directory_list)
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
	{
		fprintf(stderr, "%s: reading directory '%s': %s\n",
			ctx->prog, directory_name, strerror(errno));
		ctx->status = 2;
		return (1);
	}
	return (0);
}

/*
 * 'load_directory_entries' gestiona la apertura, lectura y cierre
 * de un directorio, sin encargarse de ordenar ni imprimir.
 *
 * Workflow:
 * 1. open_directory() -> opendir().
 *    - Si falla, informa del error y devuelve 1.
 *    - is_operand distingue el origen del directorio:
 *      · true: operando original de argv.
 *      · false: subdirectorio descubierto durante -R.
 *    - Gravedad del error de apertura:
 *      · Operando: status = 2.
 *      · Subdirectorio: status = 1.
 *      · Nunca reducir un status = 2 anterior.
 *
 * 2. read_directory() -> readdir() y store_entry().
 *    - Lee y almacena las entradas visibles según -a.
 *    - Devuelve el resultado de la lectura.
 *
 * 3. closedir().
 *    - Se ejecuta incluso si read_directory() falla.
 *    - Evita mantener descriptores abiertos durante -R.
 *    - Si falla, informa del error y actualiza status
 *      según is_operand, sin rebajar un status = 2.
 *
 * Devuelve:
 * - 1 si falla la apertura.
 * - En otro caso, devuelve el resultado de read_directory().
 *
 * La ordenación, impresión y recursividad se gestionan
 * posteriormente desde list_directory().
 *
 * TODO (Etapa 8):
 * - Revisar el tratamiento de errores de readdir(),
 *   incluida la conservación de entradas leídas parcialmente.
 * - Verificar frente a GNU ls la gravedad de los errores
 *   de readdir() y closedir() en operandos y subdirectorios.
 */
int	load_directory_entries(t_ctx *ctx, const char *directory_name, t_directory_list *directory_list, bool is_operand)
{
	DIR	*dir;
	int	flag_read;

	dir = open_directory(ctx, directory_name, is_operand);
	if (!dir)
		return (1);
	flag_read = read_directory(ctx, directory_name, dir, directory_list);
	if (closedir(dir) == -1)																							/* closedir() se ejecuta después de cualquier lectura, incluso si read_directory() falla */
	{
		fprintf(stderr, "%s: closing directory '%s': %s\n", ctx->prog, directory_name, strerror(errno));
		if (is_operand)
			ctx->status = 2;
		else if (ctx->status != 2)
			ctx->status = 1;
		/* Pendiente de verificar-Equivalencia exacta con GNU ls ante un fallo de closedir() */
	}
	return (flag_read);
}

 /*
 * TODO — ETAPA 8: errores recuperables durante la lectura del directorio.
 *
 * PROBLEMA ACTUAL
 *
 * load_directory_entries() devuelve:
 * - 0: lectura completada y sin error fatal.
 * - 1: no se puede continuar con el listado.
 *
 * Actualmente, si readdir() falla después de haber leído algunas
 * entradas, se descartan de la impresión las entradas almacenadas.
 *
 * Sin embargo, hay que distinguir tres situaciones:
 *
 * 1. ERROR DE APERTURA — opendir()
 *    - No se ha podido abrir el directorio.
 *    - Informar del error y actualizar ctx->status.
 *    - No imprimir cabecera ni entradas.
 *    - No es un error fatal.
 *
 * 2. ERROR DE LECTURA — readdir()
 *    - El directorio se ha abierto correctamente.
 *    - La lectura se interrumpe antes de terminar.
 *    - Informar del error y actualizar ctx->status.
 *    - Conservar las entradas almacenadas hasta ese momento.
 *    - Ordenar e imprimir esas entradas y la cabecera, si procede.
 *    - No es un error fatal.
 *
 * 3. ERROR DE MEMORIA — FATAL_NOMEM
 *    - Falla una reserva de memoria durante el procesamiento
 *      (realloc(), strdup() o build_path()).
 *    - Establecer ctx->fatal = FATAL_NOMEM.
 *    - No continuar el procesamiento normal ni imprimir el listado.
 *    - Liberar los recursos reservados.
 *
 * DECISIÓN DE DISEÑO [ELEGIDA]
 *
 * Mantener ctx->fatal para identificar los errores fatales.
 * No introducir tres códigos de retorno en read_directory().
 *
 * ctx->status representa el estado de salida global.
 * ctx->fatal indica si debe detenerse el procesamiento normal.
 *
 * PENDIENTE DE RESOLVER
 *
 * Distinguir el fallo de opendir() del fallo recuperable de
 * readdir(), ya que ambos pueden ocurrir con ctx->fatal == FATAL_NONE.
 *
 * No basta con comprobar únicamente ctx->fatal.
 *
 * REVISAR list_directory()
 *
 * Actualmente solo procesa el listado cuando:
 *
 *     load_directory_entries(...) == 0
 *
 * Esta condición deberá adaptarse para permitir:
 *
 * - Apertura fallida: no imprimir cabecera ni entradas.
 * - Lectura completa: imprimir cabecera y entradas.
 * - Lectura parcial: imprimir cabecera y entradas recuperadas.
 * - Error fatal: no imprimir y detener el procesamiento normal.
 *
 * La cabecera solo se imprime cuando ctx->headers es true.
 * Un directorio abierto y vacío también puede tener cabecera.
 *
 * LIMPIEZA DE RECURSOS
 *
 * - Cerrar el directorio siempre que opendir() haya tenido éxito.
 * - Liberar la memoria de la lista en todos los caminos de salida.
 *
 * OTROS ERRORES YA CONTEMPLADOS
 *
 * - lstat() de una entrada: continuar la lectura; conservar
 *   la entrada con stat_ok = 0 y actualizar ctx->status.
 * - closedir() después de leer: informar del error y actualizar
 *   ctx->status, sin descartar las entradas ya almacenadas.
 *
 * OBJETIVO
 *
 * Recuperar e imprimir las entradas disponibles ante un error
 * de lectura no fatal, sin confundirlo con un fallo de apertura
 * ni con un error fatal de memoria.
 *
 * No implementar durante la Etapa 6.
 */
