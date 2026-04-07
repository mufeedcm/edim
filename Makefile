edim: src/main.c src/editor.c src/ui.c
	gcc src/main.c src/editor.c src/ui.c -o edim -Wall -Wextra -pedantic -std=c99 -I/opt/homebrew/include -L/opt/homebrew/lib -lSDL3 -lSDL3_ttf
