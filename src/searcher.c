
/*
Proceso que se encarga de buscar en el index generado a partir del CSV de Spotify.
* Busca por Álbum y Artista, opcionalmente por Canción.
* Utiliza una tabla hash para optimizar la búsqueda.
* El resultado se envía a través de tuberías FIFO para ser mostrado en la interfaz.

*/
#define _GNU_SOURCE // Necesario para strcasestr y asprintf
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define HASH_TABLE_SIZE 500000
#define MAX_LINE_LENGTH 8192 // Longitud máxima de una línea del CSV
#define MAX_KEY_LENGTH 512 // Longitud máxima de la clave compuesta
#define MAX_RESULTS_BUFFER 65536 // Hasta 200 resultados sin problema.
typedef struct Nodo
{
    long csv_puntero;
    long siguiente_nodo_puntero;
} Nodo;
long *hash_table;

// --- Declaraciones de Funciones ---
char *buscar_campo(const char *line, int campoABuscar);
unsigned long hash_function(const char *str);
void cerrar_todo();
char *extraer_artista(const char *json_string);
void formato_resultado(char *dest, size_t dest_size, const char *csv_line);

// --- Función Principal ---
int main(int argc, char *argv[])
{
    // La inicialización es la misma...
    FILE *index_file = fopen("spotify.index", "rb");
    if (!index_file)
    {
        perror("Error: No se pudo abrir 'spotify.index'.");
        return 1;
    }
    hash_table = (long *)malloc(sizeof(long) * HASH_TABLE_SIZE);
    fread(hash_table, sizeof(long), HASH_TABLE_SIZE, index_file);
    fclose(index_file);
    FILE *csv_file = fopen("spotify_data.csv", "r");
    if (!csv_file)
    {
        free(hash_table);
        perror("Error al abrir spotify_data.csv");
        return 1;
    }
    const char *ui_pipe = "/tmp/ui_to_search_pipe";
    const char *search_pipe = "/tmp/search_to_ui_pipe";
    mkfifo(ui_pipe, 0666);
    mkfifo(search_pipe, 0666);
    atexit(cerrar_todo); // Asegura que se liberen los recursos al finalizar el programa.
    char *line_buffer = malloc(MAX_LINE_LENGTH);

    printf("Servidor de búsqueda estable (con salida formateada) iniciado.\n");

    // Bucle de servidor robusto...
    while (1)
    {
        int fd_read = open(ui_pipe, O_RDONLY);
        if (fd_read < 0)
            continue;
        char query_buffer[MAX_KEY_LENGTH * 2] = {0};
        ssize_t bytes_read = read(fd_read, query_buffer, sizeof(query_buffer) - 1);
        close(fd_read);

        if (bytes_read > 0)
        {
            //Asegura finalizar la cadena de consulta
            query_buffer[bytes_read] = '\0';
            //Crea copia de la query
            char query_copy[sizeof(query_buffer)];
            strcpy(query_copy, query_buffer);
            // Divide la consulta en partes strtok 
            // tiene memoria.
            char *album_q = strtok(query_copy, "|");
            char *artista_q = strtok(NULL, "|");
            char *cancion_q = strtok(NULL, "|");
            if (!album_q || !artista_q) 
            // Verifica que los campos obligatorios no estén vacíos
                continue;
            // Construye la clave compuesta para la búsqueda hash.
            char composite_key[MAX_KEY_LENGTH];
            snprintf(composite_key, sizeof(composite_key), "%s|%s", album_q, artista_q);
            char final_result[MAX_RESULTS_BUFFER] = {0};
            int encontrados_cuenta = 0;
            unsigned long index = hash_function(composite_key);
            long campo_nodo_index = hash_table[index];
            FILE *idx_f = fopen("spotify.index", "rb");

            if (idx_f)
            {
                while (campo_nodo_index != -1)
                {
                    fseek(idx_f, campo_nodo_index, SEEK_SET);
                    Nodo current_node;
                    fread(&current_node, sizeof(Nodo), 1, idx_f);
                    fseek(csv_file, current_node.csv_puntero, SEEK_SET);
                    fgets(line_buffer, MAX_LINE_LENGTH, csv_file);

                    char *album_from_csv = buscar_campo(line_buffer, 1);
                    char *artista_json_from_csv = buscar_campo(line_buffer, 4);
                    char *artist_from_csv = artista_json_from_csv ? extraer_artista(artista_json_from_csv) : NULL;

                    if (album_from_csv && artist_from_csv &&
                        strcmp(album_from_csv, album_q) == 0 &&
                        strcmp(artist_from_csv, artista_q) == 0)
                    {

                        if (cancion_q == NULL || strlen(cancion_q) == 0)
                        {
                            char formatted_line[MAX_LINE_LENGTH];
                            formato_resultado(formatted_line, sizeof(formatted_line), line_buffer);
                            strncat(final_result, formatted_line, MAX_RESULTS_BUFFER - strlen(final_result) - 1);
                            encontrados_cuenta++;
                        }
                        else
                        {
                            char *cancion_from_csv = buscar_campo(line_buffer, 8);
                            if (cancion_from_csv && strcasestr(cancion_from_csv, cancion_q) != NULL)
                            {
                                char formatted_line[MAX_LINE_LENGTH];
                                formato_resultado(formatted_line, sizeof(formatted_line), line_buffer);
                                strncat(final_result, formatted_line, MAX_RESULTS_BUFFER - strlen(final_result) - 1);
                                encontrados_cuenta++;
                            }
                            if (cancion_from_csv)
                                free(cancion_from_csv);
                        }
                    }
                    if (album_from_csv)
                        free(album_from_csv);
                    if (artista_json_from_csv)
                        free(artista_json_from_csv);
                    if (artist_from_csv)
                        free(artist_from_csv);
                    campo_nodo_index = current_node.siguiente_nodo_puntero;
                }
                fclose(idx_f);
            }
            if (encontrados_cuenta == 0)
                strcpy(final_result, "NA");
            int fd_write = open(search_pipe, O_WRONLY);
            if (fd_write >= 0)
            {
                write(fd_write, final_result, strlen(final_result) + 1);
                close(fd_write);
            }
        }
    }
    return 0;
}

