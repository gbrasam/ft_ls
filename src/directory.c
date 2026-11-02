/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   directory.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gebravo <gebravo@student.42.fr>            +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/16 12:09:45 by gebravo           #+#    #+#             */
/*   Updated: 2026/11/02 17:31:10 by gebravo          ###   ########.fr       */
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
static int	read_directory(t_ctx *ctx, const char *name, DIR *dir, t_directory_list *directory_list)
{
	struct dirent	*entry;

	errno = 0;
	entry = readdir(dir);
	while (entry)																									/* lee las entradas del directorio una a una hasta que readdir() devuelve NULL (no quedan entradas o hay un error) */
	{
		if (ctx->opts.a || entry->d_name[0] != '.')																	/* aplicar filtro -a */
		{
			if (store_entry(ctx, name, entry->d_name, directory_list) == 1)											/* guardar entradas válidas */
				return (1);
		}
		errno = 0;
		entry = readdir(dir);
	}
	if (errno != 0)																									/* error de lectura */
	{
		fprintf(stderr, "%s: reading directory '%s': %s\n",
			ctx->prog, name, strerror(errno));
		ctx->status = 2;
		return (1);
	}
	return (0);
}

/* ------------------------------------------ handle_directory_entries ------------------------------------------ */

/*
 * Prepara los anchos máximos del formato largo (-l) y acumula
 * los bloques de almacenamiento de las entradas de un directorio.
 *
 * Ambas operaciones se realizan en un único recorrido.
 *
 * FILTRADO DE ENTRADAS:
 *
 * directory_list ya contiene únicamente las entradas admitidas
 * por el filtro -a, aplicado en read_directory() antes de guardarlas.
 *
 * Sin -a, se excluyen las entradas cuyo nombre empieza por '.'.
 * Con -a, se incluyen también las ocultas, '.' y '..'.
 *
 * Por tanto, no es necesario volver a comprobar su visibilidad.
 *
 * METADATOS:
 *
 * Solo se consideran entradas con metadatos válidos
 * (stat_ok == 1), ya que ambas operaciones necesitan
 * acceder a los campos de struct stat.
 *
 * Las entradas sin metadatos válidos no contribuyen
 * a los anchos ni al total de bloques.
 *
 * CÁLCULO DE ANCHOS:
 *
 * update_print_widths() calcula el espacio necesario para
 * representar el número de enlaces, el propietario, el grupo
 * y el tamaño, y conserva el máximo acumulado de cada columna.
 *
 * Los anchos representan caracteres de la salida impresa,
 * no el tamaño de los datos en memoria.
 *
 * CÁLCULO DEL TOTAL:
 *
 * st_blocks es un campo de struct stat, de tipo blkcnt_t.
 * Indica el espacio de almacenamiento asignado al archivo.
 *
 * En Linux, su valor se expresa en bloques de 512 bytes,
 * independientemente del tamaño lógico del archivo (st_size).
 *
 * Ejemplo:
 *   Archivo A: st_blocks = 8
 *   Archivo B: st_blocks = 7
 *   Archivo C: st_blocks = 1
 *
 *   Suma acumulada: 16 bloques de 512 bytes.
 *
 * La función acumula los valores originales de st_blocks.
 * No convierte ni redondea cada entrada individualmente.
 *
 * La conversión a las unidades mostradas por ls debe
 * realizarse una sola vez, después de completar la suma.
 *
 * IMPORTANTE:
 *   El total corresponde a las entradas del directorio,
 *   no a los archivos pasados individualmente como operandos.
 *   Por eso no se calcula en prepare_files_widths().
 *
 * INICIALIZACIÓN:
 *
 * handle_directory_entries() inicializa previamente:
 *
 *   - Los anchos mediante init_print_widths().
 *   - El acumulador total a cero.
 *
 * Esta función recibe ambos resultados por puntero
 * y actualiza sus valores durante el recorrido.
 *
 * Cada directorio mantiene sus propios anchos y total,
 * sin acumular resultados de directorios anteriores.
 *
 * RESPONSABILIDAD:
 *
 * Prepara en un único recorrido la información necesaria
 * para alinear las columnas y mostrar el total de bloques.
 *
 * No imprime entradas, no convierte las unidades del total,
 * no aplica filtros de visibilidad ni ordena el listado.
 */
