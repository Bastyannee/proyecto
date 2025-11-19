// tarea1.c


#include <stdio.h>
#include <stdlib.h>
#include <time.h>

//VARIABLES GLOBALES 
int filas, columnas;
int **matriz;   // la matriz dinámica que piden

// Reserva memoria para la matriz 
void crear_matriz() {
    matriz = malloc(filas * sizeof(int*));
    for (int i = 0; i < filas; i++)
        matriz[i] = malloc(columnas * sizeof(int));
}

// Libera la matriz
void liberar_matriz() {
    for (int i = 0; i < filas; i++) free(matriz[i]);
    free(matriz);
}


void guardar_en_archivo(const char* nombre) {
    FILE* f = fopen(nombre, "w");
    fprintf(f, "%d %d\n", filas, columnas);
    for (int i = 0; i < filas; i++) {
        for (int j = 0; j < columnas; j++)
            fprintf(f, "%d ", matriz[i][j]);
        fprintf(f, "\n");
    }
    fclose(f);
    printf("Laberinto guardado en %s\n", nombre);
}


void generar_aleatorio(int f, int c) {
    filas = f; columnas = c;
    crear_matriz();

    // Todo vacío
    for (int i = 0; i < filas; i++)
        for (int j = 0; j < columnas; j++)
            matriz[i][j] = 0;

    // Bordes irrompibles
    for (int i = 0; i < filas; i++) matriz[i][0] = matriz[i][c-1] = 2;
    for (int j = 0; j < columnas; j++) matriz[0][j] = matriz[f-1][j] = 2;

    // Bloques destruibles aleatorios (~20-25%)
    int cantidad = (filas * columnas) / 7;
    for (int k = 0; k < cantidad; k++) {
        int x = 1 + rand() % (filas-2);
        int y = 1 + rand() % (columnas-2);
        if (matriz[x][y] == 0) matriz[x][y] = 1;
    }

    // Tanques siempre en posiciones libres
    matriz[1][1] = 3;                    // Jugador 1
    matriz[filas-2][columnas-2] = 4;     // Jugador 2
}


void imprimir() {
    for (int i = 0; i < filas; i++) {
        for (int j = 0; j < columnas; j++)
            printf("%d ", matriz[i][j]);
        printf("\n");
    }
}

int main() {
    srand(time(NULL));

    // Genera y guarda los 3 mapas de ejemplo
    generar_aleatorio(6, 6);
    guardar_en_archivo("mapa6x6.txt");

    generar_aleatorio(7, 7);
    guardar_en_archivo("mapa7x7.txt");

    generar_aleatorio(8, 8);
    guardar_en_archivo("mapa8x8.txt");
    printf("\n");
    
    

    liberar_matriz();
    return 0;
}
