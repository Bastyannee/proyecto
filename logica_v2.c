#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <SDL2/SDL.h>

// ==================== DEFINICIONES Y PROTOTIPOS ====================

// Declaración de la función externa en inter_comp2.c
void visualizar_laberinto(int **matriz, int filas, int columnas, 
                          int v1, int v2, int dir1, int dir2, int winner);

// Función auxiliar de audio
void reproducir_efecto(int id);

// Constantes
#define SHIELD_TILE 6 

// Variables Globales
int HEIGHT, WIDTH;
int** test_input_map = NULL; // Mapa original cargado del archivo
int** terrain_map = NULL;    // Mapa dinámico del juego
int winner_status = 0;       // 0: Jugando, 1: Gana P1, 2: Gana P2

typedef enum {UP, DOWN, LEFT, RIGHT} direction;

typedef struct { int x; int y; direction dir; int lives; bool has_shield; } tank;
typedef struct { int x; int y; direction dir; bool active; } bullet;

tank player1, player2;
bullet bullet_p1[5], bullet_p2[5];

// ==================== CARGA DE MAPA ====================
int** cargar_mapa(const char* archivo, int* filas, int* columnas) {
    FILE* f = fopen(archivo, "r");
    if (!f) { printf("ERROR: No se pudo abrir %s\n", archivo); *filas = *columnas = 0; return NULL; }
    fscanf(f, "%d %d", filas, columnas);
    
    int** mapa = (int**)malloc(*filas * sizeof(int*));
    for (int i = 0; i < *filas; i++) {
        mapa[i] = (int*)malloc(*columnas * sizeof(int));
        for (int j = 0; j < *columnas; j++) {
            fscanf(f, "%d", &mapa[i][j]);
        }
    }
    fclose(f);
    return mapa;
}

// ==================== LÓGICA DEL JUEGO ====================

void initialize_game() {
    // 1. Limpieza de memoria antigua (si reiniciamos)
    if (terrain_map != NULL) {
        for(int i=0; i<HEIGHT; i++) free(terrain_map[i]);
        free(terrain_map);
    }

    terrain_map = (int**)malloc(HEIGHT * sizeof(int*));
    for (int i = 0; i < HEIGHT; i++) terrain_map[i] = (int*)malloc(WIDTH * sizeof(int));
    
    // 2. Resetear Stats
    player1.lives = 3; player1.has_shield = false;
    player2.lives = 3; player2.has_shield = false;
    winner_status = 0;

    // 3. Cargar el mapa base del archivo
    for (int i = 0; i < HEIGHT; i++) for (int j = 0; j < WIDTH; j++) {
        int v = test_input_map[i][j];
        
        // Copiar terreno básico
        if (v == 0 || v == 1 || v == 2 || v == SHIELD_TILE) {
            terrain_map[i][j] = v;
        }
        else if (v == 3) { // Jugador 1
            player1.x = j; player1.y = i; player1.dir = UP; 
            terrain_map[i][j] = 0; 
        }
        else if (v == 4) { // Jugador 2
            player2.x = j; player2.y = i; player2.dir = UP; 
            terrain_map[i][j] = 0; 
        }
        else {
            terrain_map[i][j] = 0;
        }
    }

    // =========================================================
    // GENERADOR DE ESCUDOS ALEATORIOS
    // =========================================================
    // Intentamos 100 veces buscar un lugar vacío para poner un escudo
    int intentos = 0;
    int escudos_a_generar = 2;

    while (escudos_a_generar > 0 && intentos < 100) {
        int ry = rand() % HEIGHT;
        int rx = rand() % WIDTH;

        // Verificamos que sea "aire" (0) y que NO esté un jugador ahí
        bool choca_p1 = (rx == player1.x && ry == player1.y);
        bool choca_p2 = (rx == player2.x && ry == player2.y);

        if (terrain_map[ry][rx] == 0 && !choca_p1 && !choca_p2) {
            terrain_map[ry][rx] = SHIELD_TILE; // Ponemos el ID 6
            escudos_a_generar--;
        }
        intentos++;
    }

    // Resetear Balas
    for (int i = 0; i < 5; i++) {
        bullet_p1[i].active = false;
        bullet_p2[i].active = false;
    }
}