static void	prepare_entries_info(const t_directory_list *directory_list, t_print_widths *widths, blkcnt_t *total)
{
	size_t	i;

	i = 0;
	while (i < directory_list->count)
	{
		if (directory_list->items[i].stat_ok)
		{
			/* widths */
			update_print_widths(&directory_list->items[i].st, widths);
			/* total */
			*total += directory_list->items[i].st.st_blocks;
		}
		i++;
	}
}

static void	print_directory_entries(t_ctx *ctx, const t_directory_list *directory_list, const t_print_widths *widths)
{
	size_t	i;

	i = 0;
	while (i < directory_list->count)
	{
		if (ctx->opts.l)
		{
			/*
			* TODO paso 8: imprimir las entradas con stat_ok == 0
			*/
			if (directory_list->items[i].stat_ok)
			{
				/*
				* TODO paso 7: construir la ruta completa de los enlaces
				* simbólicos para readlink() y liberar la memoria reservada
				*/
				print_long_format(ctx, directory_list->items[i].name, NULL, &directory_list->items[i].st, widths);
			}
		}
		else
			printf("%s\n", directory_list->items[i].name);
		i++;
	}
}

/*
 * Coordina la preparación e impresión de las entradas de un directorio.
 *
 * CON LA OPCIÓN -l:
 *
 * 1. Inicializa los anchos de las columnas mediante
 *    init_print_widths() y el acumulador total a cero.
 *
 * 2. Prepara los anchos máximos y acumula los bloques
 *    de almacenamiento de las entradas con stat_ok == 1.
 *
 * 3. TODO: convertir e imprimir el total de bloques.
 *
 * 4. Imprime las entradas en formato largo.
 *
 * SIN LA OPCIÓN -l:
 *
 * Imprime únicamente los nombres de las entradas, sin
 * calcular los anchos ni el total de bloques.
 *
 * INDEPENDENCIA ENTRE DIRECTORIOS:
 *
 * Cada directorio tiene sus propios anchos y total.
 * Los valores se inicializan de nuevo para evitar que
 * los resultados de un directorio afecten a otro.
 *
 * TOTAL DE BLOQUES:
 *
 * total es de tipo blkcnt_t, el mismo tipo que st_blocks
 * de struct stat.
 *
 * st_blocks representa el espacio de almacenamiento
 * asignado al archivo, no su tamaño lógico (st_size).
 *
 * En Linux, st_blocks se expresa en unidades de 512 bytes.
 * La conversión a las unidades mostradas por ls se realiza
 * después de acumular los bloques de todas las entradas.
 *
 * RESPONSABILIDAD:
 *
 * Esta función organiza las operaciones necesarias para
 * mostrar el contenido de un directorio.
 *
 * No recorre directamente las entradas ni calcula los
 * anchos o los bloques de cada una.
 */
static void	handle_directory_entries(t_ctx *ctx, t_directory_list *directory_list)
{
	t_print_widths	widths;
	blkcnt_t		total;

	init_print_widths(&widths);
	total = 0;
	if (ctx->opts.l)
	{
		prepare_entries_info(directory_list, &widths, &total);
		/* TODO: convertir e imprimir "total N" */
	}
	print_directory_entries(ctx, directory_list, &widths);
}

/* -------------------------------------------------------------------------------------------------------------- */

