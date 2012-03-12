#include <curses.h>
//#include <panel.h>
#include <random>
#include <time.h>
#include <vector>
#include <algorithm>
#include <string>
#include <math.h>

#pragma warning(disable: 4996)

/* TODO
make devices have an effect
make devices throwable, activateable, hideable, droppable, pickupable, remotable, timerable
make guns structs
make monsters spawn
make monsters react to smell
make monsters react better to sound, make sounds interesting
make monsters follow other monters to the player
make pack monsters try to maintain distance from player while a group assembles, before closing in
make detailed species descriptions
make monster memory
make buildings multilevel
make the plot stuff
*/

#define arrayend(ar)  ar + (sizeof(ar) / sizeof(ar[0]))


using namespace std;

const int KEY_ESC = 27;

const int MAP_SIZE = 150;
//terrain types
const int DIRT = 0;
const int STREE = 1;
const int BTREE = 2;
const int FLOOR = 3;
const int WALL = 4;
const int DOOR = 5;
const int TERRAIN_TYPES = 6; //this should be 1 more than the highest terrain type
const char terrain_chars[] = {'.','t','T','.','#','+'};
const int terrain_colours[] = {COLOR_GREEN,COLOR_GREEN,COLOR_GREEN,COLOR_WHITE,COLOR_WHITE,COLOR_WHITE};
const char* terrain_descriptors[] = {"soil", "a small tree", "a big tree", "a paved floor", "a wall","a door"};
bool debug = false;

const int WAVELENGTHS = 5;
int map_terrain[MAP_SIZE][MAP_SIZE];
int rm_devices[WAVELENGTHS];
float map_human_scent[MAP_SIZE][MAP_SIZE];
float spare_scent_map[MAP_SIZE][MAP_SIZE];
bool map_seen[MAP_SIZE][MAP_SIZE];
bool map_access[MAP_SIZE][MAP_SIZE];

//device states. represent native life with -species code.
const int NULL_STIMULUS = 0;
const int HUMAN_STIMULUS = 1;
const int LOUD_STIMULUS = 2;
const int LOUD2_STIMULUS = 3;

const int NOT_DEVICE = 0;
const int LOW_EXPLOSIVE = 1;
const int HIGH_EXPLOSIVE = 2;
const int HOLOGRAM_PROJECTOR = 3;
const int NOISE_GENERATOR = 4;
const int SCENT_GENERATOR = 5;
const int BRAIN_SLICE = 6;

//PANEL *map_display;
const int disp_lines = 25;
const int disp_columns = 80;
const int viewport_height = 24;
const int viewport_width = 50;
int centre_x = 0;
int centre_y = 0;
const int vis_range = 8;
const int psychic_1_range = 6;
const int psychic_0_range = 40;
const int PISTOL = 0;
const int RIFLE = 1;
const int CANNON = 2;
const int WEAPONS = 3;
int ammo[WEAPONS];
int current_weapon = PISTOL;

const int BUILDING_SIZE = 17;
int target_x;
int target_y;


const pair<int,int> directions[] = { make_pair(1,0), make_pair(1,1), make_pair(0,1), make_pair(-1,1),
									 make_pair(-1,0), make_pair(-1,-1), make_pair(0,-1), make_pair(1,-1)};
inline bool on_map(int x,int y)
{
	if (x < 0 || y < 0) return false;
	if(x >=MAP_SIZE || y >= MAP_SIZE) return false;
	return true;
}
//char* i2a(int i) {return sprintf("%d", i).c_str();}

float randfloat()
{
	return (float)rand()/(float)RAND_MAX;
}

struct species
{
	int min_health, health_range;
	int damage;
	bool sees, smells, hears, psychic;
	char ch;
	int colour;
	float move_noise[TERRAIN_TYPES];
	float still_noise;
	float melee_noise;
	string syl1, syl2, name;
	string description;
};

inline int viewport_left_edge()
{
	return centre_x-viewport_width/2;
}
inline int viewport_right_edge()
{
	return centre_x+viewport_width/2;
}
inline int viewport_top_edge()
{
	return centre_y-viewport_height/2;
}
inline int viewport_bottom_edge()
{
	return centre_y+viewport_height/2;
}

const int ZOO_SIZE = 7;
vector<species*> zoo;

const char* const clusters_1[] = {"b","c","d","f","g","h","j","k","l","m","n","p","r","s","t","v","w","x","y","z"};
vector<string> vclusters_1(clusters_1,arrayend(clusters_1));
const char* const start_clusters_2[] = {"bh","bl","br","bw","by","bz","ch","cj","cl","cn","cr","cw","dh","dj","dl","dr",
	"dv","dw","dy","dz","fd","fj","fk","fl","fm","fn","fp","ft","fw","fy","gh","gj","gl","gn","gr","gv","gw",
	"gy","gz","hj","hl","hr","hy","hw","jh","jl","jn","jy","kf","kh","kj","kl","kn","kr","ks","kv","kw","ky",
	"kz","lh","lj","ll","lw","ly","mb","mf","mh","ml","mn","mp","mr","mw","my","nd","ng","nh","nj","nk","nr",
	"nt","nw","ny","pf","ph","pj","pl","pr","ps","pt","pw","py","qu","qv","qw","rh","rj","rw","ry","sc","sf",
	"sh","sj","sk","sl","sm","sn","sp","sr","st","sv","sw","sy","sz","th","tj","tl","tr","ts","tw","ty","tz",
	"vh","vj","vk","vl","vm","vn","vp","vr","vt","vw","vy","vz","wh","wr","wy","zb","zc","zd","zf","zg","zh",
	"zj","zk","zl","zm","zn","zp","zr","zs","zt","zv","zw","zy","zz"};