void move_tank(tank *t, direction dir) {
    t->dir = dir;
    int nx = t->x, ny = t->y;
    
    if (dir == UP) ny--; 
    else if (dir == DOWN) ny++; 
    else if (dir == LEFT) nx--; 
    else if (dir == RIGHT) nx++;
    
    // Colisiones con bordes
    if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) return;
    
    // Colisiones con Muros (1: Ladrillo, 2: Acero)
    if (terrain_map[ny][nx] == 1 || terrain_map[ny][nx] == 2) return;
    
    // Colisión entre tanques
    tank *ot = (t == &player1) ? &player2 : &player1;
    if (nx == ot->x && ny == ot->y) return;

    // Recoger Powerup (Escudo/Base) - ID 6
    if (terrain_map[ny][nx] == SHIELD_TILE) { 
        t->has_shield = true; 
        terrain_map[ny][nx] = 0; // Desaparece al cogerlo
        reproducir_efecto(3); // Sonido confirmación (usando el de metal por ahora)
    }

    t->x = nx; t->y = ny;
}

void fire_bullet(tank *t, bullet *arr, int max) {
    if (t->lives <= 0) return;

    for (int i = 0; i < max; i++) if (!arr[i].active) {
        int bx = t->x;
        int by = t->y;
        
        if (t->dir == UP) by--; 
        else if (t->dir == DOWN) by++;
        else if (t->dir == LEFT) bx--; 
        else if (t->dir == RIGHT) bx++;
        
        // Validaciones inmediatas
        if (bx < 0 || bx >= WIDTH || by < 0 || by >= HEIGHT) {
            reproducir_efecto(3); return;
        }
        if (terrain_map[by][bx] == 2) { // Metal
            reproducir_efecto(3); return;
        }
        if (terrain_map[by][bx] == 1) { // Ladrillo
            terrain_map[by][bx] = 0; 
            reproducir_efecto(2); return;
        }
        
        // Fuego amigo / Enemigo cercano
        tank *enemy = (t == &player1) ? &player2 : &player1;
        if (bx == enemy->x && by == enemy->y && enemy->lives > 0) {
            if (!enemy->has_shield) {
                enemy->lives--;
                reproducir_efecto(4);
            } else {
                enemy->has_shield = false;
                reproducir_efecto(3);
            }
            return;
        }

        // Crear bala
        arr[i].active = true; 
        arr[i].dir = t->dir;
        arr[i].x = bx; 
        arr[i].y = by;
        reproducir_efecto(1);
        return;
    }
}

void update_bullets() { 
    for(int p=0; p<2; p++) {
        bullet* arr = (p==0) ? bullet_p1 : bullet_p2;
        tank* en = (p==0) ? &player2 : &player1;
        
        for (int i = 0; i < 5; i++) if (arr[i].active) {
            int nx = arr[i].x, ny = arr[i].y;
            
            if (arr[i].dir == UP) ny--; 
            else if (arr[i].dir == DOWN) ny++;
            else if (arr[i].dir == LEFT) nx--; 
            else if (arr[i].dir == RIGHT) nx++;
            
            // 1. Choque Muros/Limites
            if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT || terrain_map[ny][nx] == 2) { 
                arr[i].active = false; 
                if (nx >= 0 && nx < WIDTH && ny >= 0 && ny < HEIGHT && terrain_map[ny][nx] == 2) 
                    reproducir_efecto(3);
                continue; 
            }

            // 2. Romper Ladrillo
            if (terrain_map[ny][nx] == 1) { 
                terrain_map[ny][nx] = 0; 
                arr[i].active = false; 
                reproducir_efecto(2); 
                continue; 
            }

            // 3. Impacto Tanque
            if (nx == en->x && ny == en->y && en->lives > 0) {
                arr[i].active = false;
                if(!en->has_shield) {
                    en->lives--; 
                    if (en->lives > 0) reproducir_efecto(4);
                    else reproducir_efecto(5); // Game Over sound
                } else {
                    en->has_shield = false;
                    reproducir_efecto(3);
                }
                continue;
            }
            arr[i].x = nx; arr[i].y = ny;
        }
    }
}

