#include <stdio.h>
#include <stdbool.h> //para usar varaible bool
#include <unistd.h>
#include <stdlib.h>
#define HEIGHT 12 //largo de la matriz base
#define WIDTH 15// ancho de la matriz base
// 0 = Camino (desplazable)
// 1 = Bloque destructible 
// 2 = Bloque indestructible4
// 3 = Tanque 1 (Jugador 1)
// 4 = Tanque 2 (Jugador 2)
int test_input_map[HEIGHT][WIDTH] = {//matriz base para implementar la logica del juego
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2}, // fila 0
    {2, 0, 0, 1, 1, 1, 0, 2, 0, 1, 1, 1, 0, 0, 2}, // fila 1
    {2, 0, 3, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 2}, // fila 2 (spawn J1)
    {2, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 0, 2}, // fila 3
    {2, 0, 1, 0, 2, 0, 1, 0, 1, 0, 2, 0, 1, 0, 2}, // fila 4
    {2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2}, // fila 5
    {2, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 2}, // fila 6
    {2, 0, 1, 0, 2, 0, 1, 0, 1, 0, 2, 0, 1, 0, 2}, // fila 7
    {2, 0, 0, 1, 1, 1, 0, 1, 0, 1, 1, 1, 0, 0, 2}, // fila 8
    {2, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0, 4, 0, 2}, // fila 9 (spawn J2)
    {2, 0, 0, 1, 1, 1, 0, 2, 0, 1, 1, 1, 0, 0, 2}, // fila 10
    {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2}  // fila 11
};

//definicion de structs
typedef enum{UP,DOWN,LEFT,RIGHT} direction;
typedef struct {int x; int y; direction dir;} tank;
typedef struct {int x; int y; direction dir; bool active;} bullet;

//definicion de variables globales de estado
int terrain_map[HEIGHT][WIDTH];//matriz para el terrerno que guardar solo 0, 1 y 2
tank player1;
tank player2;
bullet bullet_p1[5];
bullet bullet_p2[5];

void initialize_game(){//recorre la matriz base usando ciclos anidados e inicializa la matriz del terreno con la matriz base
    int i,j;
    for(i=0;i<HEIGHT;i++){
        for(j=0;j<WIDTH;j++){
            
            if(test_input_map[i][j]==0) terrain_map[i][j]=test_input_map[i][j];
            
            else if(test_input_map[i][j]==1) terrain_map[i][j]=test_input_map[i][j];
            
            else if(test_input_map[i][j]==2) terrain_map[i][j]=test_input_map[i][j];
            
            else if(test_input_map[i][j]==3){
                player1.x = j; player1.y = i; player1.dir = UP; terrain_map[i][j]=0;
            }
            
            else if(test_input_map[i][j]==4){
               player2.x = j; player2.y = i; player2.dir = UP; terrain_map[i][j]=0; 
            }
        }
    }
    
    for(i=0;i<5;i++){//desactiva las balas
        bullet_p1[i].active=false;
        bullet_p2[i].active=false;      
    }
}

void move_tank(tank *t,direction dir){//movimiento y actualizacion de la hitbox del tanque
    t->dir = dir;//actualizamos la direccion del tanque, incluso si el movimiento falla o es invalido

    int new_x = t->x, new_y = t->y;

    switch (dir){//calculamos las coordenadas de destino
        case UP: new_y = t-> y-1; break;
        case DOWN:  new_y = t->y + 1; break;
        case LEFT:  new_x = t->x - 1; break;
        case RIGHT: new_x = t->x + 1; break;
    }

    tank *other_tank = (t == &player1) ? &player2 : &player1;// declaracion y asignacion del puntero al otro tanque

    //debemos verificar los limites del mapa (matriz)
    if(new_x <= 0 || new_x >= WIDTH - 1 || new_y <= 0 || new_y >= HEIGHT - 1){
        return;
    }
    
    
    if(terrain_map[new_y][new_x] == 1 || terrain_map[new_y][new_x] == 2 ){//verificacion de colision con los muros
        return;
    }

    if(new_x == other_tank->x && new_y == other_tank->y ){//movimiento bloqueado por colision con el otro tanque
        return;
    }

    //si no hay colisiones se actualiza la posicion
    t->x = new_x;
    t->y = new_y;

}

