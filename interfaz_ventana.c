#include <SDL.h>
#include <SDL_image.h>  // Biblioteca para cargar imagenes (tanques)
#include <SDL_ttf.h> // Biblioteca para fuentes (letras)
#include <stdio.h>
#include <stdlib.h>
#include <math.h> 
#include <stbool.h> 

// Constantes (Para el dibujo y renderizado)
#define TILE_SIZE 40
#define ALTURA_LEYENDA 80
#define MAX_BALAS 10 
#define BALA_SIZE 8 

// Estructura de las bala (La lógica la llena, la interfaz la dibuja)
typedef struct {
    int fila;
    int col;
    int direccion;
    int activa;
    int propietario; // 1 = Amarillo, 2 = Verde
    float x_f;
    float y_f;
} Bala;

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
void dibujar_bala(float x_f, float y_f);
void dibujar_mapa(int **matriz, int filas, int columnas, int tanque1_dir, int tanque2_dir, Bala *balas, int num_balas, int t1_vidas, int t2_vidas, bool game_over, int winner);

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
        "BATTLE CITY - P1(Amarillo):FLECHAS | P2(Verde):WASD",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        ancho,
        alto,
        SDL_WINDOW_SHOWN
    );
    
    renderizador = SDL_CreateRenderer(ventana, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    fuente = TTF_OpenFont("arial.ttf", 16); // Cargar la fuente. 
    if (fuente == NULL) {
        fprintf(stderr, "ADVERTENCIA: Error al cargar fuente (arial.ttf). Los contadores no se mostrarán: %s\n", TTF_GetError());
    }

    SDL_Surface *superficie_temp = NULL;  // Carga de texturas de los tanques 

    superficie_temp = IMG_Load("tanque_amarillo.jpg");
    if (superficie_temp) {
        textura_tanque_amarillo = SDL_CreateTextureFromSurface(renderizador, superficie_temp);
        SDL_FreeSurface(superficie_temp);
    } else {
        fprintf(stderr, "No se pudo cargar la imagen tanque_amarillo.jpg: %s\n", IMG_GetError());
    }

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

// 3. Dibujar un rectángulo relleno (Tablero)
void dibujar_rect(int x, int y, int w, int h, int r, int g, int b) {
    SDL_SetRenderDrawColor(renderizador, r, g, b, 255);
    SDL_Rect rect = {x, y, w, h};
    SDL_RenderFillRect(renderizador, &rect);
}

// 4. Dibujar texto 
void dibujar_texto(const char *texto, int x, int y, int r, int g, int b) {
    if (fuente == NULL) return; 

    SDL_Color color = {r, g, b, 255};
    SDL_Surface *superficie_texto = TTF_RenderText_Solid(fuente, texto, color);
    if (superficie_texto == NULL) {
        return;
    }

    SDL_Texture *textura_texto = SDL_CreateTextureFromSurface(renderizador, superficie_texto);
    SDL_Rect destino = {x, y, superficie_texto->w, superficie_texto->h};
    
    SDL_RenderCopy(renderizador, textura_texto, NULL, &destino);
    
    SDL_DestroyTexture(textura_texto);
    SDL_FreeSurface(superficie_texto);
}

// FUNCIONES DE DIBUJO DE ELEMENTOS
// 5. Dibuja el tanque usando la textura y la rota
void dibujar_tanque_con_textura(SDL_Texture *textura, int x, int y, int direccion) {
    if (textura == NULL) {
        // Fallback si no se cargó la textura
        dibujar_rect(x, y, TILE_SIZE, TILE_SIZE, 128, 128, 128); 
        return;
    }

    SDL_Rect destino = {x, y, TILE_SIZE, TILE_SIZE};
    
    double angulo = 0.0; //Dirección del tanque
    if (direccion == 0) angulo = 0.0;   // Arriba
    else if (direccion == 1) angulo = 90.0;  // Derecha
    else if (direccion == 2) angulo = 180.0; // Abajo
    else if (direccion == 3) angulo = 270.0; // Izquierda
    
    SDL_RenderCopyEx(renderizador, textura, NULL, &destino, angulo, NULL, SDL_FLIP_NONE);
}

// 6. Dibuja el Muro Destruible
void dibujar_muro_destruible(int x, int y) { 
    int size = TILE_SIZE;
    int brick_w = size / 4;
    int brick_h = size / 4;
    
    int color_base_r = 184; int color_base_g = 100; int color_base_b = 56;
    int color_claro_r = 216; int color_claro_g = 132; int color_claro_b = 88;
    
    dibujar_rect(x, y, size, size, color_base_r, color_base_g, color_base_b);
    
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            int bx = x + j * brick_w;
            int by = y + i * brick_h;
            
            if (i % 2 == 1) {
                bx += brick_w / 2;
            }
            
            if (bx < x + size - brick_w/2) {
                dibujar_rect(bx + 1, by + 1, brick_w - 2, brick_h - 2, 
                             color_claro_r, color_claro_g, color_claro_b);
                
                dibujar_rect(bx + 1, by + brick_h - 2, brick_w - 2, 1,
                             color_base_r - 30, color_base_g - 30, color_base_b - 30);
            }
        }
    }
}

