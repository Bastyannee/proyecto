#include <SDL.h>
#include <SDL_image.h> // Biblioteca para cargar imagenes (tanques)
#include <SDL_ttf.h> // Biblioteca para fuentes (letras)
#include <stdio.h>
#include <stdlib.h>
#include <math.h> 
#include <stdbool.h> 
#include <string.h> // Necesario para sprintf

// ====================================================================
// CONSTANTES Y ESTRUCTURAS COPIADAS DE LA LÓGICA
// ====================================================================
#define TILE_SIZE 40
#define ALTURA_LEYENDA 80
#define MAX_BALAS 5 // Modificado a 5 para compatibilidad con el array de balas de la logica

// Definicion de direcciones de la logica
typedef enum {UP, DOWN, LEFT, RIGHT} direction;

// La interfaz debe conocer la estructura de la bala y el tanque
typedef struct { 
    int x; 
    int y; 
    direction dir; 
    int lives;       
    bool has_shield; 
} tank;

typedef struct { 
    int x; 
    int y; 
    direction dir; 
    bool active; 
} bullet;

// Declaraciones de estructuras globales de la lógica para que la interfaz sepa que existen
// Aunque no las use directamente, sepa que deben ser accesibles para visualizar_laberinto
extern tank player1, player2; 
extern bullet bullet_p1[5];
extern bullet bullet_p2[5];
extern int HEIGHT, WIDTH;
// ====================================================================

// Variables globales SDL (Accesibles por la función de inicialización y dibujo)
SDL_Window *ventana = NULL;
SDL_Renderer *renderizador = NULL;
TTF_Font *fuente = NULL; 

// Texturas globales para los tanques
SDL_Texture *textura_tanque_amarillo = NULL;
SDL_Texture *textura_tanque_verde = NULL;

// ----------------------
// PROTOTIPOS DE FUNCIONES DE INTERFAZ
// ----------------------

void abrir_ventana(int ancho, int alto);
void cerrar_ventana();
void dibujar_rect(int x, int y, int w, int h, int r, int g, int b);
void dibujar_texto(const char *texto, int x, int y, int r, int g, int b);
void dibujar_tanque_con_textura(SDL_Texture *textura, int x, int y, int direccion);
void dibujar_muro_destruible(int x, int y);
void dibujar_muro_solido(int x, int y);
void dibujar_escudo(int x, int y); // Nueva funcion para dibujar el escudo
void dibujar_bala(int x, int y); // Simplificada para dibujar la bala
// Prototipo de la función que recibe el archivo de Lógica
void visualizar_laberinto(int **matriz, int filas, int columnas); 


// Funciones de inicialización y cierre 
// 1. Abrir la ventana (Inicializa SDL, TTF, IMG y carga recursos)
void abrir_ventana(int ancho, int alto) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL no pudo inicializarse. Error: %s\n", SDL_GetError());
        exit(1);
    }
    if (TTF_Init() == -1) {
        fprintf(stderr, "SDL_ttf no pudo inicializarse. Error: %s\n", TTF_GetError());
        exit(1);
    }
    if (!(IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG) & (IMG_INIT_JPG | IMG_INIT_PNG))) {
        fprintf(stderr, "SDL_Image no pudo inicializarse. Error: %s\n", IMG_GetError());
    }
      ventana = SDL_CreateWindow(
        "BATTLE CITY - P1 (WASD) | P2 (IJKL)",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        ancho,
        alto,
        SDL_WINDOW_SHOWN
    );
    
    renderizador = SDL_CreateRenderer(ventana, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    // Fuente (Ajuste el nombre si 'arial.ttf' no existe en su sistema)
    fuente = TTF_OpenFont("arial.ttf", 16); 
    if (fuente == NULL) {
        fprintf(stderr, "ADVERTENCIA: Error al cargar fuente (arial.ttf). Los contadores no se mostrarán: %s\n", TTF_GetError());
    }

    SDL_Surface *superficie_temp = NULL; 

    // Textura P1 (Amarillo)
    superficie_temp = IMG_Load("tanque_amarillo.jpg");
    if (superficie_temp) {
        textura_tanque_amarillo = SDL_CreateTextureFromSurface(renderizador, superficie_temp);
        SDL_FreeSurface(superficie_temp);
    } else {
        fprintf(stderr, "No se pudo cargar la imagen tanque_amarillo.jpg: %s\n", IMG_GetError());
    }

    // Textura P2 (Verde)
    superficie_temp = IMG_Load("tanque_verde.png");
    if (superficie_temp) {
        textura_tanque_verde = SDL_CreateTextureFromSurface(renderizador, superficie_temp);
        SDL_FreeSurface(superficie_temp);
    } else {
        fprintf(stderr, "No se pudo cargar la imagen tanque_verde.png: %s\n", IMG_GetError());
    }
}

