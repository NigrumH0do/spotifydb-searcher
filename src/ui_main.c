/*
Proceso generado para la interfaz de usuario
que se conecta al proceso de búsqueda de Spotify.
* Utiliza tuberías FIFO para la comunicación.
* Permite buscar por Álbum y Artista, opcionalmente por Canción.
* Usa GTK para la interfaz gráfica.
*/

#include <gtk/gtk.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

typedef struct {
    GtkWidget *album_entrada;
    GtkWidget *artista_entrada;
    GtkWidget *cancion_entrada;
    GtkTextBuffer *buffer_resultado; 
} AppWidgets;

static void buscar_accion(GtkButton *button, gpointer user_data) {
    AppWidgets *widgets = (AppWidgets *)user_data;
    
    const char *album_q = gtk_entry_get_text(GTK_ENTRY(widgets->album_entrada));
    const char *artista_q = gtk_entry_get_text(GTK_ENTRY(widgets->artista_entrada));
    const char *cancion_q= gtk_entry_get_text(GTK_ENTRY(widgets->cancion_entrada));

    // Validar que los campos obligatorios no estén vacíos 
    if (strlen(album_q) == 0 || strlen(artista_q) == 0) {
        gtk_text_buffer_set_text(widgets->buffer_resultado, "Error: Los campos de Álbum y Artista son obligatorios.", -1);
        return;
    }
    
    // Construir la cadena de consulta para la tubería
    char query_string[1024];
    snprintf(query_string, sizeof(query_string), "%s|%s|%s", album_q, artista_q, cancion_q);

    const char *ui_pipe = "/tmp/ui_to_search_pipe";
    const char *search_pipe = "/tmp/search_to_ui_pipe";
    char buffer_resultado[65536]; // Buffer grande para recibir múltiples resultados

    int fd_write = open(ui_pipe, O_WRONLY);
    if (fd_write < 0) {
        gtk_text_buffer_set_text(widgets->buffer_resultado, "Error: El proceso de búsqueda no está activo.", -1);
        return;
    }
    write(fd_write, query_string, strlen(query_string) + 1);
    close(fd_write);
    
    gtk_text_buffer_set_text(widgets->buffer_resultado, "Buscando...", -1);
    while (gtk_events_pending()) { gtk_main_iteration(); }

    int fd_read = open(search_pipe, O_RDONLY);
    ssize_t bytes_read = read(fd_read, buffer_resultado, sizeof(buffer_resultado) - 1);
    close(fd_read);
    
    if (bytes_read > 0) {
        buffer_resultado[bytes_read] = '\0';
        gtk_text_buffer_set_text(widgets->buffer_resultado, buffer_resultado, -1);
    } else {
        gtk_text_buffer_set_text(widgets->buffer_resultado, "Error al recibir la respuesta del buscador.", -1);
    }
}

int main(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Buscador Avanzado de Spotify");
    gtk_window_set_default_size(GTK_WINDOW(window), 700, 500);
    gtk_container_set_border_width(GTK_CONTAINER(window), 15);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_container_add(GTK_CONTAINER(window), grid);

    AppWidgets *widgets = g_slice_new(AppWidgets);

    // Criterios de búsqueda
    widgets->album_entrada = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(widgets->album_entrada), "Criterio 1: Obligatorio");
    widgets->artista_entrada = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(widgets->artista_entrada), "Criterio 2: Obligatorio");
    widgets->cancion_entrada = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(widgets->cancion_entrada), "Criterio 3: Opcional");

    // Área de resultados con Scroll
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_hexpand(scrolled_window, TRUE);
    gtk_widget_set_vexpand(scrolled_window, TRUE);
    GtkWidget *vista_salir = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(vista_salir), FALSE);
    widgets->buffer_resultado = gtk_text_view_get_buffer(GTK_TEXT_VIEW(vista_salir));
    gtk_container_add(GTK_CONTAINER(scrolled_window), vista_salir);

    // Botones
    GtkWidget *boton_buscar = gtk_button_new_with_label("4. Realizar Búsqueda");
    GtkWidget *boton_salir = gtk_button_new_with_label("5. Salir");
    
    // Añadir widgets a la cuadricula
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Álbum:"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), widgets->album_entrada, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Artista:"), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), widgets->artista_entrada, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Canción (Opcional):"), 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), widgets->cancion_entrada, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), boton_buscar, 0, 3, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), boton_salir, 2, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Resultados:"), 0, 4, 3, 1);
    gtk_grid_attach(GTK_GRID(grid), scrolled_window, 0, 5, 3, 1);

    g_signal_connect(boton_buscar, "clicked", G_CALLBACK(buscar_accion), widgets);
    g_signal_connect(boton_salir, "clicked", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(window);
    gtk_main();

    g_slice_free(AppWidgets, widgets);
    return 0;
}
