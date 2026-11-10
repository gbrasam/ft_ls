/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   error.c                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/02 15:08:11 by gebravo           #+#    #+#             */
/*   Updated: 2026/11/10 12:35:15 by gebravo          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_ls.h"

/*
 * Informa de los errores fatales y establece el estado de salida 2.
 *
 * Los errores fatales impiden continuar la ejecución normal
 * o indican que la salida generada no se ha podido escribir.
 *
 * El tipo de error se conserva en ctx->fatal:
 * - FATAL_OPTION: opción no reconocida o inválida.
 * - FATAL_NOMEM: fallo de reserva de memoria.
 * - FATAL_CLOCK: fallo de clock_gettime().
 * - FATAL_WRITE: error de escritura en stdout.
 *
 * Para FATAL_CLOCK y FATAL_WRITE se utiliza ctx->error_errno
 * cuando se conoce la causa del fallo.
 *
 * En FATAL_WRITE, error_errno == 0 indica que se detectó un error
 * anterior mediante ferror(stdout), pero no se conservó su errno.
 * En ese caso se muestra un mensaje genérico.
 *
 * No es necesario ejecutar fflush(stdout) aquí porque main()
 * ya ha comprobado stdout mediante check_stdout_error().
 *
 * Los mensajes se escriben en stderr para mantenerlos separados
 * de la salida normal del listado.
 *
 * [GNU] Los mensajes de opciones inválidas siguen el formato
 * observado en GNU ls, incluido el mensaje de ayuda.
 */
void	report_fatal(t_ctx *ctx)
{
	ctx->status = 2;
	if (ctx->fatal == FATAL_OPTION)
	{
		if (ctx->invalid_kind == ARG_LONG)
			fprintf(stderr, "%s: unrecognized option '%s'\n", ctx->prog, ctx->invalid);
		else
			fprintf(stderr, "%s: invalid option -- '%c'\n", ctx->prog, ctx->invalid[0]);
		
		fprintf(stderr, "Try '%s --help' for more information.\n", ctx->prog);
	}
	else if (ctx->fatal == FATAL_NOMEM)
		fprintf(stderr, "%s: memory exhausted\n", ctx->prog);
	else if (ctx->fatal == FATAL_CLOCK)
		fprintf(stderr, "%s: clock_gettime: %s\n", ctx->prog, strerror(ctx->error_errno));
	else if (ctx->fatal == FATAL_WRITE)
	{
		if (ctx->error_errno != 0)																	/* falló el fflush() final y errno tiene la causa */
			fprintf(stderr, "%s: write error: %s\n", ctx->prog, strerror(ctx->error_errno));
		else																						/* el fflush() final funcionó, pero ferror(stdout) indica que falló una escritura anterior. Por tanto, no hay causa fiable así que se imprime solo write error */
			fprintf(stderr, "%s: write error\n", ctx->prog);
	}
}

/*
 * Informa de errores durante el listado sin detener necesariamente
 * la ejecución.
 *
 * fflush(stdout) intenta escribir los datos pendientes antes
 * de imprimir el error en stderr.
 * Esto ayuda a conservar el orden de los mensajes cuando stdout
 * y stderr comparten un mismo destino.
 *
 * Si fflush() falla, queda activado el indicador de error de stdout.
 * main() podrá detectarlo posteriormente mediante ferror(stdout).
 * Sin embargo, aquí no se conserva el errno de ese fallo, por lo
 * que podría desconocerse su causa exacta al finalizar.
 *
 * error_errno contiene el errno original de la operación fallida
 * (lstat, opendir, readdir, etc.).
 * Se recibe como argumento porque fflush() y otras operaciones
 * podrían modificar errno antes de construir el mensaje.
 *
 * 'serious' indica la gravedad del error, no su origen:
 *
 * - true:  error grave, establece status = 2.
 * - false: error no grave, establece status = 1 si no era 2.
 *
 * Para errores de directorio (opendir, readdir, closedir),
 * puede utilizarse is_operand como gravedad:
 * - true:  directorio recibido como operando.
 * - false: subdirectorio descubierto durante -R.
 *
 * En otros casos, la gravedad se determina directamente:
 * - Error de acceso a un operando original: grave.
 * - Error de lstat() o readlink() de una entrada: no grave.
 *
 * Un error no grave nunca rebaja un estado 2 anterior.
 */