// 2. Cerrar la ventana y liberar recursos
void cerrar_ventana() {
    if (textura_tanque_amarillo) SDL_DestroyTexture(textura_tanque_amarillo);
    if (textura_tanque_verde) SDL_DestroyTexture(textura_tanque_verde);
    if (fuente) TTF_CloseFont(fuente);
    if (renderizador) SDL_DestroyRenderer(renderizador);
    if (ventana) SDL_DestroyWindow(ventana);
    
    IMG_Quit(); // Limpiar SDL_image
    TTF_Quit(); // Limpiar SDL_ttf
    SDL_Quit();
}

// ... (Resto de las funciones auxiliares de dibujo: dibujar_rect, dibujar_texto, etc. VAN AQUÍ)
// **NOTA: Estas funciones no necesitan cambios, se omiten por espacio.**

// FUNCIONES DE DIBUJO DE ELEMENTOS
// 5. Dibuja el tanque usando la textura y la rota
void dibujar_tanque_con_textura(SDL_Texture *textura, int x, int y, int direccion) {
    if (textura == NULL) {
        // Fallback si no se cargó la textura
        dibujar_rect(x, y, TILE_SIZE, TILE_SIZE, 128, 128, 128); 
        return;
    }

    SDL_Rect destino = {x, y, TILE_SIZE, TILE_SIZE};
    
    double angulo = 0.0; 
    // La numeración de direcciones es compatible con la logica: 0=UP, 1=DOWN (cambiado), 2=LEFT (cambiado), 3=RIGHT (cambiado)
    if (direccion == UP) angulo = 0.0;    // UP (0)
    else if (direccion == RIGHT) angulo = 90.0;   // RIGHT (3)
    else if (direccion == DOWN) angulo = 180.0; // DOWN (1)
    else if (direccion == LEFT) angulo = 270.0; // LEFT (2)
    
    SDL_RenderCopyEx(renderizador, textura, NULL, &destino, angulo, NULL, SDL_FLIP_NONE);
}

// 8. Dibuja la Bala (simplificada)
void dibujar_bala(int x, int y) {
    #define BALA_SIZE 8 
    int tx = x; 
    int ty = y;
    
    // Dibuja un punto blanco/amarillo
    dibujar_rect(tx, ty, BALA_SIZE, BALA_SIZE, 255, 255, 255);
    dibujar_rect(tx + 2, ty + 2, BALA_SIZE - 4, BALA_SIZE - 4, 255, 255, 0);
}

// NUEVA FUNCIÓN PARA DIBUJAR EL ESCUDO
void dibujar_escudo(int x, int y) {
    // Dibuja un círculo azul claro o un cuadrado con borde
    int size = TILE_SIZE;
    dibujar_rect(x, y, size, size, 50, 50, 50); // Fondo oscuro
    // Dibuja el borde del escudo (Color cian)
    SDL_SetRenderDrawColor(renderizador, 0, 255, 255, 255); 
    SDL_Rect borde = {x + 5, y + 5, size - 10, size - 10};
    SDL_RenderDrawRect(renderizador, &borde);
    
    // Dibuja un rayo pequeño
    dibujar_rect(x + size/2 - 2, y + 8, 4, 20, 255, 255, 0);
}