// Prepara la matriz para enviar a visualizar (para inter_comp2.c)
void generate_and_print_output_map() {
    int **display_map = (int**)malloc(HEIGHT * sizeof(int*));
    for(int i=0; i<HEIGHT; i++) display_map[i] = (int*)malloc(WIDTH * sizeof(int));
    
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            int v = terrain_map[i][j];
            
            // Superponer Jugadores
            if (player1.lives > 0 && player1.y==i && player1.x==j) v = 3;
            else if (player2.lives > 0 && player2.y==i && player2.x==j) v = 4;
            else {
                // Superponer Balas
                for (int b=0; b<5; b++) {
                    if ((bullet_p1[b].active && bullet_p1[b].y==i && bullet_p1[b].x==j) ||
                        (bullet_p2[b].active && bullet_p2[b].y==i && bullet_p2[b].x==j)) { 
                        v = 5; break; 
                    }
                }
            }
            display_map[i][j] = v;
        }
    }
    
    // Determinar ganador
    winner_status = 0;
    if (player1.lives <= 0) winner_status = 2; // Gana P2
    if (player2.lives <= 0) winner_status = 1; // Gana P1

    visualizar_laberinto(display_map, HEIGHT, WIDTH, 
                         player1.lives, player2.lives, 
                         player1.dir, player2.dir, winner_status);
                         
    for(int i=0; i<HEIGHT; i++) free(display_map[i]);
    free(display_map);
}

// ==================== INPUT SDL (CONTROLES) ====================
bool input_process_sdl() {
    const Uint8 *state = SDL_GetKeyboardState(NULL);
    SDL_PumpEvents(); // Actualizar estado del teclado

    if (state[SDL_SCANCODE_Q]) return false; // Salir del juego

    if (winner_status != 0) {
        // Si hay un ganador, SOLO escuchamos ESPACIO para reiniciar
        if (state[SDL_SCANCODE_SPACE]) {
            initialize_game(); // Reiniciar variables
            SDL_Delay(200);    // Pequeña pausa para no detectar doble pulsación
        }
        return true; // Seguimos corriendo el loop, pero sin mover tanques
    }

    // --- CONTROLES DE JUEGO ---
    static int cooldown = 0;
    if (cooldown > 0) { cooldown--; return true; }

    // JUGADOR 1 (WASD + F)
    if (player1.lives > 0) {
        if (state[SDL_SCANCODE_W]) { move_tank(&player1, UP); cooldown = 5; }
        else if (state[SDL_SCANCODE_S]) { move_tank(&player1, DOWN); cooldown = 5; }
        else if (state[SDL_SCANCODE_A]) { move_tank(&player1, LEFT); cooldown = 5; }
        else if (state[SDL_SCANCODE_D]) { move_tank(&player1, RIGHT); cooldown = 5; }
        else if (state[SDL_SCANCODE_F]) { fire_bullet(&player1, bullet_p1, 5); cooldown = 10; }
    }
    
    // JUGADOR 2 (FLECHAS + INTRO NUMPAD)
    if (player2.lives > 0) {
        if (state[SDL_SCANCODE_UP])         { move_tank(&player2, UP); cooldown = 5; }
        else if (state[SDL_SCANCODE_DOWN])  { move_tank(&player2, DOWN); cooldown = 5; }
        else if (state[SDL_SCANCODE_LEFT])  { move_tank(&player2, LEFT); cooldown = 5; }
        else if (state[SDL_SCANCODE_RIGHT]) { move_tank(&player2, RIGHT); cooldown = 5; }
        // Disparo: Enter del Numpad (Derecha) O Enter normal
        else if (state[SDL_SCANCODE_KP_ENTER] || state[SDL_SCANCODE_RETURN]) { 
            fire_bullet(&player2, bullet_p2, 5); cooldown = 10; 
        }
    }

    return true;
}

// ==================== MAIN ====================
int main(int argc, char* argv[]) {
    srand(time(NULL));
    
    // 1. Cargar mapa base
    test_input_map = cargar_mapa("mapa8x8.txt", &HEIGHT, &WIDTH);
    if (!test_input_map) {
        printf("ERROR CRITICO: No se encontro mapa8x8.txt\n");
        return 1;
    }

    // 2. Iniciar Juego
    initialize_game();

    // 3. Loop Principal
    bool corriendo = true;
    while(corriendo) {
        // A. Dibujar
        generate_and_print_output_map(); 
        
        // B. Input (Controles o Reinicio)
        corriendo = input_process_sdl();
        
        // C. Actualizar Lógica (Solo si no terminó el juego)
        if (winner_status == 0) {
            update_bullets();
        }
        
        SDL_Delay(30); // ~30 FPS
    }
    
    // 4. Limpieza
    if (terrain_map) { for(int i=0; i<HEIGHT; i++) free(terrain_map[i]); free(terrain_map); }
    if (test_input_map) { for(int i=0; i<HEIGHT; i++) free(test_input_map[i]); free(test_input_map); }
    
    return 0;
}
