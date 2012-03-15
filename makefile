all:
	g++ mutantaliens.cpp -std=c++0x -D KILL_ESCAPE=1 -lncurses -o mutantaliens
esc_ok:
	g++ mutantaliens.cpp -std=c++0x -lncurses -o mutantaliens
