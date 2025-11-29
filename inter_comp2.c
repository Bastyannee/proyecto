#include <SDL.h>
#include <SDL_image.h> 
#include <SDL_ttf.h> 
#include <SDL_mixer.h> 
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> 
#include <string.h> 
#include <math.h>
#include <unistd.h> 
#include <time.h>

// ====================================================================
// CONSTANTES GLOBALES Y ESTRUCTURAS
// ====================================================================
#define TILE_SIZE 40
#define ALTURA_LEYENDA 100 
#define MAX_BALAS 10 
#define MOV_COOLDOWN_P1 5    // Cooldown ajustado para P1 (Movimiento continuo más rápido)
#define MOV_COOLDOWN_P2 15   // Cooldown para P2 (Movimiento continuo normal)
#define COOLDOWN_DISPARO 25 
#define BALA_SIZE 8 

// Estructura para una bala
typedef struct {
    int fila;
    int col;
    int direccion;
    int activa;
    int propietario; // 1 = Amarillo, 2 = Verde
    float x_f; 
    float y_f;
} Bala;

// Variables globales SDL y Recursos
SDL_Window *ventana = NULL;
SDL_Renderer *renderizador = NULL;
TTF_Font *fuente = NULL;
SDL_Texture *textura_tanque_amarillo = NULL;
SDL_Texture *textura_tanque_verde = NULL;

Mix_Chunk *sonido_disparo = NULL;
Mix_Chunk *sonido_muro = NULL;
Mix_Chunk *sonido_pared = NULL;
Mix_Chunk *sonido_tanque_hit = NULL;
Mix_Chunk *sonido_final = NULL; 

// Variables de Estado del Juego (Globales para que el main y la lógica las usen)
int tanque1_fila = 1, tanque1_col = 1, tanque1_direccion = 0, tanque1_vidas = 3;
int tanque2_fila = 13, tanque2_col = 18, tanque2_direccion = 0, tanque2_vidas = 3;

// ESTADO DE MOVIMIENTO CONTINUO
int tanque1_is_moving = 0; // 0=parado, 1=moviendo
int tanque1_last_direction = 0; // Dirección deseada por P1
int tanque2_is_moving = 0; 
int tanque2_last_direction = 0; 

int t1_ultimo_mov = 0; // Cooldown para P1
int t2_ultimo_mov = 0; 
int t1_ultimo_disparo = 0; 
int t2_ultimo_disparo = 0; 
bool game_over = false; 
int winner = 0; 

// ====================================================================
// PROTOTIPOS
// ====================================================================
void abrir_ventana(int ancho, int alto);
void cerrar_ventana();
void dibujar_rect(int x, int y, int w, int h, int r, int g, int b);
void dibujar_texto(const char *texto, int x, int y, int r, int g, int b);
void dibujar_muro_destruible(int x, int y);
void dibujar_muro_solido(int x, int y);
void dibujar_tanque_con_textura(SDL_Texture *textura, int x, int y, int direccion);
void dibujar_bala(float x_f, float y_f);
void dibujar_mapa(int **matriz, int filas, int columnas, int tanque1_dir, int tanque2_dir, Bala *balas, int num_balas, int t1_vidas, int t2_vidas, bool game_over, int winner);

void actualizar_balas(Bala *balas, int num_balas, int **mapa, int filas, int columnas, int *t1_fila, int *t1_col, int *t2_fila, int *t2_col, int *t1_vidas, int *t2_vidas, bool *game_over, int *winner);
void disparar_bala(Bala *balas, int num_balas, int fila_tanque, int col_tanque, int direccion, int propietario);
int obtener_indice_bala_libre(Bala *balas, int num_balas);
void reset_game(int **mapa, int filas, int columnas, Bala *balas, int num_balas);

// ====================================================================
// BLOQUE I: INTERFAZ GRÁFICA (IMPLEMENTACIONES)
// ====================================================================