/**
 * LIST_DIRECTORY — Abrir, leer y procesar el contenido de un directorio
 *
 * 1. ABRIR EL DIRECTORIO: opendir()
 *
 *    #include <dirent.h>
 *    DIR *opendir(const char *name);
 *
 *    - name: ruta del directorio que queremos abrir (path).
 *    - DIR *: puntero que permite gestionar su lectura.
 *
 *    En nuestro programa:
 *
 *        DIR *dir;
 *        dir = open_directory(ctx, path);
 *
 *    open_directory() utiliza opendir() y gestiona los errores.
 *
 *    - Éxito: devuelve un puntero DIR *.
 *    - Error: devuelve NULL y establece errno.
 *
 *    IMPORTANTE:
 *    opendir() abre el directorio, pero no devuelve sus entradas.
 *    Para obtenerlas necesitamos readdir().
 *
 *
 * 2. LEER LAS ENTRADAS: readdir()
 *
 *    #include <dirent.h>
 *    struct dirent *readdir(DIR *dirp);
 *
 *    - Recibe el puntero DIR * obtenido mediante opendir().
 *    - Devuelve un puntero a la siguiente entrada del directorio.
 *    - Devuelve NULL al finalizar o cuando se produce un error.
 *
 *    Ejemplo:
 *
 *        struct dirent *entry;
 *        entry = readdir(dir);
 *
 *    El nombre de la entrada se obtiene mediante:
 *
 *        entry->d_name
 *
 *    IMPORTANTE:
 *    Comprobar que entry != NULL antes de acceder a sus campos.
 *
 *    Para distinguir el final de la lectura de un error:
 *
 *    - errno = 0 antes de llamar a readdir().
 *    - NULL y errno == 0: fin normal.
 *    - NULL y errno != 0: error de lectura.
 *
 *    ATENCIÓN:
 *    readdir() puede reutilizar la memoria de la entrada devuelta.
 *    Por eso no conservamos directamente entry->d_name.
 *
 *    store_entry() utiliza strdup() para copiar el nombre
 *    y almacenarlo en nuestra propia t_directory_list.
 *
 *    read_directory() se encarga de recorrer las entradas,
 *    filtrarlas y almacenarlas mediante store_entry().
 *
 *
 * 3. ALMACENAR LAS ENTRADAS Y OBTENER SUS METADATOS
 *
 *    IMPLEMENTADO:
 *
 *    - Filtrar nombres ocultos según la opción -a.
 *    - Almacenar las entradas en t_directory_list.
 *    - Copiar sus nombres mediante strdup().
 *    - Obtener metadatos mediante store_entry() -> load_stat()
 *      cuando están activadas -l, -t o -R.
 *    - Guardar los metadatos en t_directory_entry.st.
 *    - Utilizar stat_ok para indicar si lstat() tuvo éxito.
 *
 *    load_stat() utiliza lstat() sobre la ruta completa:
 *
 *        ruta_directorio + "/" + nombre_entrada
 *
 *    Ejemplo:
 *
 *        path:       "src"
 *        name:       "main.c"
 *        ruta real:  "src/main.c"
 *
 *    IMPORTANTE:
 *    lstat() obtiene información del propio enlace simbólico,
 *    sin seguirlo hasta su destino.
 *
 *    Esto permite distinguir archivos, directorios y enlaces
 *    simbólicos mediante st_mode.
 *
 *    Si lstat() falla de forma recuperable:
 *
 *    - stat_ok permanece en 0.
 *    - Se informa del error y se actualiza ctx->status.
 *    - La entrada puede permanecer almacenada en la lista.
 *
 *    Un error fatal de memoria se registra en ctx->fatal.
 *
 *
 * 4. CERRAR EL DIRECTORIO: closedir()
 *
 *    #include <dirent.h>
 *    int closedir(DIR *dirp);
 *
 *    - Cierra el directorio abierto mediante opendir().
 *    - Devuelve 0 si tiene éxito.
 *    - Devuelve -1 si falla y establece errno.
 *
 *    En nuestro programa cerramos el directorio después
 *    de leer las entradas y antes de ordenarlas e imprimirlas.
 *
 *    Esto es posible porque ya hemos conservado sus nombres
 *    y los metadatos necesarios en nuestra propia lista.
 *
 *    IMPORTANTE:
 *    closedir() también debe ejecutarse si read_directory()
 *    falla, siempre que opendir() haya tenido éxito.
 *
 *    Para la futura opción -R, cerrar antes de descender
 *    evita mantener abiertos simultáneamente los directorios
 *    de los distintos niveles.
 *
 *
 * 5. ORDENAR LAS ENTRADAS — ETAPA 4 TERMINADA
 *
 *    sort_directory_entries() ordena las entradas almacenadas
 *    antes de imprimirlas.
 *
 *    Comportamiento:
 *
 *    - Sin -t: orden alfabético por nombre.
 *    - Con -t: orden por fecha de modificación, más reciente
 *      primero.
 *    - Con -r: invertir el orden resultante.
 *
 *    La ordenación se realiza mediante qsort().
 *
 *    Para -t se utilizan los metadatos obtenidos mediante
 *    lstat(), especialmente:
 *
 *        st.st_mtim.tv_sec
 *        st.st_mtim.tv_nsec
 *
 *    Se comparan segundos y nanosegundos para conservar
 *    la precisión temporal.
 *
 *    En caso de igualdad de fecha, se desempata por nombre.
 *
 *    Las entradas con stat_ok == 0 se consideran al ordenar:
 *    las válidas preceden a las inválidas en el orden base.
 *
 *    IMPORTANTE:
 *    La inversión -r se aplica después de la ordenación,
 *    por lo que afecta también al orden de los desempates.
 *
 *    La ordenación no modifica los nombres ni los metadatos:
 *    únicamente reorganiza las estructuras dentro del array.
 *
 *
 * 6. PREPARAR E IMPRIMIR LAS ENTRADAS — ETAPA 5 (-l)
 *
 *    handle_directory_entries() coordina la presentación
 *    del contenido del directorio.
 *
 *    Flujo:
 *
 *    1. Inicializar los anchos de columna:
 *
 *        init_print_widths(&widths);
 *
 *    2. Si está activado -l:
 *
 *        prepare_entries_widths(directory_list, &widths);
 *
 *    3. Imprimir las entradas:
 *
 *        print_directory_entries(ctx, directory_list, &widths);
 *
 *    PREPARACIÓN DE ANCHOS:
 *
 *    Cada directorio tiene sus propios anchos máximos.
 *
 *    prepare_entries_widths() recorre las entradas visibles
 *    con metadatos válidos (stat_ok == 1).
 *
 *    Para cada entrada utiliza update_print_widths(),
 *    que actualiza los máximos de:
 *
 *        widths.nlink  -> número de enlaces físicos.
 *        widths.owner  -> nombre del propietario.
 *        widths.group  -> nombre del grupo.
 *        widths.size   -> tamaño mostrado.
 *
 *    Los anchos representan caracteres de salida, no bytes
 *    ocupados por las variables en memoria.
 *
 *    FORMATO LARGO:
 *
 *    print_long_format() presenta los campos:
 *
 *    - Tipo de archivo y permisos (st_mode).
 *    - Número de enlaces físicos (st_nlink).
 *    - Propietario (st_uid -> getpwuid()).
 *    - Grupo (st_gid -> getgrgid()).
 *    - Tamaño (st_size).
 *    - Fecha de modificación (st_mtim).
 *    - Nombre de la entrada.
 *
 *    Los campos variables se alinean utilizando los anchos
 *    máximos calculados previamente.
 *
 *    Para los identificadores de propietario y grupo debe
 *    contemplarse una representación numérica si no se
 *    encuentra el nombre correspondiente.
 *
 *    ENLACES SIMBÓLICOS:
 *
 *    En formato largo se muestra:
 *
 *        nombre -> destino
 *
 *    El destino se obtiene mediante readlink().
 *
 *    Debemos distinguir:
 *
 *    - Nombre mostrado: "enlace".
 *    - Ruta completa: "src/enlace".
 *
 *    readlink() necesita la ruta completa del enlace.
 *    El nombre mostrado no debe incluir necesariamente
 *    la ruta del directorio.
 *
 *    FECHAS:
 *
 *    La presentación de st_mtim depende de su antigüedad
 *    respecto a una referencia temporal común.
 *
 *    Se utilizará ctx->now, inicializada una sola vez,
 *    para aplicar coherentemente la regla de los seis meses.
 *
 *    TOTAL DE BLOQUES:
 *
 *    Con -l, antes de imprimir las entradas del directorio,
 *    se muestra:
 *
 *        total N
 *
 *    El total se calcula a partir de st_blocks de las
 *    entradas incluidas en el listado y con stat_ok válido.
 *
 *    st_blocks expresa normalmente bloques de 512 bytes.
 *    La conversión a las unidades mostradas debe respetar
 *    el comportamiento de referencia de ls.
 *
 *    El total no representa el número de archivos ni
 *    la suma de sus tamaños st_size.
 *
 *    IMPORTANTE:
 *    El total corresponde al contenido de cada directorio.
 *    No se imprime un total para los operandos OPS_FILE
 *    mostrados individualmente.
 *
 *    ENTRADAS SIN METADATOS VÁLIDOS:
 *
 *    Si -l está activado y stat_ok == 0, la implementación
 *    prevista omite la impresión detallada de esa entrada.
 *
 *    Estas entradas tampoco contribuyen a los anchos
 *    máximos ni al total de bloques.
 *
 *    Sin -l, print_directory_entries() imprime los nombres
 *    de las entradas seleccionadas.
 *
 *
 * 7. RECORRIDO RECURSIVO — ETAPA 7 PENDIENTE
 *
 *    Cuando se implemente -R:
 *
 *    1. Procesar y mostrar el directorio actual.
 *    2. Identificar sus subdirectorios reales.
 *    3. Descender recursivamente a cada subdirectorio.
 *
 *    Nunca debemos descender a "." ni a "..".
 *
 *    Los metadatos obtenidos mediante lstat() permiten
 *    evitar confundir enlaces simbólicos con directorios.
 *
 *    PENDIENTE:
 *    Distinguir directorios recibidos como operandos argv
 *    de los descubiertos durante el recorrido recursivo.
 *
 *    Esta distinción puede afectar al tratamiento
 *    y gravedad de determinados errores.
 *
 *
 * 8. LIBERAR LA MEMORIA: clean_directory_list()
 *
 *    Después de procesar las entradas, liberamos:
 *
 *    - Cada nombre reservado mediante strdup().
 *    - El array dinámico de t_directory_entry.
 *
 *    No debemos confundir dos recursos diferentes:
 *
 *    - DIR *dir:
 *      Gestionado mediante opendir() y closedir().
 *
 *    - t_directory_list:
 *      Memoria dinámica gestionada mediante store_entry()
 *      y clean_directory_list().
 *
 *    Cerrar DIR * no libera nuestra lista.
 *    Liberar nuestra lista no cierra DIR *.
 *
 *    Si open_directory() falla, retornamos directamente
 *    sin inicializar la lista.
 *
 *    Si read_directory() falla después de inicializarla,
 *    debemos cerrar DIR * y liberar la memoria almacenada.
 *
 *
 * 9. TRATAMIENTO DE ERRORES — ETAPA 8 PENDIENTE
 *
 *    ctx->status:
 *    Representa el código de salida global del programa.
 *
 *    - 0: sin errores.
 *    - 1: errores menores durante el procesamiento.
 *    - 2: errores de mayor gravedad.
 *
 *    Un error recuperable puede permitir continuar
 *    procesando otros archivos o directorios.
 *
 *    ctx->fatal:
 *    Indica un error que impide continuar normalmente.
 *
 *    Ejemplo:
 *
 *        FATAL_NOMEM
 *
 *    IMPORTANTE:
 *    ctx->status y ctx->fatal tienen responsabilidades
 *    diferentes y no deben confundirse.
 *
 *    PENDIENTE:
 *
 *    - Distinguir errores recuperables de readdir()
 *      de errores fatales de memoria.
 *    - Utilizar ctx->fatal para identificar los errores
 *      de memoria sin añadir códigos de retorno innecesarios.
 *    - Verificar los códigos de salida frente a GNU ls.
 *    - Revisar los errores de closedir().
 *    - Revisar los errores de escritura.
 *    - Evitar sobrescribir un status de mayor gravedad
 *      con otro de menor gravedad.
 *
 *    IMPORTANTE:
 *    Actualmente la ordenación y la impresión se ejecutan
 *    cuando read_directory() devuelve 0.
 *
 *    La gestión diferenciada de errores recuperables y
 *    fatales queda pendiente de la etapa 8.
 */