vector<string> vstart_clusters_2(start_clusters_2,arrayend(start_clusters_2));
const char* const start_clusters_3[] = {"chl","chr","scl","scr","skl","skr","spl","spr","str","str"};
vector<string> vstart_clusters_3(start_clusters_3,arrayend(start_clusters_3));
const char* const pure_vowels[] = {"a","aa","e","ee","i","oo","u","uu"};
vector<string> vpure_vowels(pure_vowels,arrayend(pure_vowels));
const char* const diphthongs[] = {"ae","ai","ao","au","aw","ay","ea","ei","eo","eu","ew","ey","ia","ie","io","iu","oa",
	"oe","oi","ou","ow","oy","ua","ue","ui","uo","uy","uw"};
vector<string> vdiphthongs(diphthongs,arrayend(diphthongs));
const char* const end_clusters_2[] = {"bb","bh","bl","br","bz","cc","ch","ck","cl","cr","cs","cr","cz","dd","dh",
	"dj","dl","dr","dt","dz","ff","fh","fl","fp","ft","fs","ft","gg","gh","gl","gr","gz","jj","kh","kk","kl",
	"kr","ks","kt","lb","lc","ld","lf","lg","lh","lj","lk","ll","lm","ln","lp","ls","lt","lv","lx","lz","mb",
	"mf","mh","mm","mp","ms","mz","nc","nd","nf","ng","nh","nj","nk","nn","ns","nt","nx","nz","pl","pp","pr",
	"ps","sh","sp","ss","st","vv","vh","vz","xx","zb","zd","zg","zh","zj","zl","zz"};
vector<string> vend_clusters_2(end_clusters_2,arrayend(end_clusters_2));
//const string end_clusters_3[] = {""};
string random_start_cluster()
{
	switch (rand()%10)
	{
	case 0: case 1: case 2: case 3:
		return clusters_1[rand()%vclusters_1.size()];
	case 4: case 5: case 6:
		return start_clusters_2[rand()%vstart_clusters_2.size()];
	default:
		return start_clusters_3[rand()%vstart_clusters_3.size()];
	}
}
string random_vowel()
{
	switch (rand()%5)
	{
	case 0: case 1: case 2:
		return pure_vowels[rand()%vpure_vowels.size()];
	default:
		return diphthongs[rand()%vdiphthongs.size()];
	}
}
string random_end_cluster()
{
	switch (rand()%7)
	{
	case 0: case 1: case 2: case 3:
		return clusters_1[rand()%vclusters_1.size()];
	default://case 4: case 5: case 6:
		return end_clusters_2[rand()%vend_clusters_2.size()];
	//default:
	//	return start_clusters_3[rand()%start_clusters_3->length()];
	}
}
string random_syllable()
{
	string syl = "";
	switch (rand()%5)
	{
	case 0:
		return random_vowel() + random_end_cluster();
	case 1:
		return random_start_cluster() + random_vowel();
	default:
		return random_start_cluster() + random_vowel() + random_end_cluster();
	}
}

void init_species()
{
	species* human = new species();
	human->health_range = 0; human->min_health = 100;
	human->ch = '@';
	human->psychic = false; human->hears = true;
	human->smells = false; human->sees = true;
	human->syl1 = "hu"; human->syl2 = "man";
	human->name = "human";
	human->colour = COLOR_WHITE;
	human->damage = 1;
	human->description = "An legendary advanced alien race whose technology is built specifically for murder, ecological destruction, deception, and gentle shaking. Members of this species must be killed on immediately upon discovery to prevent catastrophic damage to the whole area.";
	zoo.push_back(human);
	for (int spsp=0; spsp<ZOO_SIZE; spsp++)
	{
		species* alien = new species();
		alien->syl1 = random_syllable();
		alien->syl2 = random_syllable();
		alien->name = alien->syl1+"-"+alien->syl2;
		alien->damage = rand()%10 + 4;
		alien->min_health = 7+(rand()%60);
		alien->health_range = (rand()%(alien->min_health/3));
		alien->psychic = (rand()%2)!=0;
		alien->sees = (rand()%2)!=0;
		alien->hears = (rand()%2)!=0;
		alien->smells = (rand()%2)!=0;
		alien->ch = alien->name[0];
		alien->colour = rand()%6+1;
		char buf[1000] = "";
		sprintf(buf,"A %s is a %s creature %s.",
			alien->name.c_str(),
			(string(alien->psychic?"psychic":"")+string(alien->psychic&&alien->sees?", ":"")+string(alien->sees?"sighted":"")).c_str(),
			(string(alien->smells||alien->hears?"with good ":"")+string(alien->hears?"hearing":"")+
			 string(alien->hears&&alien->smells?" and ":"")+string(alien->smells?"sense of smell":"")).c_str());
		alien->description = string(buf);
		zoo.push_back(alien);
	}
}

struct actor
{
	species* sspecies;
	bool is_player;
	int health;
	bool dead;
	int x, y;
	int ai_x, ai_y;
	float noise_this_turn;
};

actor* p_ptr = NULL;
actor* map_occupants[MAP_SIZE][MAP_SIZE];

struct item
{
	int colour;
	char ch;
	string name;
	string description;
	int device_type;
	int wavelength;
	int time_remaining;
	int species_stored;
	int current_action;
	int next_action;
	bool on_timer;
	bool on_remote;
	int x;
	int y;
	bool player_has;
};

item* map_items[MAP_SIZE][MAP_SIZE];
vector<actor*> actors;
vector<item*> items;
vector<item*> devices;

void add_junk(string name, string description, char ch, int colour, int x, int y)
{
	item* it = new item();
	it->colour = colour;
	it->ch = ch;
	it->device_type = NOT_DEVICE;
	it->name = name;
	it->description = description;
	items.push_back(it);
	if(map_items[x][y] == NULL)
	{
		map_items[x][y] = it;
		it->x = x;
		it->y = y;
	}
	else
	{
		for(int d=0;d<8;d++)
		{
			pair<int,int> dir = directions[d];
			if(map_items[x+dir.first][y+dir.second] == NULL)
			{
				map_items[x][y] = it;
				it->x = x+dir.first;
				it->y = y+dir.second;
				break;
			}
		}
	}
}

