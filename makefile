all:
	g++ mutantaliens.cpp -std=c++0x -O3 -D KILL_ESCAPE=1 -lncurses -o mutantaliens
esc_ok:
	g++ mutantaliens.cpp -std=c++0x -O3 -lncurses -o mutantaliens
mingw:
	g++ mutantaliens.cpp -std=c++0x -lpdcurses -O3 -o mutantaliens