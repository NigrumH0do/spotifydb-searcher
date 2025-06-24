# ---------------------------------
# Makefile para el Proyecto de Búsqueda Spotify
# base de datos https://huggingface.co/datasets/bigdata-pw/Spotify
# ---------------------------------

# --- Variables de Compilación ---

# Compilador de C a utilizar
CC = gcc

# Banderas (flags) para el compilador:
CFLAGS = -g -Wall

# --- Variables para Librerías Externas (GTK3) ---
GTK_CFLAGS = $(shell pkg-config --cflags gtk+-3.0)
GTK_LIBS = $(shell pkg-config --libs gtk+-3.0)


# --- Nombres de Archivos Fuente y Ejecutables ---

INDEXER_SRC = src/indexer.c
SEARCHER_SRC = src/searcher.c
UI_SRC = src/ui_main.c

INDEXER_EXEC = indexer
SEARCHER_EXEC = searcher
UI_EXEC = ui_program


# --- Reglas de Compilación ---
all: $(INDEXER_EXEC) $(SEARCHER_EXEC) $(UI_EXEC)
$(INDEXER_EXEC): $(INDEXER_SRC)
	$(CC) $(CFLAGS) -o $(INDEXER_EXEC) $(INDEXER_SRC)
$(SEARCHER_EXEC): $(SEARCHER_SRC)
	$(CC) $(CFLAGS) -o $(SEARCHER_EXEC) $(SEARCHER_SRC)
$(UI_EXEC): $(UI_SRC)
	$(CC) $(CFLAGS) -o $(UI_EXEC) $(UI_SRC) $(GTK_CFLAGS) $(GTK_LIBS)


# --- Reglas de Utilidad y Ejecución ---

.PHONY: all clean index run-searcher run-ui

# Regla para ejecutar el indexador. Depende de que el ejecutable ya esté compilado.
index: $(INDEXER_EXEC)
	@echo "--- Ejecutando el Indexador para crear spotify.index ---"
	./$(INDEXER_EXEC)

# Regla para ejecutar el buscador. El @ al principio evita que se imprima el comando.
buscador: $(SEARCHER_EXEC)
	@echo "--- Iniciando el proceso de búsqueda  ---"
	./$(SEARCHER_EXEC)

# Regla para ejecutar la interfaz gráfica.
ui: $(UI_EXEC)
	@echo "--- Iniciando la Interfaz Gráfica ---"
	./$(UI_EXEC)

# Regla para limpiar el directorio de spotify.index
clean:
	@echo "--- Limpiando archivo generado ---"
	rm -f $(INDEXER_EXEC) $(SEARCHER_EXEC) $(UI_EXEC) spotify.index