actor* add_actor(species* sp, bool p, int xx, int yy)
{
	actor* a = new actor();
	a->sspecies = sp;
	a->is_player = p;
	a->dead = false;
	a->x = xx;
	a->y = yy;
	a->ai_x = xx;
	a->ai_y = yy;
	actors.push_back(a);
	map_occupants[xx][yy] = a;
	a->health = (sp->health_range? rand()%(sp->health_range):0)+sp->min_health;
	return a;
}

vector<string> rlmessages;
inline string padmsg(string msg)
{
	return msg + string(viewport_width-msg.length(),' ');
}
int add_message(string msg)
{
	if(msg.length() > viewport_width) return 1;
	rlmessages.push_back(msg);
	return 0;
}
inline bool is_corner_a(int x,int y,int xsize,int ysize)
{
	return (x==0 || x == xsize) && (y==0 || y==ysize);
}
inline bool is_corner_b(int x,int y)
{
	bool n = (map_terrain[x][y-1] != WALL);
	bool s = (map_terrain[x][y+1] != WALL);
	bool e = (map_terrain[x+1][y] != WALL);
	bool w = (map_terrain[x-1][y] != WALL);
	if (n || s) return (e || w);
	return false;
}

void make_room(pair<int,int> centre, pair<int,int> size)
{
	int x=centre.first;int y=centre.second;int xsize=size.first;int ysize=size.second;
	vector<pair<int,int>> inner_walls;
	vector<pair<int,int>> outer_walls;
	for(int i=0;i<xsize+2;i++)
	{
		for(int j=0;j<ysize+2;j++)
		{
			if(i==0 || i == xsize+1 || j==0 || j==ysize+1)
			{
				if (map_terrain[i+x-xsize/2][j+y-ysize/2] == DIRT || map_terrain[i+x-xsize/2][j+y-ysize/2] == STREE || map_terrain[i+x-xsize/2][j+y-ysize/2] == BTREE)
				{
					map_terrain[i+x-xsize/2][j+y-ysize/2] = WALL;
					if(!is_corner_a(i,j,xsize+1,ysize+1))
					{ outer_walls.push_back(make_pair(i+x-xsize/2,j+y-ysize/2)); }
				}
			}
			else
			{
				if(map_terrain[i+x-xsize/2][j+y-ysize/2] != WALL && map_terrain[i+x-xsize/2][j+y-ysize/2] != DOOR)
				{
					map_terrain[i+x-xsize/2][j+y-ysize/2] = FLOOR;
				}
				else if(!is_corner_b(i+x-xsize/2,j+y-ysize/2))
				{
					inner_walls.push_back(make_pair(i+x-xsize/2,j+y-ysize/2));
				}
			}
		}
	}
	if(inner_walls.size()>0)
	{
		pair<int,int> inner_door_choice = inner_walls[rand()%inner_walls.size()];
		map_access[inner_door_choice.first+1][inner_door_choice.second] = true;
		map_access[inner_door_choice.first-1][inner_door_choice.second] = true;
		map_access[inner_door_choice.first][inner_door_choice.second+1] = true;
		map_access[inner_door_choice.first][inner_door_choice.second-1] = true;
		map_terrain[inner_door_choice.first][inner_door_choice.second] = DOOR;
	}
	pair<int,int> outer_door_choice = outer_walls[rand()%outer_walls.size()];
	map_terrain[outer_door_choice.first][outer_door_choice.second] = DOOR;
	map_access[outer_door_choice.first+1][outer_door_choice.second] = true;
	map_access[outer_door_choice.first-1][outer_door_choice.second] = true;
	map_access[outer_door_choice.first][outer_door_choice.second+1] = true;
	map_access[outer_door_choice.first][outer_door_choice.second-1] = true;

}

pair<int,int> can_make_room(pair<int,int> xy,int xcentre,int ycentre)
{
	int x=xy.first; int y = xy.second;
	int xsize = rand()%5 + 3;
	int ysize = rand()%5 + 3;
	int inner_walls=0;
	int outer_walls=0;
	int floors=0;

	for(int i=0;i<xsize+2;i++)
	{
		for(int j=0;j<ysize+2;j++)
		{
			if (!on_map(i+x-xsize/2,j+y-ysize/2)) {return make_pair(0,0);}
			if (i+x-xsize/2 > xcentre+BUILDING_SIZE || i+x-xsize/2 < xcentre - BUILDING_SIZE) {return make_pair(0,0);}
			if (j+y-ysize/2 > ycentre+BUILDING_SIZE || j+y-ysize/2 < ycentre - BUILDING_SIZE) {return make_pair(0,0);}
			if(map_access[i+x-xsize/2][j+y-ysize/2]) {return make_pair(0,0);}
			if(i==0 || i == xsize+1 || j==0 || j==ysize+1)
			{
				if (map_terrain[i+x-xsize/2][j+y-ysize/2] == DIRT || map_terrain[i+x-xsize/2][j+y-ysize/2] == STREE || map_terrain[i+x-xsize/2][j+y-ysize/2] == BTREE)
				{
					if(!is_corner_a(i,j,xsize+1,ysize+1))
					{ outer_walls++; }
				}
			}
			else
			{
				if(map_terrain[i+x-xsize/2][j+y-ysize/2] != WALL && map_terrain[i+x-xsize/2][j+y-ysize/2] != DOOR)
				{
					floors++;
				}
				else if(!is_corner_b(i+x-xsize/2,j+y-ysize/2))
				{
					inner_walls++;
				}
			}
		}
	}
	if(inner_walls>0 && outer_walls > 0 && floors > 3)
	{
		return make_pair(xsize,ysize);
	}
	return make_pair(0,0);
}

