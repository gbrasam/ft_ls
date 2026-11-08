/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   load_directory_entries.c                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 12:09:45 by gebravo           #+#    #+#             */
/*   Updated: 2026/11/08 15:25:15 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

static DIR	*open_directory(t_ctx *ctx, const char *name)
{
	DIR	*dir;

	dir = opendir(name);
	if (!dir)
	{
		fprintf(stderr, "%s: cannot open directory '%s': %s\n",
			ctx->prog, name, strerror(errno));
		ctx->status = 2;
		return (NULL);
	}
	return (dir);
}

/*
 * almacena una entrada del directorio en el array dinámico
 *
 * - amplía la capacidad cuando el array está lleno
 * - copia el nombre porque readdir() puede reutilizar su memoria
 * - obtiene metadatos solo si alguna opción los necesita (-l, -t y -R)
 *
 * devuelve:
 * 0: entrada almacenada
 * 1: error fatal de memoria
 */
static int	store_entry(t_ctx *ctx, const char *directory_name, const char *entry_name, t_directory_list *directory_list) 
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
	if (ctx->opts.l || ctx->opts.t || ctx->opts.big_r)
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
 * determinar el tipo. Cuando sea necesario, se consulta mediante
 * stat() o lstat()
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
			if (store_entry(ctx, directory_name, entry->d_name, directory_list) == 1)									/* guardar entradas válidas */
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
* workflow:
* 1. opendir()
* 2. readdir()  → store_entry() guardar entradas
* 3. closedir() → se ejecuta antes de ordenar e imprimir, lo que evitará acumular descriptores abiertos durante la futura recursividad
*/
int	load_directory_entries(t_ctx *ctx, const char *directory_name, t_directory_list *directory_list)
{
	DIR	*dir;
	int	flag_read;

	dir = open_directory(ctx, directory_name);
	if (!dir)
		return (1);
	flag_read = read_directory(ctx, directory_name, dir, directory_list);
	if (closedir(dir) == -1)																							/* closedir() se ejecuta después de cualquier lectura, incluso si read_directory() falla */
	{
		fprintf(stderr, "%s: closing directory '%s': %s\n",
			ctx->prog, directory_name, strerror(errno));
		/*
     		* TODO etapa 8: verificar gravedad del error
			* verificar frente a GNU ls qué código de salida
     		* corresponde a un fallo de closedir()
     		*
     		* Actualmente se establece status = 2
     		* Comprobar si la gravedad depende de que el directorio sea
     		* un operando de argv o un subdirectorio encontrado con -R
     		* No sobrescribir un status = 2 anterior con uno menor
     	*/
		ctx->status = 2;
	}
	return (flag_read);
}

/* si closedir() falla, estableces ctx->status = 2 pero devuelves el resultado de la lectura.
Es una decisión provisional válida mientras verificas el comportamiento de GNU ls */

/*
 * ¡! ETAPA 7 (-R): distinguir el origen del directorio
 *
 * PROBLEMA ACTUAL:
 * list_directory() solo recibe directorios procedentes de
 * los operandos de argv.
 *
 * Cuando implementemos -R, esta misma función también
 * recibirá subdirectorios descubiertos durante el recorrido.
 *
 * Es necesario distinguir ambos casos porque GNU ls puede
 * asignar distinta gravedad a los errores según el origen
 * del directorio.
 *
 * PROPUESTA: añadir un parámetro is_operand:
 *
 * - is_operand = 1:
 *   El directorio procede directamente de argv.
 *   Ejemplo: ./ft_ls directorio
 *
 * - is_operand = 0:
 *   El directorio se ha descubierto durante el recorrido -R.
 *   Ejemplo: ./ft_ls -R directorio
 *   y posteriormente se procesa directorio/subdirectorio.
 *
 * CRITERIO PREVISTO:
 * - Error de opendir() en un operando: status = 2.
 * - Error de opendir() en un subdirectorio: status = 1.
 * - No sobrescribir un status = 2 anterior con status = 1.
 *
 * PENDIENTE DE VERIFICACIÓN:
 * Comprobar frente a GNU ls los códigos correspondientes
 * a los errores de opendir() y readdir() en ambos casos.
 * No asumir que ambos tipos de error tienen necesariamente
 * la misma gravedad sin comprobarlo.
 *
 * IMPLEMENTACIÓN FUTURA:
 * - Añadir is_operand como parámetro de list_directory().
 * - Pasar is_operand a open_directory() para que pueda
 *   determinar la gravedad de los errores de apertura.
 * - Utilizar también esta información al tratar los
 *   errores de lectura de read_directory().
 * - Actualizar las llamadas desde handle_directories()
 *   y desde el futuro recorrido recursivo.
 *
 * IMPORTANTE:
 * is_operand pertenece a cada llamada de list_directory(),
 * no al contexto global t_ctx.
 *
 * Durante una misma ejecución pueden procesarse tanto
 * operandos originales como subdirectorios descubiertos.
 * El origen debe conservarse individualmente.
 *
 * Este cambio queda pendiente hasta implementar -R.
 * */

 /*
* ¡! ETAPA 8: distinguir errores de lectura y errores de memoria
*
* PROBLEMA ACTUAL:
* read_directory() devuelve 0 si termina correctamente y 1 si falla,
	 * pero no distingue la causa del fallo:
	 *
	 * - flag_read = 0: lectura completa. Ordenar e imprimir las entradas
	 * - flag_read = 1: puede significar dos situaciones diferentes:
	 *
	 *   1. Error de readdir() (recuperable):
	 *      No se ha podido completar la lectura del directorio, pero
	 *      podemos ordenar e imprimir las entradas ya almacenadas
	 *      Debemos informar del error y actualizar ctx->status
	 *
	 *   2. Error de memoria (fatal):
	 *      Ha fallado una reserva de memoria (realloc(), strdup() o
	 *      build_path()). No debemos continuar el procesamiento normal
	 *      ctx->fatal = FATAL_NOMEM y debemos liberar la memoria reservada
	 *
	 * SOLUCIÓN PENDIENTE:
	 * Modificar read_directory() para distinguir ambos errores:
	 *
	 * - return (0): lectura completada correctamente.
	 * - return (1): error de lectura recuperable (readdir())
	 * - return (2): error fatal de memoria
	 *
	 * Estos valores son códigos internos de read_directory(), no deben
	 * confundirse con ctx->status, que representa el código de salida
	 * global del programa
	 *
	 * En list_directory():
	 * - Resultado 0: ordenar e imprimir
	 * - Resultado 1: informar del error, ordenar e imprimir lo ya leído
	 * - Resultado 2: no imprimir; liberar memoria y detener el
	 *   procesamiento normal
	 *
	 * TBD ctx->fatal, también podríamos utilizarlo para distinguir los errores
	 * sin introducir tres valores de retorno [ELEGIDO]
	 *
	 * En todos los casos debemos cerrar el directorio y liberar
	 * la memoria de la lista.
	*/