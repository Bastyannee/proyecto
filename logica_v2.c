#include <stdio.h>
#include <termios.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>//para "randomizar" la aparicion del escudo
// ==================== CARGAR MAPA ====================
int** cargar_mapa(const char* archivo, int* filas, int* columnas) {
    FILE* f = fopen(archivo, "r");
    if (!f) {
        printf("ERROR: No se pudo abrir %s\n", archivo);
        *filas = *columnas = 0;
        return NULL;
    }
    fscanf(f, "%d %d", filas, columnas);
    int** mapa = (int**)malloc(*filas * sizeof(int*));
    for (int i = 0; i < *filas; i++) {
        mapa[i] = (int*)malloc(*columnas * sizeof(int));
        for (int j = 0; j < *columnas; j++) fscanf(f, "%d", &mapa[i][j]);
    }
    fclose(f);
    return mapa;
}

// ==================== VARIABLES GLOBALES ====================
int HEIGHT, WIDTH;
int** test_input_map = NULL;
int** terrain_map = NULL;

typedef enum {UP, DOWN, LEFT, RIGHT} direction;

// MODIFICACIÓN 1: Agregamos vidas y escudo al struct del tanque
typedef struct { 
    int x; 
    int y; 
    direction dir; 
    int lives;       // Contador de vidas
    bool has_shield; // Estado del escudo
} tank;

typedef struct { int x; int y; direction dir; bool active; } bullet;

tank player1, player2;
bullet bullet_p1[5];
bullet bullet_p2[5];

#define BULLET_TILE 5
#define SHIELD_TILE 6 // Definimos el valor del escudo en el mapa

// ==================== GUARDAR PARTIDA ====================
void guardar_partida(const char* nombre) {
    FILE* f = fopen(nombre, "w");
    if (!f) { printf("ERROR al guardar\n"); return; }

    fprintf(f, "%d %d\n", HEIGHT, WIDTH);
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) fprintf(f, "%d ", terrain_map[i][j]);
        fprintf(f, "\n");
    }
    // MODIFICACIÓN: Guardamos también las vidas y el escudo
    fprintf(f, "#P1 %d %d %d %d %d\n", player1.x, player1.y, player1.dir, player1.lives, player1.has_shield);
    fprintf(f, "#P2 %d %d %d %d %d\n", player2.x, player2.y, player2.dir, player2.lives, player2.has_shield);
    
    fprintf(f, "#B1\n");
    for (int i = 0; i < 5; i++) if (bullet_p1[i].active)
        fprintf(f, "%d %d %d\n", bullet_p1[i].x, bullet_p1[i].y, bullet_p1[i].dir);
    fprintf(f, "#E1\n");
    fclose(f);
    printf("¡PARTIDA GUARDADA en %s!\n", nombre);
}

// ==================== INICIALIZAR ====================
void initialize_game() {
    terrain_map = (int**)malloc(HEIGHT * sizeof(int*));
    for (int i = 0; i < HEIGHT; i++) terrain_map[i] = (int*)malloc(WIDTH * sizeof(int));

    // Inicializar stats de jugadores
    player1.lives = 3; 
    player1.has_shield = false;
    
    player2.lives = 3;
    player2.has_shield = false;

    for (int i = 0; i < HEIGHT; i++) for (int j = 0; j < WIDTH; j++) {
        int v = test_input_map[i][j];
        // MODIFICACIÓN: Aceptamos el 6 (Escudo) como parte del terreno
        if (v == 0 || v == 1 || v == 2 || v == SHIELD_TILE) terrain_map[i][j] = v;
        else if (v == 3) { player1.x = j; player1.y = i; player1.dir = UP; terrain_map[i][j] = 0; }
        else if (v == 4) { player2.x = j; player2.y = i; player2.dir = UP; terrain_map[i][j] = 0; }
        else terrain_map[i][j] = 0;
    }
    for (int i = 0; i < 5; i++) bullet_p1[i].active = bullet_p2[i].active = false;
}