void	list_directory(t_ctx *ctx, const char *path)
{
	DIR					*dir;
	t_directory_list	directory_list;
	int					flag_read;

	/*
	 * workflow:
	 * 1. opendir()
	 * 2. readdir()  → guardar entradas
	 * 3. closedir() → se ejecuta antes de ordenar e imprimir, lo que evitará acumular descriptores abiertos durante la futura recursividad
	 * 4. ordenar entradas
	 * 5. imprimir entradas
	 * 6. recorrer subdirectorios con -R TODO
	 * 7. liberar memoria
	*/
	
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
  	 */
	dir = open_directory(ctx, path);
	if (!dir)
		return ;
	init_directory_list(&directory_list);
	flag_read = read_directory(ctx, path, dir, &directory_list);
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
	if (closedir(dir) == -1)
	{
		fprintf(stderr, "%s: closing directory '%s': %s\n",
			ctx->prog, path, strerror(errno));
		/*
     		* ¡! ETAPA 8: verificar frente a GNU ls qué código de salida
     		* corresponde a un fallo de closedir()
     		*
     		* Actualmente se establece status = 2
     		* Comprobar si la gravedad depende de que el directorio sea
     		* un operando de argv o un subdirectorio encontrado con -R
     		* No sobrescribir un status = 2 anterior con uno menor
     	*/
		ctx->status = 2;
	}
	if (flag_read == 0)
	{
		sort_directory_entries(ctx, &directory_list);
		handle_directory_entries(ctx, &directory_list);
        /* TODO: Recursividad -R (pendiente) */
	}
	clean_directory_list(&directory_list);
}
