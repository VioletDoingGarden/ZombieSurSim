# Build the main game executable
tgame4: tgame4.cpp utils.cpp Weather.cpp startgame.cpp
	g++ tgame4.cpp utils.cpp Weather.cpp startgame.cpp -o tgame4 -lSDL2 -lSDL2_ttf -lSDL2_image -lSDL2_gfx

# Build and run the game
run: tgame4.cpp utils.cpp Weather.cpp startgame.cpp
	g++ tgame4.cpp utils.cpp Weather.cpp startgame.cpp -o tgame4 -lSDL2 -lSDL2_ttf -lSDL2_image -lSDL2_gfx
	./tgame4

# Remove the executable and object files
clean:
	rm -f tgame4

# Example: make tgame4 to build, make run to build and run, make clean to remove executable