void move_tank(tank *t, direction dir) {
    // 1. Actualizar dirección
    t->dir = dir;

    // === AQUÍ SE DECLARAN LAS VARIABLES ===
    // 'nx' es tu 'new_x' y 'ny' es tu 'new_y'
    int nx = t->x;
    int ny = t->y;

    // 2. Calcular coordenadas destino
    if (dir == UP) ny--; 
    else if (dir == DOWN) ny++; 
    else if (dir == LEFT) nx--; 
    else if (dir == RIGHT) nx++;
    
    // 3. Validar límites del mapa
    if (nx <= 0 || nx >= WIDTH-1 || ny <= 0 || ny >= HEIGHT-1) return;
    
    // 4. Validar colisión con muros (1 y 2)
    if (terrain_map[ny][nx] == 1 || terrain_map[ny][nx] == 2) return;
    
    // 5. Validar colisión con otro tanque
    tank *ot = (t == &player1) ? &player2 : &player1;
    if (nx == ot->x && ny == ot->y) return;

    // 6. Lógica de recoger Escudo (Item 6)
    if (terrain_map[ny][nx] == 6) {
        t->has_shield = true;       // Activar escudo
        terrain_map[ny][nx] = 0;    // Eliminar el objeto del mapa
    }

    // 7. Actualizar posición final
    t->x = nx; 
    t->y = ny;

    // === GENERACIÓN ALEATORIA DE ESCUDO ===
    // 10% de probabilidad (rand() % 100 < 10) al moverse
    if (rand() % 100 < 10) { 
        int intentos = 0;
        while (intentos < 10) {
            int ry = rand() % HEIGHT;
            int rx = rand() % WIDTH;
            
            // Verificamos que sea suelo (0) y que NO haya nadie ahí
            bool ocupado_por_p1 = (player1.x == rx && player1.y == ry);
            bool ocupado_por_p2 = (player2.x == rx && player2.y == ry);
            
            if (terrain_map[ry][rx] == 0 && !ocupado_por_p1 && !ocupado_por_p2) {
                terrain_map[ry][rx] = 6; // Aparece el escudo (número 6)
                break; 
            }
            intentos++;
        }
    }
}

void fire_bullet(tank *t, bullet *arr, int max) {
    // MODIFICACIÓN: Verificar si tiene vidas antes de disparar (opcional, pero lógico)
    if (t->lives <= 0) return;

    for (int i = 0; i < max; i++) if (!arr[i].active) {
        arr[i].active = true; arr[i].dir = t->dir;
        arr[i].x = t->x; arr[i].y = t->y;
        
        // Mover la bala un paso inmediatamente para que no nazca dentro del tanque
        if (t->dir == UP) arr[i].y--; else if (t->dir == DOWN) arr[i].y++;
        else if (t->dir == LEFT) arr[i].x--; else if (t->dir == RIGHT) arr[i].x++;
        
        // Validaciones iniciales de la bala
        if (arr[i].x < 0 || arr[i].x >= WIDTH || arr[i].y < 0 || arr[i].y >= HEIGHT) { arr[i].active = false; return; }
        if (terrain_map[arr[i].y][arr[i].x] == 2) arr[i].active = false;
        if (terrain_map[arr[i].y][arr[i].x] == 1) { terrain_map[arr[i].y][arr[i].x] = 0; arr[i].active = false; }
        // Las balas pasan sobre los escudos (6) sin destruirlos
        return;
    }
}

