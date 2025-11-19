#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

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
typedef struct { int x; int y; direction dir; } tank;
typedef struct { int x; int y; direction dir; bool active; } bullet;

tank player1, player2;
bullet bullet_p1[5];
bullet bullet_p2[5];
#define BULLET_TILE 5

// ==================== GUARDAR PARTIDA (SÍ FUNCIONA) ====================
void guardar_partida(const char* nombre) {
    FILE* f = fopen(nombre, "w");
    if (!f) { printf("ERROR al guardar\n"); return; }

    fprintf(f, "%d %d\n", HEIGHT, WIDTH);
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) fprintf(f, "%d ", terrain_map[i][j]);
        fprintf(f, "\n");
    }
    fprintf(f, "#P1 %d %d %d\n", player1.x, player1.y, player1.dir);
    fprintf(f, "#P2 %d %d %d\n", player2.x, player2.y, player2.dir);
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

    for (int i = 0; i < HEIGHT; i++) for (int j = 0; j < WIDTH; j++) {
        int v = test_input_map[i][j];
        if (v == 0 || v == 1 || v == 2) terrain_map[i][j] = v;
        else if (v == 3) { player1.x = j; player1.y = i; player1.dir = UP; terrain_map[i][j] = 0; }
        else if (v == 4) { player2.x = j; player2.y = i; player2.dir = UP; terrain_map[i][j] = 0; }
        else terrain_map[i][j] = 0;
    }
    for (int i = 0; i < 5; i++) bullet_p1[i].active = bullet_p2[i].active = false;
}

// ==================== EL RESTO DE FUNCIONES (igual que antes) ====================
void move_tank(tank *t, direction dir) {
    t->dir = dir;
    int nx = t->x, ny = t->y;
    if (dir == UP) ny--; else if (dir == DOWN) ny++; else if (dir == LEFT) nx--; else if (dir == RIGHT) nx++;
    if (nx <= 0 || nx >= WIDTH-1 || ny <= 0 || ny >= HEIGHT-1) return;
    if (terrain_map[ny][nx] == 1 || terrain_map[ny][nx] == 2) return;
    tank *ot = (t == &player1) ? &player2 : &player1;
    if (nx == ot->x && ny == ot->y) return;
    t->x = nx; t->y = ny;
}

void fire_bullet(tank *t, bullet *arr, int max) {
    for (int i = 0; i < max; i++) if (!arr[i].active) {
        arr[i].active = true; arr[i].dir = t->dir;
        arr[i].x = t->x; arr[i].y = t->y;
        if (t->dir == UP) arr[i].y--; else if (t->dir == DOWN) arr[i].y++;
        else if (t->dir == LEFT) arr[i].x--; else if (t->dir == RIGHT) arr[i].x++;
        if (arr[i].x < 0 || arr[i].x >= WIDTH || arr[i].y < 0 || arr[i].y >= HEIGHT) { arr[i].active = false; return; }
        if (terrain_map[arr[i].y][arr[i].x] == 2) arr[i].active = false;
        if (terrain_map[arr[i].y][arr[i].x] == 1) { terrain_map[arr[i].y][arr[i].x] = 0; arr[i].active = false; }
        return;
    }
}

void update_single_bullet_array(bullet *arr, int max, tank *enemy) {
    for (int i = 0; i < max; i++) if (arr[i].active) {
        int nx = arr[i].x, ny = arr[i].y;
        if (arr[i].dir == UP) ny--; else if (arr[i].dir == DOWN) ny++;
        else if (arr[i].dir == LEFT) nx--; else if (arr[i].dir == RIGHT) nx++;
        if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) { arr[i].active = false; continue; }
        if (terrain_map[ny][nx] == 2) { arr[i].active = false; continue; }
        if (terrain_map[ny][nx] == 1) { terrain_map[ny][nx] = 0; arr[i].active = false; continue; }
        if (nx == enemy->x && ny == enemy->y) { arr[i].active = false; printf("¡GOLPE!\n"); continue; }
        arr[i].x = nx; arr[i].y = ny;
    }
}
void update_bullets() { update_single_bullet_array(bullet_p1,5,&player2); update_single_bullet_array(bullet_p2,5,&player1); }

void generate_and_print_output_map() {
    system("cls||clear");
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            int v = terrain_map[i][j];
            if (player1.y==i && player1.x==j) v = 3;
            if (player2.y==i && player2.x==j) v = 4;
            for (int b=0; b<5; b++) {
                if (bullet_p1[b].active && bullet_p1[b].y==i && bullet_p1[b].x==j) v = 5;
                if (bullet_p2[b].active && bullet_p2[b].y==i && bullet_p2[b].x==j) v = 5;
            }
            printf("%d ", v);
        }
        printf("\n");
    }
    printf("w a s d = mover | f = disparar | g = GUARDAR | q = salir\n");
}

void input_process() {
    char c = getchar(); while(getchar()!='\n');
    switch(c) {
        case 'w': case 'W': move_tank(&player1,UP); break;
        case 'a': case 'A': move_tank(&player1,LEFT); break;
        case 's': case 'S': move_tank(&player1,DOWN); break;
        case 'd': case 'D': move_tank(&player1,RIGHT); break;
        case 'f': case 'F': fire_bullet(&player1,bullet_p1,5); break;
        case 'g': case 'G': guardar_partida("save.txt"); break;
        case 'q': case 'Q': exit(0);
    }
}

// ==================== MAIN ====================
int main() {
    test_input_map = cargar_mapa("mapa7x7.txt", &HEIGHT, &WIDTH);
    if (!test_input_map) return 1;
    initialize_game();
    printf("¡JUEGO LISTO! Pulsa g para guardar en cualquier momento.\n");
    while(1) {
        generate_and_print_output_map();
        input_process();
        update_bullets();
        usleep(100000);
    }
}