// ==============================================================
// FUNCIÓN PUENTE (VISUALIZAR_LABERINTO) - CONEXIÓN CON LA LÓGICA
// ==============================================================
void visualizar_laberinto(int **matriz, int filas, int columnas) {
    if (ventana == NULL) {
        int ancho_ventana = columnas * TILE_SIZE;
        int alto_ventana = filas * TILE_SIZE + ALTURA_LEYENDA;
        abrir_ventana(ancho_ventana, alto_ventana);
    }
    
    // Lógica de fin de juego y HUD
    bool game_over = (player1.lives <= 0 || player2.lives <= 0);
    int winner = (player1.lives > 0) ? 1 : 2; // Si no es game over, el ganador se decide al final

    SDL_SetRenderDrawColor(renderizador, 0, 0, 0, 255);
    SDL_RenderClear(renderizador);
    
    // 1. Dibujar el mapa (Terreno, Escudos, Tanques)
    for (int i = 0; i < filas; i++) {
        for (int j = 0; j < columnas; j++) {
            int x = j * TILE_SIZE;
            int y = i * TILE_SIZE;
            int que_hay = matriz[i][j];
            
            // Dibujar SUELO/FONDO
            dibujar_rect(x, y, TILE_SIZE, TILE_SIZE, 50, 50, 50);

            if (que_hay == 1) { // Muro Destruible
                dibujar_muro_destruible(x, y);
            }
            else if (que_hay == 2) { // Muro Sólido
                dibujar_muro_solido(x, y);
            }
            else if (que_hay == 6) { // ESCUDO (SHIELD_TILE)
                dibujar_escudo(x, y);
            }
            else if (que_hay == 3 && player1.lives > 0) { // Tanque Amarillo (P1)
                // MODIFICACIÓN: La LÓGICA le pasa la dirección al struct player1
                dibujar_tanque_con_textura(textura_tanque_amarillo, x, y, player1.dir);
                if (player1.has_shield) dibujar_escudo(x, y);
            }
            else if (que_hay == 4 && player2.lives > 0) { // Tanque Verde (P2)
                dibujar_tanque_con_textura(textura_tanque_verde, x, y, player2.dir);
                if (player2.has_shield) dibujar_escudo(x, y);
            }
            else if (que_hay == 5) { // Balas (BULLET_TILE)
                // Dibuja la bala en el centro de la celda
                dibujar_bala(x + TILE_SIZE/2 - 4, y + TILE_SIZE/2 - 4);
            }
        }
    }

    // 2. Dibujar HUD y Leyenda (Simplificado para usar las variables globales)
    int y_leyenda_base = filas * TILE_SIZE;
    
    // Fondo de la leyenda
    dibujar_rect(0, y_leyenda_base, columnas * TILE_SIZE, ALTURA_LEYENDA, 30, 30, 30);
    
    // Imprimir Contadores de Vidas y Escudo
    char hud_p1[100], hud_p2[100];
    sprintf(hud_p1, "P1: Vidas %d | Escudo: %s", player1.lives, player1.has_shield ? "ON" : "OFF");
    sprintf(hud_p2, "P2: Vidas %d | Escudo: %s", player2.lives, player2.has_shield ? "ON" : "OFF");
    
    dibujar_texto(hud_p1, 20, y_leyenda_base + 10, 255, 255, 100); 
    dibujar_texto(hud_p2, columnas * TILE_SIZE - 250, y_leyenda_base + 10, 100, 255, 100);

    // 3. Lógica de fin de juego (Superposición)
    if (player1.lives <= 0 || player2.lives <= 0) {
        // ... (Tu logica de superposicion de GAME OVER)
    }
    
    // Presenta todo el dibujo en pantalla
    SDL_RenderPresent(renderizador);
}