void update_single_bullet_array(bullet *arr, int max, tank *enemy) {
    for (int i = 0; i < max; i++) if (arr[i].active) {
        int nx = arr[i].x, ny = arr[i].y;
        if (arr[i].dir == UP) ny--; else if (arr[i].dir == DOWN) ny++;
        else if (arr[i].dir == LEFT) nx--; else if (arr[i].dir == RIGHT) nx++;
        
        // Límites
        if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) { arr[i].active = false; continue; }
        
        // Colisión Muros Indestructibles
        if (terrain_map[ny][nx] == 2) { arr[i].active = false; continue; }
        
        // Colisión Muros Destructibles
        if (terrain_map[ny][nx] == 1) { terrain_map[ny][nx] = 0; arr[i].active = false; continue; }
        
        // MODIFICACIÓN 3: Colisión con Enemigo (Vidas y Escudo)
        if (nx == enemy->x && ny == enemy->y && enemy->lives > 0) { 
            arr[i].active = false; 
            
            if (enemy->has_shield) {
                // Si tiene escudo, se rompe pero no pierde vida
                enemy->has_shield = false;
                // printf("¡ESCUDO ROTO!\n"); // Debug
            } else {
                // Si no tiene escudo, pierde vida
                enemy->lives--;
                // printf("¡GOLPE! Vidas restantes: %d\n", enemy->lives); // Debug
                
                if (enemy->lives <= 0) {
                    // Lógica de muerte (sacar del mapa o fin del juego)
                    // Por ahora, lo dejamos en el mapa pero con 0 vidas
                }
            }
            continue; 
        }
        
        arr[i].x = nx; arr[i].y = ny;
    }
}

void update_bullets() { 
    update_single_bullet_array(bullet_p1,5,&player2); 
    update_single_bullet_array(bullet_p2,5,&player1); 
}

// Función auxiliar para contar balas disponibles
int contar_balas_disponibles(bullet *arr, int max) {
    int count = 0;
    for(int i=0; i<max; i++) if(!arr[i].active) count++;
    return count;
}

// --- AL INICIO DE LOGICA_V2.C (Junto a los otros includes) ---
// Declaramos que existe una función externa llamada visualizar_laberinto
void visualizar_laberinto(int **matriz, int filas, int columnas);