pair<int,int> five_tries_to_make_room(pair<int,int> xy, int xcentre, int ycentre)
{
	for(int i=0;i<5;i++)
	{
		pair<int,int> ret = can_make_room(xy,xcentre, ycentre);
		if(ret != make_pair(0,0)) return ret;
	}
	return make_pair(0,0);
}

void init_map()
{
	for(int x=0;x<MAP_SIZE; x++)
	{
		for(int y=0;y<MAP_SIZE; y++)
		{
			map_occupants[x][y] = NULL;
			map_items[x][y] = NULL;
			map_seen[x][y] = false;
			map_access[x][y] = false;
			map_human_scent[x][y] = 0.0;
			if(x != 0 && x != MAP_SIZE-1 && y != 0 && y != MAP_SIZE-1)
			{
				switch(rand()%11)
				{
				case 0: case 1: case 2: case 3:
					map_terrain[x][y] = STREE; break;
				case 4: case 5: case 6:
					map_terrain[x][y] = BTREE;break;
				default:
					map_terrain[x][y] = DIRT;
				}
			}
			else
			{
				map_terrain[x][y] = WALL;
			}
		}
	}
	int buildings = (rand()%3)+5;
	int *building_xcentres = (int*)malloc(buildings*sizeof(int));
	int *building_ycentres = (int*)malloc(buildings*sizeof(int));
	for (int i=0;i<buildings;i++)
	{
		building_xcentres[i] = -BUILDING_SIZE;
		building_ycentres[i] = -BUILDING_SIZE;
	}
	for(int i=0;i<buildings;i++)
	{
		bool is_ok = false;
		int xcentre = rand()%(MAP_SIZE-2*BUILDING_SIZE)+BUILDING_SIZE;
		int ycentre = rand()%(MAP_SIZE-2*BUILDING_SIZE)+BUILDING_SIZE;
		while(!is_ok)
		{
			int xcentre = rand()%(MAP_SIZE-2*BUILDING_SIZE)+BUILDING_SIZE;
			int ycentre = rand()%(MAP_SIZE-2*BUILDING_SIZE)+BUILDING_SIZE;
			is_ok = true;
			for(int j=0;j<i;j++)
			{
				if(abs(xcentre-building_xcentres[j])<=2*BUILDING_SIZE+3)
				{
					is_ok = false;
					xcentre = rand()%(MAP_SIZE-2*BUILDING_SIZE-2)+BUILDING_SIZE;
					ycentre = rand()%(MAP_SIZE-2*BUILDING_SIZE-2)+BUILDING_SIZE;
				}
			}
		}
		vector<pair<int,int>> room_centres;
		room_centres.push_back(make_pair(xcentre,ycentre));
		vector<pair<int,int>> room_sizes;
		room_sizes.push_back(make_pair(rand()%5 + 2,rand()%5+2));
		vector<int> fail_counts;
		fail_counts.push_back(0);
		make_room(room_centres[0],room_sizes[0]);
		int successive_failures = 0;
		int room_count = 0;
		while(successive_failures < 10 && room_count < 10)
		{
			int x_size = rand()%5 + 3;
			int y_size = rand()%5 + 3;
			int parent_choice = rand()%room_centres.size();
			pair<int,int> parent_centre = room_centres[parent_choice];
			pair<int,int> parent_size = room_sizes[parent_choice];
			vector<pair<int,int>> centre_choices;
			vector<pair<int,int>> size_choices;
			int choice_range_x = rand()%(x_size-2)+parent_size.first+2;
			int choice_range_y = rand()%(y_size-2)+parent_size.second+2;
			pair<int,int> candidate;
			pair<int,int> candidate_size;
			for(int x=0;x<choice_range_x;x++)
			{
				candidate=make_pair(x+parent_centre.first-choice_range_x/2,parent_centre.second-choice_range_y/2);
				candidate_size = five_tries_to_make_room(candidate,xcentre,ycentre);
				if(candidate_size != make_pair(0,0)) {centre_choices.push_back(candidate); size_choices.push_back(candidate_size);}

				candidate=make_pair(x+parent_centre.first-choice_range_x/2,choice_range_y-1+parent_centre.second-choice_range_y/2);
				candidate_size = five_tries_to_make_room(candidate,xcentre,ycentre);
				if(candidate_size != make_pair(0,0)) {centre_choices.push_back(candidate); size_choices.push_back(candidate_size);}
			}
			for(int y=1;y<choice_range_y;y++)
			{
				candidate = make_pair(parent_centre.first-choice_range_x/2,y+parent_centre.second-choice_range_y/2);
				candidate_size = five_tries_to_make_room(candidate,xcentre,ycentre);
				if(candidate_size != make_pair(0,0)) {centre_choices.push_back(candidate); size_choices.push_back(candidate_size);}

				candidate=make_pair(parent_centre.first-choice_range_x/2+choice_range_x-1,y+parent_centre.second-choice_range_y/2);
				candidate_size = five_tries_to_make_room(candidate,xcentre,ycentre);
				if(candidate_size != make_pair(0,0)) {centre_choices.push_back(candidate); size_choices.push_back(candidate_size);}
			}
			if(centre_choices.size() == 0)
			{
				successive_failures++;
			}
			else
			{
				successive_failures = 0;
				int choice = rand()%centre_choices.size();
				make_room(centre_choices[choice],size_choices[choice]);
				room_centres.push_back(centre_choices[choice]);
				room_sizes.push_back(size_choices[choice]);
				fail_counts.push_back(0);
				room_count++;
			}
			
		}
	}

	free(building_xcentres); free(building_ycentres);
}

void init_actors()
{
	p_ptr = add_actor(zoo[0],true,50,50);
	add_actor(zoo[(rand()%ZOO_SIZE)+1],false,40,60);
}

void swap(int *i, int *j) {
	int t=*i;
	*i=*j;
	*j=t;
}

