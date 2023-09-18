compile:
	gcc main.c glad/src/glad.c -ograppleman -O2 -lcglm -lglfw -lGL -lX11 -lpthread -ldl -lm -Iglad/include
	
debug:
	gcc main.c glad/src/glad.c -ograppleman -O2 -lcglm -lglfw -lGL -lX11 -lpthread -ldl -lm -Iglad/include -g
	
	
windows:
	x86_64-w64-mingw32-gcc main.c glad/src/glad.c -ograppleman -O2 -lglfw3 -lopengl32 -lgdi32 -Iglad/include