// --- REEMPLAZA TU FUNCIÓN generate_and_print_output_map POR ESTA ---
void generate_and_print_output_map() {
    // 1. Limpieza de pantalla (Mantenemos tu optimización ANSI)
    printf("\033[H\033[J"); 

    // 2. Imprimimos el HUD (Vidas, Balas, etc.)
    // La interfaz solo dibuja el mapa, los datos numéricos los pones tú aquí.
    printf("=== BATTLE CITY ===\n");
    printf("P1: Vidas[%d] Escudo[%s] | P2: Vidas[%d] Escudo[%s]\n", 
           player1.lives, player1.has_shield ? "ON" : "OFF", 
           player2.lives, player2.has_shield ? "ON" : "OFF");

    // 3. CREAR MATRIZ TEMPORAL PARA LA VISUALIZACIÓN
    // Necesitamos una matriz "foto" que combine terreno + jugadores + balas
    // para pasársela limpia a la función de interfaz.
    int **display_map = (int**)malloc(HEIGHT * sizeof(int*));
    for(int i=0; i<HEIGHT; i++) {
        display_map[i] = (int*)malloc(WIDTH * sizeof(int));
    }

    // 4. LLENAR LA MATRIZ TEMPORAL
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            int v = terrain_map[i][j]; // Base: Terreno (0, 1, 2, 6)
            
            // Superponer Jugadores
            if (player1.lives > 0 && player1.y==i && player1.x==j) v = 3;
            else if (player2.lives > 0 && player2.y==i && player2.x==j) v = 4;
            else {
                // Superponer Balas
                for (int b=0; b<5; b++) {
                    if ((bullet_p1[b].active && bullet_p1[b].y==i && bullet_p1[b].x==j) ||
                        (bullet_p2[b].active && bullet_p2[b].y==i && bullet_p2[b].x==j)) {
                        v = 5;
                        break; 
                    }
                }
            }
            display_map[i][j] = v;
        }
    }

    // 5. LLAMAR A LA INTERFAZ
    // Aquí ocurre la magia: le damos la matriz lista para imprimir bonito
    visualizar_laberinto(display_map, HEIGHT, WIDTH);

    // 6. IMPRIMIR CONTROLES (Abajo del mapa)
    printf("\n[WASD]: P1  [IJKL]: P2  [F/H]: Disparo  [G]: Guardar  [Q]: Salir\n");

    // 7. LIMPIEZA DE MEMORIA TEMPORAL
    // Como usamos malloc, debemos liberar esto inmediatamente para no saturar la RAM
    for(int i=0; i<HEIGHT; i++) free(display_map[i]);
    free(display_map);
}
void cleanup_game() {
    // Liberar mapa del terreno
    if (terrain_map) {
        for (int i = 0; i < HEIGHT; i++) {
            free(terrain_map[i]);
        }
        free(terrain_map);
    }
    
    // Liberar mapa de entrada original (si sigue en memoria)
    if (test_input_map) {
        for (int i = 0; i < HEIGHT; i++) {
            free(test_input_map[i]);
        }
        free(test_input_map);
    }
}
// Función auxiliar para leer una tecla SIN esperar Enter en Linux
int getch(void) {
    struct termios oldattr, newattr;
    int ch;
    
    // 1. Obtener la configuración actual de la terminal
    tcgetattr(STDIN_FILENO, &oldattr);
    newattr = oldattr;
    
    // 2. Desactivar ICANON (buffer de línea) y ECHO (imprimir tecla)
    newattr.c_lflag &= ~(ICANON | ECHO);
    
    // 3. Aplicar la nueva configuración inmediatamente
    tcsetattr(STDIN_FILENO, TCSANOW, &newattr);
    
    // 4. Leer la tecla
    ch = getchar();
    
    // 5. Restaurar la configuración original (IMPORTANTE)
    tcsetattr(STDIN_FILENO, TCSANOW, &oldattr);
    
    return ch;
}
void input_process() {
    if (player1.lives <= 0 && player2.lives <= 0) {
        printf("GAME OVER. Q para salir.\n");
    }

    char c = getch();

    switch(c) {
        // ... (TUS CONTROLES DE MOVIMIENTO IGUAL QUE ANTES) ...
        case 'w': case 'W': if(player1.lives > 0) move_tank(&player1,UP); break;
        case 'a': case 'A': if(player1.lives > 0) move_tank(&player1,LEFT); break;
        case 's': case 'S': if(player1.lives > 0) move_tank(&player1,DOWN); break;
        case 'd': case 'D': if(player1.lives > 0) move_tank(&player1,RIGHT); break;
        case 'f': case 'F': if(player1.lives > 0) fire_bullet(&player1,bullet_p1,5); break;

        case 'i': case 'I': if(player2.lives > 0) move_tank(&player2,UP); break;
        case 'j': case 'J': if(player2.lives > 0) move_tank(&player2,LEFT); break;
        case 'k': case 'K': if(player2.lives > 0) move_tank(&player2,DOWN); break;
        case 'l': case 'L': if(player2.lives > 0) move_tank(&player2,RIGHT); break;
        case 'h': case 'H': if(player2.lives > 0) fire_bullet(&player2,bullet_p2,5); break;

        case 'g': case 'G': guardar_partida("save.txt"); break;
        
        // MODIFICACIÓN PARA SALIR LIMPIO
        case 'q': case 'Q': 
            cleanup_game(); // Liberamos memoria antes de matar el programa
            exit(0);
    }
}
// ==================== MAIN ====================
int main() {
    srand(time(NULL));
    // Asegúrate de que tu archivo mapa7x7.txt tenga algún '6' para probar el escudo
    // O puedes editar manualmente el mapa después de cargarlo para probar.
    test_input_map = cargar_mapa("mapa7x7.txt", &HEIGHT, &WIDTH);
    if (!test_input_map) return 1;
    
    initialize_game();
    
    // TRUCO DE PRUEBA: Si tu mapa no tiene un 6, descomenta la línea de abajo para poner uno
    // terrain_map[2][2] = 6; 
    while(true) {
        generate_and_print_output_map();
        input_process();
        update_bullets();
        // usleep(100000); // En Windows puede que necesites Sleep(100) y <windows.h>
    }
}