bool is_wall(int x, int y)
{
	return map_terrain[x][y] == WALL;
}

bool is_opaque(int x, int y)
{
	return map_terrain[x][y] == WALL || map_terrain[x][y] == BTREE;
}

void init_items()
{
	ammo[PISTOL] = 150;
	ammo[RIFLE] = 300;
	ammo[CANNON] = 30;
}

void init()
{
	rlmessages.push_back(" ");
	initscr();
	if(has_colors() == FALSE)
	{
		endwin();
		printf("Your terminal does not support color\n");
		exit(1);
	}
    start_color();
	for(int i=0;i<8;i++)
	{
		init_pair(i,i,COLOR_BLACK);
	}
	noecho();
	cbreak();
	keypad(stdscr,TRUE);
	init_species();
	init_map();
	init_actors();
	init_items();
	target_x = rand()%MAP_SIZE; target_y = rand()%MAP_SIZE;
	while(map_terrain[target_x][target_y] != FLOOR)
	{
		target_x = rand()%MAP_SIZE; target_y = rand()%MAP_SIZE;
	}
	add_junk("radio transmitter","So this is what's been causing you so much trouble!",'?',COLOR_BLUE,target_x,target_y);
}

inline void draw_ch(int x,int y,char ch, int col)
{
	attron(COLOR_PAIR(col));
	mvaddch(1+y-viewport_top_edge(),x-viewport_left_edge(),ch);
}
void draw_stats()
{
	mvprintw(3,viewport_width,"HP: %d/%d    ",p_ptr->health,p_ptr->sspecies->min_health);
	mvprintw(4,viewport_width,"Ammo: %d,%d,%d      ",ammo[PISTOL],ammo[RIFLE],ammo[CANNON]);
}
void draw_tile_debug(int x, int y)
{
	char ch = terrain_chars[map_terrain[x][y]];
	int colour = terrain_colours[map_terrain[x][y]];
	if (!map_seen[x][y])
	{draw_ch(x,y,' ',colour); return;}
	if (map_items[x][y] != NULL)
	{
		ch = map_items[x][y]->ch;
		colour = map_items[x][y]->colour;
	}
	if (map_occupants[x][y] != NULL)
	{

			ch = map_occupants[x][y]->sspecies->ch;
			colour = map_occupants[x][y]->sspecies->colour;

	}
	draw_ch(x,y,ch,colour);
}

bool los_exists(int x1, int y1, int x2, int y2)
{
    // if x1 == x2 or y1 == y2, then it does not matter what we set here
    int delta_x(x2 - x1);
    signed char ix((delta_x > 0) - (delta_x < 0));
    delta_x = std::abs(delta_x) << 1;
 
    int delta_y(y2 - y1);
    signed char iy((delta_y > 0) - (delta_y < 0));
    delta_y = std::abs(delta_y) << 1;
 
    if (delta_x >= delta_y)
    {
        // error may go below zero
        int error(delta_y - (delta_x >> 1));
 
        while (x1 != x2)
        {
            if (error >= 0)
            {
                if (error || (ix > 0))
                {
                    y1 += iy;
                    error -= delta_x;
                }
                // else do nothing
            }
            // else do nothing
 
            x1 += ix;
            error += delta_y;
			if (x1 != x2)//(x1 != x2 && y1 != y2)
			{
				if(debug)
				;//{draw_ch(x1,y1,'&',COLOR_WHITE); getch();draw_tile_debug(x1,y1);}
				if (is_opaque(x1, y1)) return false;
			}
        }
    }
    else
    {
        // error may go below zero
        int error(delta_x - (delta_y >> 1));
 
        while (y1 != y2)
        {
            if (error >= 0)
            {
                if (error || (iy > 0))
                {
                    x1 += ix;
                    error -= delta_y;
                }
                // else do nothing
            }
            // else do nothing
 
            y1 += iy;
            error += delta_x;
 
            if (y1 != y2)//(x1 != x2 && y1 != y2)
			{
				if (is_opaque(x1, y1)) return false;
			}
        }
    }
	return true;
}

bool symmetrical_los(int x1, int y1, int x2, int y2)
{
	return (los_exists(x1, y1, x2, y2) || los_exists(x2, y2, x1, y1));
}

void undraw_tile_description()
{
	for (int i=0;i<3;i++)
	mvaddstr(i,viewport_width,string(disp_columns-viewport_width,' ').c_str());
}

void draw_tile_description(int x, int y)
{
	if (!map_seen[x][y])
	{
		attron(COLOR_PAIR(COLOR_WHITE));
		mvprintw(0,viewport_width,"%d, %d, unseen",x,y);
	}
	else
	{
		attron(COLOR_PAIR(terrain_colours[map_terrain[x][y]]));
		mvaddstr(0,viewport_width,terrain_descriptors[map_terrain[x][y]]);
		if (map_items[x][y] != NULL)
		{
			attron(COLOR_PAIR(map_items[x][y]->colour));
			mvaddstr(1,viewport_width,("a "+map_items[x][y]->name).c_str());
		}
		if (map_occupants[x][y] != NULL)
		{
			if(symmetrical_los(p_ptr->x,p_ptr->y,x,y))
			{
				attron(COLOR_PAIR(map_occupants[x][y]->sspecies->colour));
				mvaddstr(2,viewport_width,map_occupants[x][y]->sspecies->name.c_str());
			}
		}
	}
}

void draw_tile(int x, int y)
{
	char ch = terrain_chars[map_terrain[x][y]];
	int colour = terrain_colours[map_terrain[x][y]];
	if (!map_seen[x][y])
	{draw_ch(x,y,' ',colour); return;}
	if(debug && !symmetrical_los(p_ptr->x,p_ptr->y,x,y))
	{draw_ch(x,y,' ',colour); return;}
	if (map_items[x][y] != NULL)
	{
		ch = map_items[x][y]->ch;
		colour = map_items[x][y]->colour;
	}
	if (map_occupants[x][y] != NULL)
	{
		if(symmetrical_los(p_ptr->x,p_ptr->y,x,y))
		{
			ch = map_occupants[x][y]->sspecies->ch;
			colour = map_occupants[x][y]->sspecies->colour;
		}
	}
	draw_ch(x,y,ch,colour);
}