// 7. Dibuja el Muro Sólido
void dibujar_muro_solido(int x, int y) { 
    int size = TILE_SIZE;
    int gris_base = 160;
    int gris_claro = 200;
    int gris_oscuro = 120;
    
    dibujar_rect(x, y, size, size, gris_base, gris_base, gris_base);
    
    int block_size = size / 2;
    
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            int bx = x + j * block_size;
            int by = y + i * block_size;
            
            dibujar_rect(bx + 2, by + 2, block_size - 4, block_size - 4, 
                         gris_claro, gris_claro, gris_claro);
            
            dibujar_rect(bx + 2, by + 2, block_size - 4, 2, 220, 220, 220);
            dibujar_rect(bx + 2, by + 2, 2, block_size - 4, 220, 220, 220);
            
            dibujar_rect(bx + 2, by + block_size - 4, block_size - 4, 2,
                         gris_oscuro, gris_oscuro, gris_oscuro);
            dibujar_rect(bx + block_size - 4, by + 2, 2, block_size - 4,
                         gris_oscuro, gris_oscuro, gris_oscuro);
        }
    }
}

// 8. Dibuja la Bala
void dibujar_bala(float x_f, float y_f) {
    int tx = (int)(x_f * TILE_SIZE - (float)BALA_SIZE/2.0f); 
    int ty = (int)(y_f * TILE_SIZE - (float)BALA_SIZE/2.0f);
    
    dibujar_rect(tx, ty, BALA_SIZE, BALA_SIZE, 255, 255, 255);
    dibujar_rect(tx + 2, ty + 2, BALA_SIZE - 4, BALA_SIZE - 4, 255, 255, 0);
}