// --- Funciones de Inicialización y Cierre de SDL ---
void abrir_ventana(int ancho, int alto) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) { 
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

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        fprintf(stderr, "ADVERTENCIA: SDL_mixer no pudo inicializarse. El juego continuará sin sonido: %s\n", Mix_GetError());
    } else {
        sonido_disparo = Mix_LoadWAV("disparo.wav"); 
        sonido_muro = Mix_LoadWAV("muro.wav");
        sonido_pared = Mix_LoadWAV("pared.wav"); 
        sonido_tanque_hit = Mix_LoadWAV("tanque.wav"); 
        sonido_final = Mix_LoadWAV("final.wav");
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

    fuente = TTF_OpenFont("arial.ttf", 16);
    if (fuente == NULL) {
        fprintf(stderr, "ADVERTENCIA: Error al cargar fuente (arial.ttf). Los contadores no se mostrarán: %s\n", TTF_GetError());
    }

    SDL_Surface *superficie_temp = NULL;

    superficie_temp = IMG_Load("tanque_amarillo.png");
    if (superficie_temp) {
        textura_tanque_amarillo = SDL_CreateTextureFromSurface(renderizador, superficie_temp);
        SDL_FreeSurface(superficie_temp);
    } 

    superficie_temp = IMG_Load("tanque_verde.png");
    if (superficie_temp) {
        textura_tanque_verde = SDL_CreateTextureFromSurface(renderizador, superficie_temp);
        SDL_FreeSurface(superficie_temp);
    }
}

void cerrar_ventana() {
    if (sonido_disparo) Mix_FreeChunk(sonido_disparo);
    if (sonido_muro) Mix_FreeChunk(sonido_muro);
    if (sonido_pared) Mix_FreeChunk(sonido_pared);
    if (sonido_tanque_hit) Mix_FreeChunk(sonido_tanque_hit);
    if (sonido_final) Mix_FreeChunk(sonido_final);

    Mix_CloseAudio(); 
    
    if (textura_tanque_amarillo) SDL_DestroyTexture(textura_tanque_amarillo);
    if (textura_tanque_verde) SDL_DestroyTexture(textura_tanque_verde);
    if (fuente) TTF_CloseFont(fuente);
    if (renderizador) SDL_DestroyRenderer(renderizador);
    if (ventana) SDL_DestroyWindow(ventana);
    
    IMG_Quit(); 
    TTF_Quit(); 
    SDL_Quit();
}

// --- Funciones Primitivas y Dibujo ---
void dibujar_rect(int x, int y, int w, int h, int r, int g, int b) {
    SDL_Rect rect = {x, y, w, h};
    SDL_SetRenderDrawColor(renderizador, r, g, b, 255);
    SDL_RenderFillRect(renderizador, &rect);
}

void dibujar_texto(const char *texto, int x, int y, int r, int g, int b) {
    if (!fuente) return;
    SDL_Color color = {r, g, b, 255};
    SDL_Surface *superficie_texto = TTF_RenderText_Solid(fuente, texto, color);
    if (superficie_texto) {
        SDL_Texture *textura_texto = SDL_CreateTextureFromSurface(renderizador, superficie_texto);
        SDL_Rect destino = {x, y, superficie_texto->w, superficie_texto->h};
        SDL_RenderCopy(renderizador, textura_texto, NULL, &destino);
        SDL_DestroyTexture(textura_texto);
        SDL_FreeSurface(superficie_texto);
    }
}

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
            }
        }
    }
}
void dibujar_muro_solido(int x, int y) { 
    int size = TILE_SIZE;
    int gris_base = 160;
    int gris_claro = 200;
    
    dibujar_rect(x, y, size, size, gris_base, gris_base, gris_base);
    
    int block_size = size / 2;
    
    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            int bx = x + j * block_size;
            int by = y + i * block_size;
            
            dibujar_rect(bx + 2, by + 2, block_size - 4, block_size - 4, 
                         gris_claro, gris_claro, gris_claro);
        }
    }
}

void dibujar_bala(float x_f, float y_f) {
    int tx = (int)(x_f * TILE_SIZE - (float)BALA_SIZE/2.0f); 
    int ty = (int)(y_f * TILE_SIZE - (float)BALA_SIZE/2.0f);
    
    dibujar_rect(tx, ty, BALA_SIZE, BALA_SIZE, 255, 255, 255);
    dibujar_rect(tx + 2, ty + 2, BALA_SIZE - 4, BALA_SIZE - 4, 255, 255, 0);
}

void dibujar_tanque_con_textura(SDL_Texture *textura, int x, int y, int direccion) {
    if (textura == NULL) {
        // Fallback si la textura no carga
        dibujar_rect(x, y, TILE_SIZE, TILE_SIZE, 128, 128, 128); // Gris oscuro
        return;
    }

    SDL_Rect destino = {x, y, TILE_SIZE, TILE_SIZE};
    
    double angulo = 0.0;
    if (direccion == 0) angulo = 0.0;
    else if (direccion == 1) angulo = 90.0;
    else if (direccion == 2) angulo = 180.0;
    else if (direccion == 3) angulo = 270.0;
    
    SDL_RenderCopyEx(renderizador, textura, NULL, &destino, angulo, NULL, SDL_FLIP_NONE);
}