void draw_line(int x1, int y1, int x2, int y2, char ch)
{
    // if x1 == x2 or y1 == y2, then it does not matter what we set here
    int delta_x(x2 - x1);
    signed char ix((delta_x > 0) - (delta_x < 0));
    delta_x = std::abs(delta_x) << 1;
 
    int delta_y(y2 - y1);
    signed char iy((delta_y > 0) - (delta_y < 0));
    delta_y = std::abs(delta_y) << 1;
	int col = COLOR_BLUE;
 
    if (delta_x >= delta_y)
    {
        // error may go below zero
        int error(delta_y - (delta_x >> 1));
 
        while (x1 != x2)
        {
            if (error >= 0)
            {
                if (error || (ix > 0))
                {
                    y1 += iy;
                    error -= delta_x;
                }
                // else do nothing
            }
            // else do nothing
 
            x1 += ix;
            error += delta_y;
			if (is_opaque(x1,y1)) {col = COLOR_RED;}
			draw_ch(x1,y1,ch,col);
        }
    }
    else
    {
        // error may go below zero
        int error(delta_x - (delta_y >> 1));
 
        while (y1 != y2)
        {
            if (error >= 0)
            {
                if (error || (iy > 0))
                {
                    x1 += ix;
                    error -= delta_y;
                }
                // else do nothing
            }
            // else do nothing
 
            y1 += iy;
            error += delta_x;
			if (is_opaque(x1,y1)) {col = COLOR_RED;}
			draw_ch(x1,y1,ch,col);
        }
    }
	move(y2,x2-1);
}
void undraw_line(int x1, int y1, int x2, int y2)
{
    // if x1 == x2 or y1 == y2, then it does not matter what we set here
    int delta_x(x2 - x1);
    signed char ix((delta_x > 0) - (delta_x < 0));
    delta_x = std::abs(delta_x) << 1;
 
    int delta_y(y2 - y1);
    signed char iy((delta_y > 0) - (delta_y < 0));
    delta_y = std::abs(delta_y) << 1;
 
    if (delta_x >= delta_y)
    {
        // error may go below zero
        int error(delta_y - (delta_x >> 1));
 
        while (x1 != x2)
        {
            if (error >= 0)
            {
                if (error || (ix > 0))
                {
                    y1 += iy;
                    error -= delta_x;
                }
                // else do nothing
            }
            // else do nothing
 
            x1 += ix;
            error += delta_y;
			draw_tile(x1,y1);
        }
    }
    else
    {
        // error may go below zero
        int error(delta_x - (delta_y >> 1));
 
        while (y1 != y2)
        {
            if (error >= 0)
            {
                if (error || (iy > 0))
                {
                    x1 += ix;
                    error -= delta_y;
                }
                // else do nothing
            }
            // else do nothing
 
            y1 += iy;
            error += delta_x;
			draw_tile(x1,y1);
        }
    }
	move(y2,x2);
}

void draw_last_msg()
{
	attron(COLOR_PAIR(COLOR_WHITE));
	mvaddstr(0,0,padmsg(rlmessages.back()).c_str());
}

void draw_info(int x,int y)
{
	for(int i = 0; y<10; y++)
	{
		mvaddstr(i,0,string(viewport_width,' ').c_str());
	}
	if(map_items[x][y] != NULL)
	mvprintw(0,0,map_items[x][y]->description.c_str());
	if(map_occupants[x][y] != NULL)
	mvprintw(0,0,map_occupants[x][y]->sspecies->description.c_str());
}

void draw()
{
	draw_last_msg();
	draw_stats();
	centre_x = min(p_ptr->x,MAP_SIZE - viewport_width/2);
	centre_x = max(centre_x,viewport_width/2);
	centre_y = min(p_ptr->y,MAP_SIZE - viewport_height/2);
	centre_y = max(centre_y,viewport_height/2);
	for(int x=viewport_left_edge();x<viewport_right_edge(); x++)
	{
		for(int y=viewport_top_edge();y<viewport_bottom_edge(); y++)
		{
			draw_tile(x,y);
		}
	}
	refresh();
}
bool terrain_immovable(actor* a, int x, int y)
{
	if (map_terrain[x][y] == WALL) return true;
	if (map_occupants[x][y] != NULL && map_occupants[x][y] != a) return true;
	return false;
}
int move_actor(actor* a, int dx, int dy)
{
	if (dx==0 && dy==0)
		return 1;
	int ox = a->x; int oy = a->y;
	int x = ox+dx; int y = oy+dy;
	if (terrain_immovable(a,x,y)) return true;
	map_occupants[ox][oy] = NULL;
	map_occupants[x][y] = a;
	a->x = x; a->y = y;
	return 0;
}

inline int dinf(int x,int y,int xx,int yy)
{
	return max(abs(x-xx),abs(y-yy));
}

inline int d1(int x,int y,int xx,int yy)
{
	return abs(x-xx)+abs(y-yy);
}

inline int d22(int x, int y, int xx, int yy)
{
	return (x-xx)*(x-xx)+(y-yy)*(y-yy);
}

inline bool in_vis_range(int x, int y, int xx, int yy)
{
	return d22(x,y,xx,yy) <= vis_range*vis_range;
}

void kill_actor(actor* a)
{
	map_occupants[a->x][a->y] = NULL;
	a->dead = true;
	add_message("The "+a->sspecies->name+" dies.");
	add_junk(a->sspecies->name +" corpse","the bloody corpse of a "+a->sspecies->name,'%',a->sspecies->colour,a->x,a->y);
}

