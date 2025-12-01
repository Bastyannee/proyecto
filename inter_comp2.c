#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h> 
#include <SDL2/SDL_ttf.h> 
#include <SDL2/SDL_mixer.h> 
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h> 
#include <math.h>

// Constantes globales
#define TILE_SIZE 40
#define ALTURA_LEYENDA 120
#define BALA_SIZE 8 

SDL_Window *ventana = NULL;
SDL_Renderer *renderizador = NULL;
TTF_Font *fuente = NULL;
TTF_Font *fuente_mediana = NULL;
TTF_Font *fuente_grande = NULL;
SDL_Texture *textura_tanque_amarillo = NULL;
SDL_Texture *textura_tanque_verde = NULL;

Mix_Chunk *sonido_disparo = NULL;
Mix_Chunk *sonido_muro = NULL;
Mix_Chunk *sonido_pared = NULL;
Mix_Chunk *sonido_tanque_hit = NULL;
Mix_Chunk *sonido_final = NULL; 

// Funciones para crear ventana
void dibujar_rect(int x, int y, int w, int h, int r, int g, int b) {
    SDL_Rect rect = {x, y, w, h};
    SDL_SetRenderDrawColor(renderizador, r, g, b, 255);
    SDL_RenderFillRect(renderizador, &rect);
}

void dibujar_texto(const char *texto, int x, int y, int r, int g, int b) {
    if (!fuente) return;
    SDL_Color color = {r, g, b, 255};

    SDL_Surface *superficie_texto = TTF_RenderText_Blended(fuente, texto, color);
    
    if (superficie_texto) {
        SDL_Texture *textura_texto = SDL_CreateTextureFromSurface(renderizador, superficie_texto);
        SDL_Rect destino = {x, y, superficie_texto->w, superficie_texto->h};
        SDL_RenderCopy(renderizador, textura_texto, NULL, &destino);
        SDL_DestroyTexture(textura_texto);
        SDL_FreeSurface(superficie_texto);
    }
}

// Inicializa SDL con mensajes de depuración
void abrir_ventana(int ancho, int alto) {
    printf("[DEBUG] Iniciando SDL...\n");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) { 
        printf("SDL Error: %s\n", SDL_GetError()); exit(1);
    }
    
    printf("[DEBUG] Iniciando TTF/IMG/Mixer...\n");
    TTF_Init();
    IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
    
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        printf("Error Audio: %s (El juego continuará sin sonido)\n", Mix_GetError());
    }

    printf("[DEBUG] Creando Ventana...\n");
    ventana = SDL_CreateWindow("BATTLE CITY", 
                               SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                               ancho, alto, 
                               SDL_WINDOW_FULLSCREEN_DESKTOP);
    if (!ventana) { printf("Error Ventana: %s\n", SDL_GetError()); exit(1); }

    renderizador = SDL_CreateRenderer(ventana, -1, SDL_RENDERER_ACCELERATED);
    SDL_RenderSetLogicalSize(renderizador, ancho, alto);
    
    printf("[DEBUG] Cargando Fuente...\n"); // Carga la fuente 
    fuente = TTF_OpenFont("arial.ttf", 6); // En tamaño 6
    
    fuente_grande = TTF_OpenFont("arial.ttf", 18);  // Fuente titulo
    if(!fuente_grande) fuente_grande = fuente; 
    
    fuente_mediana = TTF_OpenFont("arial.ttf", 12); // Fuente subtitulo
    if(!fuente_mediana) fuente_mediana = fuente;
    if (fuente == NULL) {
        // Si falló, intentamos con mayúscula (común al copiar desde Windows/Mac)
        fuente = TTF_OpenFont("Arial.ttf", 16);
    }

    if (fuente == NULL) {
        fprintf(stderr, "ADVERTENCIA: No se encontró fuente (arial.ttf ni Arial.ttf). Texto no visible: %s\n", TTF_GetError());
    }

    // Cargar imágenes 
    printf("[DEBUG] Cargando Imágenes...\n");
    SDL_Surface *s = IMG_Load("tanque_amarillo.png");
    if (s) { textura_tanque_amarillo = SDL_CreateTextureFromSurface(renderizador, s); SDL_FreeSurface(s); }
    else printf("Error tanque amarillo: %s\n", IMG_GetError());
    
    s = IMG_Load("tanque_verde.png");
    if (s) { textura_tanque_verde = SDL_CreateTextureFromSurface(renderizador, s); SDL_FreeSurface(s); }
    else printf("Error tanque verde: %s\n", IMG_GetError());

    // Cargar sonidos
    printf("[DEBUG] Cargando Sonidos...\n");
    sonido_disparo = Mix_LoadWAV("disparo.wav"); 
    sonido_muro = Mix_LoadWAV("muro.wav");
    sonido_pared = Mix_LoadWAV("pared.wav"); 
    sonido_tanque_hit = Mix_LoadWAV("tanque.wav"); 
    sonido_final = Mix_LoadWAV("final.wav");
    
    printf("[DEBUG] Inicialización Completa.\n");
}