// ====================================================================
// BLOQUE II: LÓGICA (Funciones internas del juego)
// ====================================================================

int obtener_indice_bala_libre(Bala *balas, int num_balas) {
    for (int i = 0; i < num_balas; i++) {
        if (!balas[i].activa) {
            return i;
        }
    }
    return -1;
}

void disparar_bala(Bala *balas, int num_balas, int fila_tanque, int col_tanque, int direccion, int propietario) {
    int i = obtener_indice_bala_libre(balas, num_balas);
    if (i != -1) {
        balas[i].activa = 1;
        balas[i].direccion = direccion;
        balas[i].propietario = propietario;

        // Ajuste inicial para que la bala salga del "cañón" del tanque
        balas[i].y_f = (float)fila_tanque + 0.5f;
        balas[i].x_f = (float)col_tanque + 0.5f;
        
        if (direccion == 0) balas[i].y_f -= 0.6f; // Arriba
        else if (direccion == 2) balas[i].y_f += 0.6f; // Abajo
        else if (direccion == 3) balas[i].x_f -= 0.6f; // Izquierda
        else if (direccion == 1) balas[i].x_f += 0.6f; // Derecha

        if (sonido_disparo) {
            Mix_PlayChannel(-1, sonido_disparo, 0); 
        }
    }
}

void actualizar_balas(Bala *balas, int num_balas, int **mapa, int filas, int columnas, int *t1_fila, int *t1_col, int *t2_fila, int *t2_col, int *t1_vidas, int *t2_vidas, bool *game_over, int *winner) {
    float velocidad = 0.25f;

    for (int i = 0; i < num_balas; i++) {
        if (balas[i].activa) {
            
            if (balas[i].direccion == 0) balas[i].y_f -= velocidad;
            else if (balas[i].direccion == 1) balas[i].x_f += velocidad;
            else if (balas[i].direccion == 2) balas[i].y_f += velocidad;
            else if (balas[i].direccion == 3) balas[i].x_f -= velocidad;

            int px = (int)(balas[i].x_f * TILE_SIZE - (float)BALA_SIZE/2.0f);
            int py = (int)(balas[i].y_f * TILE_SIZE - (float)BALA_SIZE/2.0f);
            
            SDL_Rect bala_rect = {px, py, BALA_SIZE, BALA_SIZE};

            int min_fila = (int)(balas[i].y_f - 1.0f);
            int max_fila = (int)(balas[i].y_f + 1.0f);
            int min_col = (int)(balas[i].x_f - 1.0f);
            int max_col = (int)(balas[i].x_f + 1.0f);

            if (min_fila < 0) min_fila = 0;
            if (max_fila >= filas) max_fila = filas - 1;
            if (min_col < 0) min_col = 0;
            if (max_col >= columnas) max_col = columnas - 1;

            // Colisión con el Mapa
            for (int r = min_fila; r <= max_fila; r++) {
                for (int c = min_col; c <= max_col; c++) {
                    int destino = mapa[r][c];

                    if (destino == 1 || destino == 2) { 
                        SDL_Rect muro_rect = {c * TILE_SIZE, r * TILE_SIZE, TILE_SIZE, TILE_SIZE};

                        if (SDL_HasIntersection(&bala_rect, &muro_rect)) 
                        {
                            balas[i].activa = 0;
                            
                            if (destino == 1) { // MURO DESTRUIBLE (PARED.WAV)
                                mapa[r][c] = 0; 
                                if (sonido_pared) Mix_PlayChannel(-1, sonido_pared, 0); 
                            } else { // MURO SÓLIDO (MURO.WAV)
                                if (sonido_muro) Mix_PlayChannel(-1, sonido_muro, 0); 
                            }
                            goto next_bullet; 
                        }
                    }
                }
            }
            
            // Colisión con Tanques
            if (balas[i].propietario != 1 && *t1_vidas > 0) {
                SDL_Rect t1_rect = {*t1_col * TILE_SIZE, *t1_fila * TILE_SIZE, TILE_SIZE, TILE_SIZE};
                
                if (SDL_HasIntersection(&bala_rect, &t1_rect)) 
                {
                    balas[i].activa = 0;
                    if (*t1_vidas > 0) {
                        (*t1_vidas)--;
                        if (*t1_vidas > 0) {
                            mapa[*t1_fila][*t1_col] = 0; *t1_fila = 1; *t1_col = 1; mapa[*t1_fila][*t1_col] = 3; 
                            if (sonido_tanque_hit) Mix_PlayChannel(-1, sonido_tanque_hit, 0); 
                        } else {
                            mapa[*t1_fila][*t1_col] = 0; *game_over = true; *winner = 2; 
                            if (sonido_final) Mix_PlayChannel(-1, sonido_final, 0); 
                        }
                    }
                    goto next_bullet;
                }
            }
            if (balas[i].propietario != 2 && *t2_vidas > 0) {
                 SDL_Rect t2_rect = {*t2_col * TILE_SIZE, *t2_fila * TILE_SIZE, TILE_SIZE, TILE_SIZE};

                if (SDL_HasIntersection(&bala_rect, &t2_rect))
                {
                    balas[i].activa = 0;
                    if (*t2_vidas > 0) {
                        (*t2_vidas)--;
                        if (*t2_vidas > 0) {
                            mapa[*t2_fila][*t2_col] = 0; *t2_fila = filas - 2; *t2_col = columnas - 2; mapa[*t2_fila][*t2_col] = 4;
                            if (sonido_tanque_hit) Mix_PlayChannel(-1, sonido_tanque_hit, 0); 
                        } else {
                            mapa[*t2_fila][*t2_col] = 0; *game_over = true; *winner = 1; 
                            if (sonido_final) Mix_PlayChannel(-1, sonido_final, 0); 
                        }
                    }
                    goto next_bullet;
                }
            }
            
            // 4. Chequear límites de pantalla
            if (px < 0 || px + BALA_SIZE > columnas * TILE_SIZE ||
                py < 0 || py + BALA_SIZE > filas * TILE_SIZE) {
                balas[i].activa = 0;
            }
            
            next_bullet:; 
        }
    }
}

