#include <stdio.h>
#include <stdlib.h>

// Códigos de colores ANSI
#define COLOR_RESET   "\x1b[0m"
#define COLOR_ROJO    "\x1b[31m"
#define COLOR_VERDE   "\x1b[32m"
#define COLOR_AMARILLO "\x1b[33m"
#define COLOR_GRIS    "\x1b[90m"
#define COLOR_CYAN    "\x1b[36m"


void limpiar_pantalla() {
    system("clear");   // Mac / Linux
    // system("cls");  // Windows
}

void visualizar_laberinto(int **matriz, int filas, int columnas) {

    printf("\n");
    printf("╔════════════════════════════════╗\n");
    printf("║     BATTLE CITY - GAME VIEW    ║\n");
    printf("╚════════════════════════════════╝\n\n");

    for (int i = 0; i < filas; i++) {
        for (int j = 0; j < columnas; j++) {

            int celda = matriz[i][j];

            switch (celda) {
                case 0:
                    printf("  ");
                    break;

                case 1: // destructible
                    printf("%s▓▓%s", COLOR_AMARILLO, COLOR_RESET);
                    break;

                case 2: // muro
                    printf("%s██%s", COLOR_GRIS, COLOR_RESET);
                    break;

                case 3: //t1
                    printf("%sT1%s", COLOR_VERDE, COLOR_RESET);
                    break;

                case 4: //t2
                    printf("%sT2%s", COLOR_ROJO, COLOR_RESET);
                    break;

                case 5: //bala
                    printf("%sᗤ%s", COLOR_CYAN, COLOR_RESET);
                    break;

                case 6: 
                    printf("%s[]%s", COLOR_CYAN, COLOR_RESET);
                    break;
                default:
                    printf("??");
                    break;
            }
        }
        printf("\n");}

    printf("\nLeyenda: ");
    printf("%s▓▓%s Destruible  ", COLOR_AMARILLO, COLOR_RESET);
    printf("%s██%s Muro  ", COLOR_GRIS, COLOR_RESET);
    printf("%sT1%s Jugador1  ", COLOR_VERDE, COLOR_RESET);
    printf("%sT2%s Jugador2\n\n", COLOR_ROJO, COLOR_RESET);
}

void actualizar_visualizacion(int **matriz, int filas, int columnas) {
    limpiar_pantalla();
    visualizar_laberinto(matriz, filas, columnas);
}
