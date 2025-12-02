INSTRUCCIONES

1. Una vez abierto su archivo encriptado, verifique que todos los archivos hayan quedado en la misma de la carpeta.
2. Luego abra su terminal y debe ejecutar los sigueintes comandos:
   --> xcode-select --install
   --> brew install sdl2 sdl2_image sdl2_ttf sdl2_mixer
3. Luego de que estos comando hayan sido instalados exitosamente, puede abrir la terminal y ejecutar lo siguiente:
   --> gcc -o mi_juego battlecity.c $(sdl2-config --cflags --libs) -I/opt/homebrew/include -lSDL2_image -lSDL2_ttf -lSDL2_mixer (en caso de ser MacOs)
   --> Despues ./mi_juego
   Ahi se iniciara la ejecución del programa y se abrira una ventana con el juego.

Para Windows:
1. Descargue e instale MSYS2 desde msys2.org.
2. Abra la terminal llamada "MSYS2 MinGW 64-bit" (o UCRT64).
3. Instale las librerías necesarias copiando y pegando este comando:
   --> pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_image mingw-w64-x86_64-SDL2_ttf mingw-w64-x86_64-SDL2_mixer.
4. Luego de que estos comando hayan sido instalados exitosamente, puede abrir la terminal y ejecutar lo siguiente:
   --> gcc -o mi_juego inter_comp2.c logica_v2.c -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer
   --> Despues:./mi_juego
   Se abrira una ventana respectivamemte donde podra interactuar con dicho programa.
   
COMANDOS DE JUEGO:

JUGADOR 1 (Tanque Amarillo)
    Moverse: Teclas W, A, S, D
    Disparar: Tecla F
    
JUGADOR 2 (Tanque Verde)
    Moverse: Flechas de dirección (↑, ↓, ←, →)
    Disparar: Tecla Enter (Intro)
    
GENERAL
    Reiniciar: Espacio (Solo cuando termina la partida)
    Salir: Tecla Q