void reset_game(int **mapa, int filas, int columnas, Bala *balas, int num_balas) {
    // 1. Resetear el mapa a la configuración inicial (solo muros, sin tanques)
    for (int i = 0; i < filas; i++) {
        for (int j = 0; j < columnas; j++) {
            // Limpiamos posiciones de tanques y suelo
            if (mapa[i][j] == 3 || mapa[i][j] == 4 || mapa[i][j] == 0) {
                mapa[i][j] = 0; 
            }
        }
    }
    
    // 2. Resetear variables de estado
    tanque1_fila = 1; tanque1_col = 1; tanque1_direccion = 0; tanque1_vidas = 3;
    tanque2_fila = filas - 2; tanque2_col = columnas - 2; tanque2_direccion = 0; tanque2_vidas = 3;
    
    tanque1_is_moving = 0;
    tanque1_last_direction = 0;
    tanque2_is_moving = 0;
    tanque2_last_direction = 0;

    t1_ultimo_mov = 0; 
    t2_ultimo_mov = 0;
    t1_ultimo_disparo = 0;
    t2_ultimo_disparo = 0;
    
    // 3. Colocar tanques en el mapa
    mapa[tanque1_fila][tanque1_col] = 3;
    mapa[tanque2_fila][tanque2_col] = 4;
    
    // 4. Desactivar todas las balas
    for (int i = 0; i < num_balas; i++) {
        balas[i].activa = 0;
    }
    
    // 5. Resetear estado del juego
    game_over = false;
    winner = 0;
    printf("--- JUEGO REINICIADO ---\n");
}


