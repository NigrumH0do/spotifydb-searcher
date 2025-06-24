# Proyecto 1: Sistema de Búsqueda de Datos de Spotify

Este proyecto implementa un sistema de búsqueda de alto rendimiento en C para consultar un dataset masivo de canciones de Spotify (`25,681,792 filas`). El sistema está diseñado con una arquitectura de múltiples procesos y utiliza técnicas de indexación y comunicación entre procesos (IPC) para manejar grandes volúmenes de datos (7 GB) de manera eficiente y rapida.

## Autores

* **Nombres:** José Leonardo Pinilla, Frank Olmos, Juan David Ladino
* **Curso:** Sistemas Operativos
* **Universidad:** Universidad Nacional de Colombia

## Características Principales

* **Arquitectura Multi-proceso:** El sistema se divide en tres procesos independientes: un indexador, un proceso de búsqueda y una interfaz de usuario.
* **Comunicación IPC:** La comunicación entre la interfaz y el buscador se realiza de forma robusta y segura mediante **tuberías nombradas (Named Pipes/FIFOs)**.
* **Indexación Eficiente:** Se implementa un proceso de indexación (`indexer`) que lee el dataset de 7 GB una sola vez y genera un **índice binario** optimizado para búsquedas rápidas.
* **Tabla Hash:** El núcleo de la búsqueda se basa en una **tabla hash** con manejo de colisiones (encadenamiento en disco) para un acceso a los datos en tiempo casi constante.
* **Bajo Consumo de Memoria:** El proceso buscador (`searcher`) fue diseñado para cumplir un estricto límite de **<10 MB de RAM**, manteniendo el dataset y la mayor parte del índice en disco.
* **Interfaz Gráfica (GUI):** Se desarrolló una interfaz de usuario amigable con la librería **GTK3**, permitiendo una interacción más allá de la consola.
* **Lógica de Búsqueda Avanzada:**
    * Búsqueda por criterios obligatorios (`Álbum` y `Artista`).
    * Filtro opcional con búsqueda parcial e insensible a mayúsculas/minúsculas para el `Nombre de la Canción`.
    * Salida de resultados formateada para una fácil lectura.

## Requisitos Previos

Para compilar y ejecutar este proyecto, necesitarás:

* El compilador `gcc` y las herramientas de construcción `make`.
* La librería de desarrollo de GTK3.

## Dataset

El proyecto utiliza el dataset "Spotify Tracks DB" disponible en Hugging Face.
* **Enlace:** [https://huggingface.co/datasets/bigdata-pw/Spotify](https://huggingface.co/datasets/bigdata-pw/Spotify)

El archivo `spotify_data.csv` que usa el proyecto fue generado a partir de los .parquet alojados en HuggingFace
(Se usó la biblioteca de Python datasets para unificar los archivos)
- Adjunto el código que use para unificarlo.
## Compilación

El proyecto incluye un `Makefile` que automatiza todo el proceso de compilación. Simplemente ejecuta:

```bash
make
```
O también:
```bash
make all
```
Esto generará los tres ejecutables necesarios: `indexer`, `searcher` y `ui_program`.

## Modo de Uso

Para que todo funcione, sigue estos pasos en orden:

1.  **Generar el Índice (una sola vez, suponiendo que ya se tiene el spotify_data.csv):**
    Este paso lee el archivo `spotify_data.csv` de 7 GB y crea `spotify.index`. **Puede tardar un par de segundos.**
    ```bash
    make index
    ```

2.  **Iniciar el Servidor de Búsqueda:**
    Abre una terminal y ejecuta el siguiente comando. **Esta terminal debe permanecer abierta** mientras usas la aplicación.
    ```bash
    make buscador
    ```

3.  **Iniciar la Interfaz Gráfica:**
    Abre una **segunda terminal** y ejecuta:
    ```bash
    make ui
    ```

4.  **Realizar Búsquedas:** Utiliza la ventana que aparecerá para introducir los criterios y buscar.

## Limpieza

Para eliminar todos los archivos generados (ejecutables y el archivo de índice), ejecuta:
```bash
make clean
```