void lose()
{
	add_message("You die. Press 'Q' to quit.");
	draw();
	int kp = 0;
	while(kp != 'Q') kp = getch();
	endwin();
	exit(0);

}

void hurt_actor(actor* a,int damage)
{
	a->health -= damage;
	if(a->health <= 0)
	{
		if (a->is_player) {lose();}
		else kill_actor(a);
	}
}

void melee_attack(actor* attacker, actor* defender)
{
	add_message("The "+attacker->sspecies->name+" claws you.");
	hurt_actor(defender,attacker->sspecies->damage);
}

void fire_weapon(int x1, int y1, int x2, int y2)
{
	add_message("BANG!");
	if(los_exists(x1,y1,x2,y2))
	{
		actor* victim = map_occupants[x2][y2];
		if(victim != NULL)
		{
			add_message("The "+victim->sspecies->name+" is hit!");
			int damage = 0;
			switch(current_weapon)
			{
			case PISTOL:
				damage = rand()%2 + 1;
				ammo[PISTOL] -= 1;
				break;
			case RIFLE:
				damage = rand()%4+1;
				ammo[RIFLE] -= 4;
				break;
			case CANNON:
				damage = rand()%4+5;
				ammo[CANNON] -= 1;
				break;
			}
			hurt_actor(victim,damage);
		}
	}
}

void random_walk(actor* a)
{
	pair<int,int> valid_dirs[8];
	int dir_count = 0;
	for(int d=0;d<8;d++)
	{
		pair<int,int> dir = directions[d];
		if(is_wall(a->x+dir.first,a->y+dir.second))
		{
			valid_dirs[dir_count] = dir;
			dir_count++;
		}
		int choice = rand()%dir_count;
		move_actor(a,valid_dirs[choice].first,valid_dirs[choice].second);

	}
}


void ai_turn(actor* a)
{
	if(dinf(a->x,a->y,p_ptr->x,p_ptr->y) == 1)
	{
		melee_attack(a,p_ptr);
		return;
	}
	bool uncertain = true;

	if(a->sspecies->psychic)
	{
		float d2 = sqrt((float)d22(a->x,a->y,p_ptr->x,p_ptr->y));
		if(d2<=psychic_0_range && randfloat() < 1 + (psychic_1_range - d2)*(psychic_1_range - d2) /
			((psychic_0_range - psychic_1_range)*(psychic_0_range - psychic_1_range)))
		{
			a->ai_x = p_ptr->x; a->ai_y = p_ptr->y;
			uncertain = false;
		}
	}
	if(uncertain && a->sspecies->sees && symmetrical_los(a->x,a->y,p_ptr->x,p_ptr->y))
	{
		a->ai_x = p_ptr->x; a->ai_y = p_ptr->y;
		uncertain = false;
	}
	if(uncertain && a->sspecies->hears && rand()%10 < 3)
	{
		a->ai_x = p_ptr->x; a->ai_y = p_ptr->y;
		uncertain = false;
	}
	if(a->x==a->ai_x && a->y ==a->ai_y)
	{
		random_walk(a); return;
	}
	int dx = (a->x) - (a->ai_x);
	int dy = (a->y) - (a->ai_y);

	int best = d1(a->x,a->y,p_ptr->x,p_ptr->y)+2;
	int bestdx = 0; int bestdy = 0;
	for(int d=0;d<8;d++)
	{
		pair<int,int> dir = directions[d];
		int ddx = dir.first; int ddy = dir.second;
		if (!terrain_immovable(a,a->x+ddx,a->y+ddy))
		{
			int dist = d1(a->x+ddx,a->y+ddy,p_ptr->x,p_ptr->y);
			if (dist < best)
			{best = dist; bestdx = ddx; bestdy = ddy;}
			else if (dist == best)
			{
				int ddist = dinf(a->x+ddx,a->y+ddy,p_ptr->x,p_ptr->y);
				int ddist2 = dinf(a->x+bestdx,a->y+bestdy,p_ptr->x,p_ptr->y);
				if (ddist < ddist2)
				{best = dist; bestdx = ddx; bestdy = ddy;}
			}
		}
	}
	move_actor(a,bestdx,bestdy);
}

void update_map_seen()
{
	int px = p_ptr->x;
	int py = p_ptr->y;
	for (int x=px - vis_range;x <= px + vis_range; x++)
	{
		for (int y=py - vis_range;y <= py + vis_range; y++)
		{
			if(on_map(x,y))
			{
				if(in_vis_range(x,y,px,py))
				{
					if (!map_seen[x][y])
					{
						map_seen[x][y] = symmetrical_los(x,y,px,py);
					}
				}
			}
		}
	}
}

string signal_strength(int x,int y)
{
	int d2 = d22(x,y,target_x,target_y);
	if((float)sqrt((float)d2) < 9+rand()%3)
		return "coming from very close by!";
	else if ((float)sqrt((float)d2) < 18 + rand()%6)
		return "very strong!";
	else if ((float)sqrt((float)d2) < 36 + rand()%12)
		return "quite strong.";
	else if ((float)sqrt((float)d2) < 72 + rand()%24)
		return "quite weak.";
	else
		return "very weak...";
}