void fire_bullet(tank *t, bullet *bullet_array, int max_bullets){
    int i;
    bullet *new_bullet = NULL;
    
    for (i = 0; i < max_bullets; i++) {
        if (bullet_array[i].active == false) {
            new_bullet = &bullet_array[i];
            break; // encontró un espacio, sale del bucle
        }
    }

    //si new_bullet es NULL, significa que el tanque ya tiene 'max_bullets' activas
    if(new_bullet == NULL){
        return;
    }

    //inicializamos la bala frente a la posicion del tanque
    new_bullet->dir = t->dir;
    new_bullet->active = true;

    new_bullet->x = t->x;
    new_bullet->y = t->y;

    switch (t->dir) {
        case UP:    new_bullet->y--; break;
        case DOWN:  new_bullet->y++; break;
        case LEFT:  new_bullet->x--; break;
        case RIGHT: new_bullet->x++; break;
    }


    
    // verificar si la bala nació fuera de los limites del mapa
    if (new_bullet->x < 0 || new_bullet->x >= WIDTH || new_bullet->y < 0 || new_bullet->y >= HEIGHT) {
        new_bullet->active = false;
        return;
    }

    // verificar qué hay en la posición inicial de la bala
    int initial_terrain = terrain_map[new_bullet->y][new_bullet->x];

    if (initial_terrain == 2) { 
        // nació dentro de un bloque indestructible
        new_bullet->active = false; // Se destruye la bala inmediatamente
    } 
    else if (initial_terrain == 1) { 
        // nació dentro de un bloque destructible
        new_bullet->active = false; // Se destruye la bala
        terrain_map[new_bullet->y][new_bullet->x] = 0; // Y se destruye el bloque
    }
}

#define BULLET_TILE 5
void generate_and_print_output_map(){
    int i,j;
    int output_map[HEIGHT][WIDTH];


    for(i=0; i < HEIGHT; i++){//copiamos el terreno (con los muros ya destruidos)
        for(j=0; j < WIDTH; j++){
            output_map[i][j] = terrain_map[i][j];
        }
    }

    for(i=0; i<5; i++){//"pintamos" las balas (J1)
        if(bullet_p1[i].active) output_map[bullet_p1[i].y][bullet_p1[i].x] = BULLET_TILE;
    }
    //aqui tambien iria el bucle para las balas del J2 si estuvieran activas


    //"pintamos" los tanques (encima de todo)
    output_map[player1.y][player1.x] = 3;
    output_map[player2.y][player2.x] = 4;

    //imprimimos la matriz en la consola
    //opcional: limpiar la consola
    printf("\n----NUEVO FRAME----\n");
    for(i=0; i < HEIGHT; i++){
        for(j=0; j< WIDTH; j++){
            printf("%d ", output_map[i][j]);
        }
        printf("\n");
    }
}

void input_process() {
    printf("Ingrese una instruccion (w=arriba, s=abajo, a=izq, d=der, f=fuego): \n");

    // 1. Leer un solo caracter de la entrada
    char input_char = getchar();

    // 2. Limpiar el búfer de entrada
    // Esto consume cualquier caracter extra que el usuario haya escrito
    // (como el 'Enter' o si escribió "wwww" por accidente).
    int temp_c;
    while ((temp_c = getchar()) != '\n' && temp_c != EOF);

    // 3. Procesar el caracter en el switch
    switch (input_char) {
        case 'w': // Arriba
        case 'W':
        case '1':
            move_tank(&player1, UP);
            break;

        case 's': // Abajo
        case 'S':
        case '2':
            move_tank(&player1, DOWN);
            break;

        case 'a': // Izquierda
        case 'A':
        case '3':
            move_tank(&player1, LEFT);
            break;

        case 'd': // Derecha
        case 'D':
        case '4':
            move_tank(&player1, RIGHT);
            break;

        case 'f': // dispara
        case 'F':
        case '5':
            fire_bullet(&player1, bullet_p1, 5);
            break;

        case 'q':
        case 'Q':
            exit(0);
            break;

        default:
            // si no se presionó una tecla válida no hace nadaq
            break;
    }
}

void update_single_bullet_array(bullet *bullet_array, int max_bullets, tank *enemy_tank) {
    int i;
    for(i=0;i<max_bullets;i++){
        bullet *b = &bullet_array[i];//obtenemos un puntero a la balla actual para falicitar la escritura

    //primero omitimos las balas inactivas
    if(b->active == false) continue;
    
    //calculamos la siguiente posicion de la bala

    int new_x = b->x, new_y = b->y;

    switch(b->dir){
        case UP: new_y--; break;
        case DOWN: new_y++; break;
        case LEFT: new_x--; break;
        case RIGHT: new_x++; break;
    }

    //comprobamos las colisiones

    int terrain_at_next_pos = terrain_map[new_y][new_x];

    if(terrain_at_next_pos == 2){//choca con bloque indestructible
        b->active = false; continue; //pasa a la siguiente bala
    }

    if(terrain_at_next_pos == 1){//choca con objeto destructible
        b->active = false;
        terrain_map[new_y][new_x] = 0;
        continue;//pasa a la siguiente bala
    }

    if(new_x == enemy_tank->x && new_y == enemy_tank->y){
        b->active = false; //la bala se destruye

         printf("jugador alcanzado\n"); continue;
    }

    //movemos la tabla
    //si no colisiono con nada se actualiza su posicion
    b->x = new_x;
    b->y = new_y;
    }
}

void update_bullets(){

    update_single_bullet_array(bullet_p1, 5, &player2);

    update_single_bullet_array(bullet_p2, 5, &player1);
}

int main(){

    initialize_game();


    while(true){
        
        input_process();
        
        update_bullets();
        
        generate_and_print_output_map();
        

    }

    return 0;
}
