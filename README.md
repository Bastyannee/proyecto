INSTRUCCIONES

1. Una vez abierto su archivo encriptado, verifique que todos los archivos hayan quedado en la misma de la carpeta.
2. Luego abra su terminal y debe ejecutar los sigueintes comandos:
   --> xcode-select --install
   --> brew install sdl2 sdl2_image sdl2_ttf sdl2_mixer
3. Luego de que estos comando shayan sido instalados exitosamente, puede abrir la terminal y ejecutar lo siguiente:
   --> gcc -o mi_juego battlecity.c $(sdl2-config --cflags --libs) -I/opt/homebrew/include -lSDL2_image -lSDL2_ttf -lSDL2_mixer (en caso de ser MacOs)
   --> Despues ./mi_juego
   Ahi se iniciara la ejecución del programa y se abrira una ventana con el juego.
   