void player_turn(actor* a)
{
	update_map_seen();
	draw();
	int turn = -1;
	while(turn)
	{
		int kp = getch();
		switch(kp)
		{
		case KEY_UP: case 'k':
			turn = move_actor(a,0,-1); break;
		case KEY_DOWN: case 'j':
			turn = move_actor(a,0,1); break;
		case KEY_LEFT: case 'h':
			turn = move_actor(a,-1,0); break;
		case KEY_RIGHT: case 'l':
			turn = move_actor(a,1,0); break;
		case 'y':
			turn = move_actor(a,-1,-1); break;
		case 'u':
			turn = move_actor(a,1,-1); break;
		case 'b':
			turn = move_actor(a,-1,1); break;
		case 'n':
			turn = move_actor(a,1,1); break;
		case '.':
			turn = 0; break;
		case 'X':
			if(current_weapon != PISTOL) {turn = 0; add_message("You wield your pistol."); current_weapon = PISTOL;} break;
		case 'Y':
			if(current_weapon != RIFLE) {turn = 0; add_message("You wield your automatic rifle!"); current_weapon = RIFLE;} break;
		case 'Z':
			if(current_weapon != CANNON) {turn = 0; add_message("You get out the plasma cannon!!"); current_weapon = CANNON;} break;
		case 'r':
			turn = 0; add_message("The radio signal is "+signal_strength(p_ptr->x,p_ptr->y)); draw_last_msg(); break;
		case 'd':
			debug = !debug; break;
		case 't':
			{
				bool throw_mode = true;
				int lx = p_ptr->x; int ly = p_ptr->y;
			while(throw_mode)
			{
				draw_tile_description(lx,ly);
				draw_line(p_ptr->x,p_ptr->y,lx,ly,'*');
				int kpp = getch();
				undraw_line(p_ptr->x,p_ptr->y,lx,ly);
				undraw_tile_description();
				switch(kpp)
				{
				case KEY_UP: case 'k':
					ly = max(ly-1,viewport_top_edge()); break;
				case KEY_DOWN: case 'j':
					ly = min(ly+1,viewport_bottom_edge()); break;
				case KEY_LEFT: case 'h':
					lx = max(lx-1,viewport_left_edge()); break;
				case KEY_RIGHT: case 'l':
					lx = min(lx+1,viewport_right_edge()); break;
				case 'y':
					lx = max(lx-1,viewport_left_edge()); ly = max(ly-1,viewport_top_edge()); break;
				case 'u':
					lx = min(lx+1,viewport_right_edge()); ly = max(ly-1,viewport_top_edge()); break;
				case 'b':
					lx = max(lx-1,viewport_left_edge()); ly = min(ly+1,viewport_bottom_edge()); break;
				case 'n':
					lx = min(lx+1,viewport_right_edge()); ly = min(ly+1,viewport_bottom_edge()); break;
				case 't':
					{

					}
					break;
				case KEY_ESC:
					 throw_mode = false; break;
				case 'i':
					draw_info(lx,ly); getch(); draw(); break;
				default: ;
				}
			}
			}
			break;
		case 'f':
		{
			bool fire_mode = true;
			int lx = p_ptr->x; int ly = p_ptr->y;
			while (fire_mode)
			{
				draw_tile_description(lx,ly);
				draw_line(p_ptr->x,p_ptr->y,lx,ly,'*');
				int kpp = getch();
				undraw_line(p_ptr->x,p_ptr->y,lx,ly);
				undraw_tile_description();
				switch(kpp)
				{
				case KEY_UP: case 'k':
					ly = max(ly-1,viewport_top_edge()); break;
				case KEY_DOWN: case 'j':
					ly = min(ly+1,viewport_bottom_edge()); break;
				case KEY_LEFT: case 'h':
					lx = max(lx-1,viewport_left_edge()); break;
				case KEY_RIGHT: case 'l':
					lx = min(lx+1,viewport_right_edge()); break;
				case 'y':
					lx = max(lx-1,viewport_left_edge()); ly = max(ly-1,viewport_top_edge()); break;
				case 'u':
					lx = min(lx+1,viewport_right_edge()); ly = max(ly-1,viewport_top_edge()); break;
				case 'b':
					lx = max(lx-1,viewport_left_edge()); ly = min(ly+1,viewport_bottom_edge()); break;
				case 'n':
					lx = min(lx+1,viewport_right_edge()); ly = min(ly+1,viewport_bottom_edge()); break;
				case 'f':
					if(ammo[current_weapon] == 0)
					{add_message("Can't fire this weapon, no ammo.");}
					else
					{
					fire_mode = false;
					fire_weapon(p_ptr->x,p_ptr->y,lx,ly);
					turn = 0;
					}
					break;
				case KEY_ESC:
					 fire_mode = false; break;
				case 'i':
					draw_info(lx,ly); getch(); draw(); break;
				default:;
				}
			}
			undraw_line(p_ptr->x,p_ptr->y,lx,ly);
			undraw_tile_description();
		}
		break;
			
		default: ;
		}
	}
}

void diffuse_scent_human()
{
	for (int x=0;x<MAP_SIZE;x++)
	{
		for(int y=0; y < MAP_SIZE;y++)
		{
			if(!is_wall(x,y))
			{
				spare_scent_map[x][y] = map_human_scent[x][y];
				int count = 1;
				for(int d=0;d<8;d++)
				{
					pair<int,int> dir = directions[d];
					int xx = x+dir.first; int yy = y+dir.second;
					if(!is_wall(xx,yy))
					{
						spare_scent_map[x][y] += map_human_scent[xx][yy];
						count++;
					}
				}
				map_human_scent[x][y] = 0.995f*spare_scent_map[x][y]/count;
			}
		}
	}
}

int play_turn()
{
		for(vector<actor*>::iterator a = actors.begin(); a != actors.end(); ++a)
		{
			if ((*a)->sspecies->name == "human" && !(*a)->is_player)
			{
				map_human_scent[(*a)->x][(*a)->y] += 0.7f; //non-player humans get an easier time >:)
			}
			if(!(*a)->dead)
			{
				if ((*a)->is_player)
				{
					player_turn(*a);
					map_human_scent[(*a)->x][(*a)->y] += 1.0;
					diffuse_scent_human();
				}
				else
				{
					ai_turn(*a);
				}
			}
		}
	return 0;
}


int main()
{
	srand((unsigned int)time(NULL));
	init();
	ESCDELAY=0;
	int end_game = 0;
	while(!end_game)
	{
		end_game = play_turn();
	}
	endwin();
}