// FUNCIÓN PRINCIPAL DE DIBUJO (CON LAYOUT CORREGIDO)
void dibujar_mapa(int **matriz, int filas, int columnas, int tanque1_dir, int tanque2_dir, Bala *balas, int num_balas, int t1_vidas, int t2_vidas, bool game_over, int winner) {
    
    int ancho_total = columnas * TILE_SIZE; 
    
    SDL_SetRenderDrawColor(renderizador, 0, 0, 0, 255);
    SDL_RenderClear(renderizador);
    
    // Dibujar el mapa (muros y tanques en su posición de baldosa)
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
            else if (que_hay == 3 && t1_vidas > 0) { 
                dibujar_tanque_con_textura(textura_tanque_amarillo, x, y, tanque1_dir);
            }
            else if (que_hay == 4 && t2_vidas > 0) { 
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
    
    // 2. Dibujar HUD y Leyenda (DISEÑO MEJORADO)
    int y_leyenda_base = filas * TILE_SIZE;
    
    // Fondo de la leyenda
    SDL_SetRenderDrawColor(renderizador, 30, 30, 30, 255);
    SDL_Rect fondo_leyenda = {0, y_leyenda_base, ancho_total, ALTURA_LEYENDA};
    SDL_RenderFillRect(renderizador, &fondo_leyenda);
    
    // POSICIONES CLAVE (Ajustadas para la mejor legibilidad en un ancho estándar)
    int x_p1_start = 20; 
    int x_p2_start = ancho_total - 380; 
    
    // --- FILA 1: INFORMACIÓN DE RECURSOS ---
    int y_fila1 = y_leyenda_base + 5;
    int y_salto_muros = y_fila1 + 18; 
    
    // Muro Destruible
    dibujar_rect(x_p1_start, y_fila1, 20, 20, 216, 132, 88); 
    dibujar_texto("Pared", x_p1_start + 25, y_fila1 + 2, 255, 255, 255);

    // Muro Sólido
    dibujar_rect(x_p1_start + 150, y_fila1, 20, 20, 160, 160, 160); 
    dibujar_texto("Muro", x_p1_start + 175, y_fila1 + 2, 255, 255, 255);
    
    // Bala
    dibujar_rect(x_p1_start + 290, y_fila1 + 5, 10, 10, 255, 255, 255); 
    dibujar_texto("Bala", x_p1_start + 305, y_fila1 + 2, 255, 255, 255);
    
    // --- FILA 2: CONTADORES DE VIDA ---
    int y_fila2 = y_leyenda_base + 35;
    
    // P1 Vidas (Amarillo)
    char buf1[50];
    sprintf(buf1, "P1 Vidas: %d", t1_vidas);
    dibujar_rect(x_p1_start, y_fila2, 20, 20, 255, 200, 0); 
    dibujar_texto(buf1, x_p1_start + 25, y_fila2 + 2, 255, 255, 100); 

    // P2 Vidas (Verde)
    char buf2[50];
    sprintf(buf2, "P2 Vidas: %d", t2_vidas);
    dibujar_rect(x_p2_start, y_fila2, 20, 20, 0, 150, 0); 
    dibujar_texto(buf2, x_p2_start + 25, y_fila2 + 2, 100, 255, 100); 
    
    // --- FILA 3: CONTROLES ---
    int y_fila3 = y_leyenda_base + 65; 
    int y_salto_controles = y_fila3 + 18;
    
    // Controles P1 (Amarillo)
    dibujar_texto("P1 Mov: FLECHAS (Continuo)", x_p1_start, y_fila3, 255, 255, 100);
    dibujar_texto("Disparo: ESPACIO", x_p1_start, y_salto_controles, 255, 255, 100);

    // Controles P2 (Verde)
    dibujar_texto("P2 Mov: W A S D (Continuo)", x_p2_start, y_fila3, 100, 255, 100);
    dibujar_texto("Disparo: R-SHIFT", x_p2_start, y_salto_controles, 100, 255, 100);

    // LÓGICA DE FIN DE JUEGO (Menú de Reinicio/Salida)
    if (game_over) {
        int centro_x = ancho_total / 2;
        int centro_y = filas * TILE_SIZE / 2;
        
        const char *msg;
        int r, g, b;

        if (winner == 1) { 
            msg = "JUGADOR 1 (AMARILLO) GANA!";
            r = 255; g = 255; b = 100; 
        } else { 
            msg = "JUGADOR 2 (VERDE) GANA!";
            r = 100; g = 255; b = 100; 
        }

        SDL_SetRenderDrawBlendMode(renderizador, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderizador, 0, 0, 0, 150);
        SDL_Rect pantalla_completa = {0, 0, ancho_total, filas * TILE_SIZE};
        SDL_RenderFillRect(renderizador, &pantalla_completa);
        SDL_SetRenderDrawBlendMode(renderizador, SDL_BLENDMODE_NONE);
        
        dibujar_texto(msg, centro_x - 150, centro_y - 30, r, g, b);
        dibujar_texto("Presiona ESPACIO para REINICIAR", centro_x - 150, centro_y + 10, 255, 255, 255);
        dibujar_texto("Presiona ESC para SALIR", centro_x - 150, centro_y + 40, 255, 255, 255);
    }
    
    SDL_RenderPresent(renderizador);
}