void	report_error(t_ctx *ctx, bool serious, const char *error_message, const char *path, int error_errno)
{
	fflush(stdout);
	fprintf(stderr, "%s: %s '%s': %s\n", ctx->prog, error_message, path, strerror(error_errno));
	if (serious)
		ctx->status = 2;
	else if (ctx->status != 2)
		ctx->status = 1;
}

/*
 * Comprueba los errores de escritura de stdout antes de finalizar.
 *
 * ¿POR QUÉ ES NECESARIO?
 *
 * printf() no siempre escribe inmediatamente en el destino
 * (terminal, archivo, pipe, etc.).
 * Puede almacenar temporalmente los datos en un búfer de memoria.
 *
 * Por tanto, printf() puede devolver éxito aunque los datos
 * todavía no se hayan escrito realmente en su destino.
 *
 * Ejemplo: ./ft_ls > /dev/full
 *
 * /dev/full es un dispositivo de Linux que rechaza las escrituras
 * con ENOSPC (No space left on device).
 * Los printf() pueden parecer correctos si solo llenan el búfer.
 * El error aparece cuando se intenta escribir en /dev/full.
 *
 * ¿QUÉ HACE fflush(stdout)?
 *
 * - Intenta escribir los datos pendientes del búfer de stdout.
 * - Devuelve 0 si tiene éxito y EOF si falla.
 * - No borra los datos que ya se han escrito correctamente.
 *
 * Si falla, guardamos inmediatamente errno para conservar
 * la causa antes de ejecutar otras operaciones.
 *
 * ¿QUÉ HACE ferror(stdout)?
 *
 * La biblioteca de C mantiene un indicador de error asociado
 * a cada flujo (stdout, stderr, archivos abiertos, etc.).
 *
 * Cuando falla una operación de escritura, el indicador
 * de error del flujo queda activado.
 *
 * ferror(stdout) consulta ese indicador:
 * - 0: no hay ningún error registrado en stdout.
 * - Distinto de 0: existe un error registrado en stdout.
 *
 * ferror() no intenta escribir, no modifica el indicador
 * y no devuelve el errno que provocó el error.
 *
 * ¿POR QUÉ UTILIZAMOS AMBAS FUNCIONES?
 *
 * - fflush(stdout) intenta escribir los datos todavía pendientes.
 * - ferror(stdout) detecta también errores de escritura anteriores,
 *   aunque se hayan producido antes del fflush() final.
 *
 * ¿PARA QUÉ SIRVE flush_error?
 *
 * Se inicializa a 0 y conserva el errno de fflush() si falla.
 * Si fflush() tiene éxito, permanece en 0.
 *
 * En Linux/POSIX, fflush() establece errno cuando falla.
 * Por eso flush_error != 0 permite detectar ese fallo
 * sin guardar por separado el resultado de fflush().
 *
 * Si únicamente ferror() detecta un error anterior,
 * desconocemos su errno original y conservamos 0.
 * report_fatal() mostrará entonces un mensaje genérico.
 *
 * ¿CÓMO SE REGISTRA EL ERROR?
 *
 * Solo se establece FATAL_WRITE si no existe otro error fatal,
 * para no sustituir una causa registrada previamente.
 *
 * ctx->error_errno conserva la causa conocida del fallo,
 * o 0 cuando se desconoce.
 *
 * report_fatal() establecerá status = 2 e informará por stderr.
 *
 * Esta comprobación se realiza antes de terminar el programa
 * porque un vaciado automático al finalizar podría detectar
 * un error cuando ya no podemos actualizar ctx->status.
 */
void	check_stdout_error(t_ctx *ctx)
{
	int	flush_error;

	flush_error = 0;
	if (fflush(stdout) == EOF)
		flush_error = errno;
	if ((flush_error != 0 || ferror(stdout)) && ctx->fatal == FATAL_NONE)
	{
		ctx->fatal = FATAL_WRITE;
		ctx->error_errno = flush_error;
	}
}
