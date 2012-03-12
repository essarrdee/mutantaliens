all:
	g++ mutantaliens.cpp -std=c++0x -KILL_ESCAPE=1 -lncurses -o mutantaliens
esc_ok:
	g++ mutantaliens.cpp -std=c++0x -lncurses -o mutantaliens