// Función principal - conecta con la logica y lo que esta pasando 
// 9. Dibuja el mapa completo recibiendo el estado actual del juego
void dibujar_mapa(int **matriz, int filas, int columnas, int tanque1_dir, int tanque2_dir, Bala *balas, int num_balas, int t1_vidas, int t2_vidas, bool game_over, int winner) {
    SDL_SetRenderDrawColor(renderizador, 0, 0, 0, 255);
    SDL_RenderClear(renderizador);
    
    // Dibujar el mapa (muros y tanques)
    for (int i = 0; i < filas; i++) {
        for (int j = 0; j < columnas; j++) {
            int x = j * TILE_SIZE;
            int y = i * TILE_SIZE;
            int que_hay = matriz[i][j];
            
            if (que_hay == 1) {
                dibujar_muro_destruible(x, y);
            }
            else if (que_hay == 2) {
                dibujar_muro_solido(x, y);
            }
            else if (que_hay == 3 && t1_vidas > 0) { // Tanque Amarillo (P1)
                dibujar_tanque_con_textura(textura_tanque_amarillo, x, y, tanque1_dir);
            }
            else if (que_hay == 4 && t2_vidas > 0) { // Tanque Verde (P2)
                dibujar_tanque_con_textura(textura_tanque_verde, x, y, tanque2_dir);
            }
        }
    }
    
    // Dibujar balas activas
    for (int i = 0; i < num_balas; i++) {
        if (balas[i].activa) {
            dibujar_bala(balas[i].x_f, balas[i].y_f);
        }
    }
    
    // Dibujar leyenda en la parte inferior (hay que arreglarlo)
    int y_leyenda_base = filas * TILE_SIZE;
    int y_simbolos = y_leyenda_base + 10;
    int x_inicio = 20;
    
    // Fondo de la leyenda
    SDL_SetRenderDrawColor(renderizador, 30, 30, 30, 255);
    SDL_Rect fondo_leyenda = {0, y_leyenda_base, columnas * TILE_SIZE, ALTURA_LEYENDA};
    SDL_RenderFillRect(renderizador, &fondo_leyenda);
    
    // Símbolos y Texto de Muros y Balas
    dibujar_rect(x_inicio, y_simbolos, 20, 20, 216, 132, 88); // Muro
    dibujar_texto("Muro Destruible", x_inicio + 30, y_simbolos + 2, 255, 255, 255);

    dibujar_rect(x_inicio + 180, y_simbolos, 20, 20, 160, 160, 160); // Acero
    dibujar_texto("Muro Solido", x_inicio + 210, y_simbolos + 2, 255, 255, 255);

    dibujar_rect(x_inicio + 360, y_simbolos + 5, 10, 10, 255, 255, 255); // Bala
    dibujar_texto("Bala", x_inicio + 380, y_simbolos + 2, 255, 255, 255);

    // Contadores de Vidas 
    char buf1[50];
    sprintf(buf1, "P1 Vidas: %d", t1_vidas);
    dibujar_rect(x_inicio + 470, y_simbolos, 20, 20, 255, 200, 0); // Amarillo
    dibujar_texto(buf1, x_inicio + 500, y_simbolos + 2, 255, 255, 100); 

    char buf2[50];
    sprintf(buf2, "P2 Vidas: %d", t2_vidas);
    dibujar_rect(x_inicio + 670, y_simbolos, 20, 20, 0, 150, 0); // Verde oscuro
    dibujar_texto(buf2, x_inicio + 700, y_simbolos + 2, 100, 255, 100); 
    
    // Texto inferior con Controles
    int y_texto = y_leyenda_base + 50;
    dibujar_texto("P1 (Amarillo) - Mov: FLECHAS (Mantener) | Disparo: ESPACIO", 20, y_texto, 255, 255, 100);
    dibujar_texto("P2 (Verde) - Mov: WASD (Pulsar 1 vez) | Disparo: L-SHIFT/R-SHIFT", 550, y_texto, 100, 255, 100);

    // Logica de fin de juego 
    if (game_over) {
        SDL_SetRenderDrawBlendMode(renderizador, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderizador, 0, 0, 0, 150); 
        SDL_Rect pantalla_completa = {0, 0, columnas * TILE_SIZE, filas * TILE_SIZE};
        SDL_RenderFillRect(renderizador, &pantalla_completa);
        SDL_SetRenderDrawBlendMode(renderizador, SDL_BLENDMODE_NONE);

        const char *msg;
        int r, g, b;

        if (winner == 1) { // Mensaje de ganador
            msg = "JUGADOR 1 (AMARILLO) GANA!"; 
            r = 255; g = 255; b = 100; 
        } else {
            msg = "JUGADOR 2 (VERDE) GANA!";
            r = 100; g = 255; b = 100; 
        }
        
        int texto_x = (columnas * TILE_SIZE) / 2 - 150; 
        int texto_y = (filas * TILE_SIZE) / 2 - 20;
        
        dibujar_texto(msg, texto_x, texto_y, r, g, b);
        dibujar_texto("Presiona ESC para salir", texto_x + 20, texto_y + 30, 255, 255, 255);
    }
    
    // Presenta todo el dibujo en pantalla
    SDL_RenderPresent(renderizador);
}