// Dibujo de elementos
void dibujar_muro_destruible(int x, int y) {
    dibujar_rect(x, y, TILE_SIZE, TILE_SIZE, 184, 100, 56); 
    dibujar_rect(x+2, y+2, TILE_SIZE/2-4, TILE_SIZE/2-4, 216, 132, 88); 
    dibujar_rect(x+TILE_SIZE/2+2, y+TILE_SIZE/2+2, TILE_SIZE/2-4, TILE_SIZE/2-4, 216, 132, 88);
}
void dibujar_muro_solido(int x, int y) { 
    dibujar_rect(x, y, TILE_SIZE, TILE_SIZE, 160, 160, 160); 
    dibujar_rect(x+5, y+5, TILE_SIZE-10, TILE_SIZE-10, 200, 200, 200); 
}
void dibujar_bala(int x, int y) {
    int centro_x = x + TILE_SIZE/2 - BALA_SIZE/2;
    int centro_y = y + TILE_SIZE/2 - BALA_SIZE/2;
    dibujar_rect(centro_x, centro_y, BALA_SIZE, BALA_SIZE, 255, 255, 255);
}
void dibujar_tanque_con_textura(SDL_Texture *textura, int x, int y, int dir) {
    if (textura) {
        SDL_Rect destino = {x, y, TILE_SIZE, TILE_SIZE};
        double angulo = 0.0;
        // Asumiendo enum {UP=0, DOWN=1, LEFT=2, RIGHT=3}
        if(dir == 1) angulo = 180.0; 
        else if(dir == 2) angulo = 270.0;
        else if(dir == 3) angulo = 90.0;
        
        SDL_RenderCopyEx(renderizador, textura, NULL, &destino, angulo, NULL, SDL_FLIP_NONE);
    } else {
        dibujar_rect(x, y, TILE_SIZE, TILE_SIZE, 0, 255, 0); 
    }
}
void dibujar_texto_centrado(TTF_Font *font, const char *texto, int centro_x, int y, int r, int g, int b) {
    if (!font) return;
    SDL_Color color = {r, g, b, 255};
    SDL_Surface *surf = TTF_RenderText_Blended(font, texto, color);
    if (surf) {
        SDL_Texture *tex = SDL_CreateTextureFromSurface(renderizador, surf);
        // Calculamos la X para que quede centrado: Centro - (Ancho / 2)
        SDL_Rect dest = {centro_x - (surf->w / 2), y, surf->w, surf->h};
        SDL_RenderCopy(renderizador, tex, NULL, &dest);
        SDL_DestroyTexture(tex);
        SDL_FreeSurface(surf);
    }
}
// Función principal de enlace
void visualizar_laberinto(int **matriz, int filas, int columnas, int v1, int v2, int dir1, int dir2, int winner) {
    
    // Inicialización segura
    if (ventana == NULL) {
        abrir_ventana(columnas * TILE_SIZE, filas * TILE_SIZE + ALTURA_LEYENDA);
    }
    
    int ancho_total = columnas * TILE_SIZE; 

    // Limpiar Pantalla
    SDL_SetRenderDrawColor(renderizador, 0, 0, 0, 255);
    SDL_RenderClear(renderizador);

    // Dibujar Mapa
    for (int i = 0; i < filas; i++) {
        for (int j = 0; j < columnas; j++) {
            int x = j * TILE_SIZE;
            int y = i * TILE_SIZE;
            int valor = matriz[i][j];

            switch(valor) {
                case 1: dibujar_muro_destruible(x, y); break;
                case 2: dibujar_muro_solido(x, y); break;
                case 3: dibujar_tanque_con_textura(textura_tanque_amarillo, x, y, dir1); break; 
                case 4: dibujar_tanque_con_textura(textura_tanque_verde, x, y, dir2); break;    
                case 5: dibujar_bala(x, y); break;
                case 6: // DIBUJAR ESCUDO
                    dibujar_rect(x + 10, y + 10, TILE_SIZE - 20, TILE_SIZE - 20, 0, 255, 255); 
                    dibujar_rect(x + 12, y + 12, TILE_SIZE - 24, TILE_SIZE - 24, 0, 0, 0);
                    break;
            
        }
    }
    
    // Dibujar HUD
    int y_hud = filas * TILE_SIZE;
    
    // Fondo gris oscuro
    SDL_SetRenderDrawColor(renderizador, 20, 20, 20, 255);
    SDL_Rect fondo_hud = {0, y_hud, ancho_total, ALTURA_LEYENDA};
    SDL_RenderFillRect(renderizador, &fondo_hud);
    
    // Configuración de filas (espaciado vertical) 
    int margen_sup = 15;
    int y_fila1 = y_hud + margen_sup;
    int y_fila2 = y_hud + margen_sup + 35;
    int y_fila3 = y_hud + margen_sup + 70;

    // Columnas (solo izquierda y derecha)
    int x_col1 = 30;                 // Izquierda (Leyenda)
    int x_col3 = ancho_total - 140;  // Derecha (Tanques)
    
    // Tamaño para los iconos de la leyenda (20x20)
    int mini_size = 20;

    // Fila 1: ladrillo (Izq) | TANQUE P1 (Der)
    // Icono ladrillo (miniatura)
    dibujar_rect(x_col1, y_fila1, mini_size, mini_size, 184, 100, 56);
    dibujar_rect(x_col1+1, y_fila1+1, mini_size/2-2, mini_size/2-2, 216, 132, 88);
    dibujar_rect(x_col1+mini_size/2+1, y_fila1+mini_size/2+1, mini_size/2-2, mini_size/2-2, 216, 132, 88);
    
    dibujar_texto("Muro", x_col1 + 30, y_fila1 + 4, 180, 180, 180);

    // Tanque P1 + Vidas
    SDL_Rect rect_p1 = {x_col3, y_fila1 - 5, 30, 30}; 
    SDL_RenderCopyEx(renderizador, textura_tanque_amarillo, NULL, &rect_p1, 0, NULL, SDL_FLIP_NONE);
    
    char txt_p1[20]; sprintf(txt_p1, "x %d", v1);
    dibujar_texto(txt_p1, x_col3 + 40, y_fila1 + 4, 255, 255, 0);
        
    // FILA 2: ACERO (Izq) | TANQUE P2 (Der)
    // Icono Acero (Miniatura)
    dibujar_rect(x_col1, y_fila2, mini_size, mini_size, 160, 160, 160);
    dibujar_rect(x_col1+4, y_fila2+4, mini_size-8, mini_size-8, 200, 200, 200); 
    dibujar_texto("Acero", x_col1 + 30, y_fila2 + 4, 180, 180, 180);

    // Tanque P2 + Vidas
    SDL_Rect rect_p2 = {x_col3, y_fila2 - 5, 30, 30}; 
    SDL_RenderCopyEx(renderizador, textura_tanque_verde, NULL, &rect_p2, 0, NULL, SDL_FLIP_NONE);
    
    char txt_p2[20]; sprintf(txt_p2, "x %d", v2);
    dibujar_texto(txt_p2, x_col3 + 40, y_fila2 + 4, 50, 255, 50);
        
    // FILA 3: ESCUDO/BASE (Izq)

    // Icono base (Miniatura)
    dibujar_rect(x_col1, y_fila3, mini_size, mini_size, 0, 255, 255); 
    dibujar_rect(x_col1+3, y_fila3+3, mini_size-6, mini_size-6, 0, 0, 0); 
    
    dibujar_texto("Escudo", x_col1 + 30, y_fila3 + 4, 0, 255, 255);
    
    dibujar_texto("[Q] Salir", (ancho_total/2)-20, y_fila3 + 4, 80, 80, 80);

    // GAME OVER OVERLAY
    if (winner != 0) {
        // Fondo oscuro
        SDL_SetRenderDrawBlendMode(renderizador, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderizador, 0, 0, 0, 230);
        SDL_Rect full = {0, 0, ancho_total, filas * TILE_SIZE + ALTURA_LEYENDA};
        SDL_RenderFillRect(renderizador, &full);
        SDL_SetRenderDrawBlendMode(renderizador, SDL_BLENDMODE_NONE);

        int cx = ancho_total / 2;     
        int cy = (filas * TILE_SIZE) / 2; 

        // Titulo (Más arriba)
        dibujar_texto_centrado(fuente_grande, "FIN DEL JUEGO", cx, cy - 80, 255, 0, 0);

        // Ganador
        char vic_msg[50];
        if (winner == 1) sprintf(vic_msg, "GANADOR: JUGADOR 1");
        else sprintf(vic_msg, "GANADOR: JUGADOR 2");
        dibujar_texto_centrado(fuente_mediana, vic_msg, cx, cy - 40, 255, 255, 255);

        // Estadisticas (apiladas verticalmente para que quepan)
        char p1_msg[50];
        char p2_msg[50];
        sprintf(p1_msg, "P1 (Amarillo): %d Vidas", v1);
        sprintf(p2_msg, "P2 (Verde):    %d Vidas", v2);

        // P1
        dibujar_texto_centrado(fuente, p1_msg, cx, cy + 10, 255, 255, 0); // Amarillo
        // P2 (Debajo de P1)
        dibujar_texto_centrado(fuente, p2_msg, cx, cy + 30, 0, 255, 0);   // Verde

        // Instrucciones (Textos más cortos y separados)
        dibujar_texto_centrado(fuente, "[ ESPACIO ]: Reiniciar", cx, cy + 70, 200, 200, 200);
        dibujar_texto_centrado(fuente, "[ Q ]: Salir del Juego", cx, cy + 90, 200, 200, 200);
    }

    SDL_RenderPresent(renderizador);
}

// Función auxiliar para reproducir audio desde la lógica
void reproducir_efecto(int id) {
    switch(id) {
        case 1: if(sonido_disparo) Mix_PlayChannel(-1, sonido_disparo, 0); break;
        case 2: if(sonido_pared) Mix_PlayChannel(-1, sonido_pared, 0); break;
        case 3: if(sonido_muro) Mix_PlayChannel(-1, sonido_muro, 0); break;
        case 4: if(sonido_tanque_hit) Mix_PlayChannel(-1, sonido_tanque_hit, 0); break;
        case 5: if(sonido_final) Mix_PlayChannel(-1, sonido_final, 0); break;
    }
}