void formato_resultado(char *dest, size_t dest_size, const char *csv_line)
{
    char *album = buscar_campo(csv_line, 1);
    char *artista_json = buscar_campo(csv_line, 4);
    char *artist = artista_json ? extraer_artista(artista_json) : NULL;
    char *cancion = buscar_campo(csv_line, 8);
    char *duracion_str = buscar_campo(csv_line, 6);
    char *popularidad_str = buscar_campo(csv_line, 10);

    char duration_formatted[32] = "N/A";
    if (duracion_str)
    {
        long duration_ms = atol(duracion_str);
        int minutes = duration_ms / 60000;
        int seconds = (duration_ms % 60000) / 1000;
        snprintf(duration_formatted, sizeof(duration_formatted), "%d min %d seg", minutes, seconds);
    }

    snprintf(dest, dest_size,
             "Álbum: %s\n"
             "Artista: %s\n"
             "Canción: %s\n"
             "Duración: %s\n"
             "Popularidad: %s\n"
             "--------------------------------------------------\n",
             album ? album : "N/A",
             artist ? artist : "N/A",
             cancion ? cancion : "N/A",
             duration_formatted,
             popularidad_str ? popularidad_str : "N/A");

    // Liberar toda la memoria alocada por buscar_campo y extraer_artista
    if (album)
        free(album);
    if (artista_json)
        free(artista_json);
    if (artist)
        free(artist);
    if (cancion)
        free(cancion);
    if (duracion_str)
        free(duracion_str);
    if (popularidad_str)
        free(popularidad_str);
}
/*
* Función de limpieza para liberar recursos y eliminar tuberías
* al finalizar el programa o usar ejemplo CTRL+C
*
*/

void cerrar_todo()
{
    if (hash_table)
        free(hash_table);
    unlink("/tmp/ui_to_search_pipe");
    unlink("/tmp/search_to_ui_pipe");
    printf("\nBúsqueda terminada. Tuberías eliminadas.\n");
}

unsigned long hash_function(const char *str)
{
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
    {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % HASH_TABLE_SIZE;
}
char *buscar_campo(const char *line, int campoABuscar)
{
    char *buffer = malloc(MAX_LINE_LENGTH);
    if (!buffer)
        return NULL;
    int campoActual = 1, in_quotes = 0, buffer_pos = 0;
    const char *line_ptr = line;
    while (*line_ptr)
    {
        if (*line_ptr == '"' && *(line_ptr + 1) != '"')
        {
            in_quotes = !in_quotes;
        }
        else if (*line_ptr == ',' && !in_quotes)
        {
            if (campoActual == campoABuscar)
            {
                buffer[buffer_pos] = '\0';
                return buffer;
            }
            campoActual++;
            buffer_pos = 0;
        }
        else
        {
            if (campoActual == campoABuscar)
            {
                if (buffer_pos < MAX_LINE_LENGTH - 1)
                {
                    buffer[buffer_pos++] = *line_ptr;
                }
            }
        }
        line_ptr++;
    }
    if (campoActual == campoABuscar)
    {
        if (buffer_pos > 0 && buffer[buffer_pos - 1] == '\n')
            buffer_pos--;
        buffer[buffer_pos] = '\0';
        return buffer;
    }
    free(buffer);
    return NULL;
}
char *extraer_artista(const char *json_string)
{
    // Busca el patrón con comillas simples: 'artist_nombre': '...'
    const char *key = "'artist_name': '";
    const char *comienzo = strstr(json_string, key);
    if (!comienzo)
    {
        //devuelve el original
        return strdup(json_string);
    }
    comienzo += strlen(key);

    // Busca la comilla simple de cierre
    const char *final = strchr(comienzo, '\'');
    if (!final)
    {
        return strdup(json_string);
    }

    size_t length = final - comienzo;
    char *nombre = malloc(length + 1);
    if (nombre)
    {
        strncpy(nombre, comienzo, length);
        nombre[length] = '\0';
    }
    return nombre;
}