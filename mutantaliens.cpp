#include <curses.h>
//#include <panel.h>
#include <random>
#include <time.h>
#include <vector>
#include <algorithm>
#include <string>
#include <math.h>
#include <array>

//these are from pdcurses
//#define KEY_A1        0x1c1  /* upper left on Virtual keypad */
#define KEY_A2        0x1c2  /* upper middle on Virt. keypad */
//#define KEY_A3        0x1c3  /* upper right on Vir. keypad */
#define KEY_B1        0x1c4  /* middle left on Virt. keypad */
//#define KEY_B2        0x1c5  /* center on Virt. keypad */
#define KEY_B3        0x1c6  /* middle right on Vir. keypad */
//#define KEY_C1        0x1c7  /* lower left on Virt. keypad */
#define KEY_C2        0x1c8  /* lower middle on Virt. keypad */
//#define KEY_C3        0x1c9  /* lower right on Vir. keypad */

#pragma warning(disable: 4996)
/*
Scoring:
Died: -1000
Left the ship: 50
Entered a building or destroyed the transmitter: 100
Destroyed the transmitter: 200
Entered the ship after destroying transmitter: 400
Closed the doors: 600
Left the planet: 800
Ran: 
100 per small explosive left
400 per large explosive left
50 per distraction left
5 per pistol bullet left
3 per rifle bullet left
25 per cannon shot left
-1 per second? per tick?
times difficulty level


*/

/*
allowable polish TODO:

if the last message happened between the player's last turn and now, colour it, or reverse??? Make message section separate, add message review command.
pick transmitter location according to difficulty?
ensure C to close doors thing in the status panel only appears when closing the doors is possible
disallow targetting and object placement outside map bounds
*/

/*
TODO for ARRP:

*rebalance distractions/monster senses to make them useful (but still not overpowered)
*completely rescale difficulty
*add tutorial
  - moving
  - killing small aliens
    - first one dies after two hits
	- second dies in two hits, second critical
	- third dies in one critical hit
  - killing medium aliens
    - first one comes out from behind a tree next to the player, use LE (and take splash damage)
	- second one comes out from further away, use cannon
  - killing huge aliens
    - run-and-gun with rifle for first one (clear some trees out) - discover weakness after a few shots
	- ambush by 2 huge aliens - kill with HE, hurt self
  - look for transmitter:
    - take several steps north, find no difference in signal strength
	- take steps west, get bad signal
	- go east, find building, tada
	- destroy with cannon or explosives
*allow different keymaps: 2 or 3 hardcoded ones (easy) or otherwise (hard)
*add cryo guy near transmitter whose AI strikes a balance between shooting aliens (if ahead of the player)
  and getting into the ship (if behind the player or has seen ship), and is capable of navigating round obstacles to get in.
  (surprise: very willing to take off without you)
*completely rescale difficulty
*add props relevant to cryo guy so the place doesn't seem dead
*make messages, alerts, and map borders behave without weirdness
*explain and write scores to file
*draw sound as red lights on the border of the map display?
*make carnivorous tentacle plants (ubiquitous galactic pest)?
*skill sets (soldier - more health and ammo, easier to hit known criticals
             sneak - when not running or shooting, orders of magnitude less noise and aliens only see you if you're in sight two turns in a row
			 meditating xenobiologist - psychics don't detect you, criticals are easier to discover
			 technician - all distractions more powerful, all devices interchangeable)


*/

/*
REAL TODO
make radio receiver get transmissions from ship after transmitter destroyed
countdown to doors closing, leave a burnt hole in the ground if player is too slow (relative to difficulty)
make graduated sensitivity to smell, make alternatives for smell generator, 
make blinding lights
make remotes
allow pickup, drop, hide, activate

*/
const float PI = 3.14159265f;

/* TODO
start with small aliens, or mention in tutorial.

prevent shooting, throwing onto wall tiles?
allow transparent impassable things?

make devices activateable, hideable, droppable, pickupable, remotable
make guns structs
make monsters react better to sound,
make monsters follow other monters to the player
make pack monsters try to maintain distance from player while a group assembles, before closing in
make detailed species descriptions
make buildings multilevel
*/

#define arrayend(ar)  ar + (sizeof(ar) / sizeof(ar[0]))

inline int squarei(int x) {return x*x;}
inline float squaref(float x) {return x*x;}
//using namespace std;

WINDOW* scratch;
bool forcefed_help = false;
int difficulty = 0;
const int KEY_ESC = 27;
bool transmitter_destroyed = false;
bool door_closable_told = false;
const int MAP_SIZE = 150;
//terrain types
const int DIRT = 0;
const int STREE = 1;
const int BTREE = 2;
const int FLOOR = 3;
const int WALL = 4;
const int DOOR = 5;
const int SW_NE = 6;
const int NW_SE = 7;
const int NOSE = 8;
const int CPANEL = 9;
const int NS = 10;
const int WE = 11;
const int TWINDOW = 12;
const int TERRAIN_TYPES = 13; //this should be 1 more than the highest terrain type
const char terrain_chars[] = {'.','t','T','.','#','+','/','\\','<','#','|','-','#'};
const int terrain_colours[] = {COLOR_GREEN,COLOR_GREEN,COLOR_GREEN,COLOR_WHITE,COLOR_WHITE,COLOR_WHITE,
COLOR_WHITE,COLOR_WHITE,COLOR_WHITE,COLOR_WHITE,COLOR_WHITE,COLOR_WHITE,COLOR_CYAN};
const char* terrain_descriptors[] = {"soil", "a small tree", "a big tree", "a paved floor", "a wall","a door","ship hull",
"ship hull","ship hull","control panel","ship hull","ship hull","window"};
const char* mode_text[] = {"walk mode", "fire mode", "throw mode", "info mode"};
const char* size_descriptors[] = {"smal", "medm", "huge"};
const char* damage_descriptors[] = {"weak","strg"};
bool debug = false;
int turn_count = 0;
const int WALK_MODE = 0;
const int FIRE_MODE = 1;
const int THROW_MODE = 2;
const int INFO_MODE = 3;
int current_mode = 0;
bool cycle_advice=true;
int current_advice=0;

bool died = false;
bool left_ship = false;
bool ran = false;
bool entered_building_destroyed_transmitter = false;
bool destroyed_transmitter = false;
bool returned_to_ship = false;
bool closed_doors = false;
bool left_planet = false;


const char* const advice[] = {
   /*                              */  
	"Press R to run!",
	"Press R to run!",
	"Flee often!",
	"Conserve your stamina!",
	"Yellow aliens: slow and strong",
	"Magenta aliens: fast and weak",
	"Cyan aliens: could be anything",
	"Press f to fire!",
	"Use your ammunition wisely!",
	"Press t to throw devices!",
	"Press t to throw devices!",
	"Press t to throw devices!",
	"Press t to throw explosives!",
	"Press t to throw explosives!",
	"Press t to throw distractions!",
	"Press t to throw distractions!",
	"Use your explosives wisely!",
	"Find the radio transmitter!",
	"Find the radio transmitter!",
	"Find the radio transmitter!",
	"Find the radio transmitter!",
	"Destroy the transmitter!",
	"Destroy the transmitter!",
	"Destroy the transmitter!",
	"Don't forget where the ship is",
	"You have explosives!",
	"You have explosives!",
	"You have explosives!",
	"You have explosives!",
	"You have explosives!",
	"Explosives destroy walls!",
	"Explosives destroy walls!",
	"Explosives destroy walls!",
	"Explosives destroy walls!",
	"Explosives destroy trees!",
	"Explosives destroy trees!",
	"The cannon makes explosions!",
	"The cannon makes explosions!",
	"The cannon makes explosions!",
	"The cannon can't destroy walls!",
	"The cannon can't destroy trees!",
	"Devices distract aliens",
	"Devices distract aliens",
	"Press X to use the pistol?",
	"Press Y to use the rifle!",
	"Press Z to use the cannon!!",
	"Press Z to use the cannon!!",
	"How close is the transmitter?",
	"How close is the transmitter?",
	"How close is the transmitter?",
	"Press r to check for signal!",
	"Press r to check for signal!",
	"Press r to check for signal!",
	"Press r to check for signal!",
	"Can't see through big trees!",
	"Be careful of ambushes!",
	"Trees have ears!",
	"Trees have noses!",
	"Trees have psychic powers!",
	"Consult the memory often!",
	"Aliens don't carry ammo!",
	"Aliens don't carry medkits!",
	"Holograms distract aliens!",
	"Holograms distract aliens!",
	"Noise gen.s distract aliens!",
	"Scent gen.s distract aliens!",
	"Brain slices distract psychics",
	"High expl.s kill huge aliens!",
	"Low stamina means running slow",
	"Low stamina means running slow"
	
};
std::vector<std::string> vadvice(advice,arrayend(advice));

const int WAVELENGTHS = 5;
int map_terrain[MAP_SIZE][MAP_SIZE];
float map_human_scent[MAP_SIZE][MAP_SIZE];
float spare_scent_map[MAP_SIZE][MAP_SIZE];
bool map_seen[MAP_SIZE][MAP_SIZE];
bool map_access[MAP_SIZE][MAP_SIZE];

//device states. represent native life with -species code.
const int NULL_STIMULUS = 0;
const int HUMAN_STIMULUS = 1;
const int LOUD_STIMULUS = 2;
const int LOUD2_STIMULUS = 3;

const int NOT_DEVICE = -1;
const int LOW_EXPLOSIVE = 0;
const int HIGH_EXPLOSIVE = 1;
const int HOLOGRAM_PROJECTOR = 2;
const int NOISE_GENERATOR = 3;
const int SCENT_GENERATOR = 4;
const int BRAIN_SLICE = 5;
const int DEVICE_TYPES = 6;

const int UNCONFIGURED = 0;
const int ON_TIMER = 1;
const int ON_REMOTE = 2;
const int DEVICE_CONFIGURATIONS = 3;
const int MAX_IDENTICAL_DEVICES = 10;

const int ZOO_SIZE = 7;

const char* const config_names[] = {"unconfigured", "on a timer", "remote controlled"};
std::vector<std::string> vconfig_names(config_names,config_names+DEVICE_CONFIGURATIONS);
template<class T> void kill_velement(std::vector<T>& vec, T elem)
{
	vec.erase(std::remove(vec.begin(), vec.end(), elem), vec.end());

}

//PANEL *map_display;
const int disp_lines = 25;
const int disp_columns = 80;
const int viewport_height = 19;
const int message_area_height = 6;
const int viewport_width = 49;
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
int current_weapon = RIFLE;

const int BUILDING_SIZE = 17;
int target_x;
int target_y;
int ship_x;
int ship_y;

const char* const transmission[] = {"Please enjoy your stay!",
	"Toilets are situated on every floor!",
	"Please visit again!",
	"Debarnacle your ship for free at the drydock... just kidding!",
	"The art gallery ship IDF Fuseli is currently docked at Bay 12. Visiting charge is 69,125 Zorkmids.",
	"Please remember to take all your luggage and personal belongings with you when leaving the spaceport.",
	"... are you there? Can you help me?",
	"Isn't it such a beautiful day? I love it when that nebula does that thing with the sun... Hey, why not buy some stuff?",
	"If you want to buy some stuff, why not get lost in the mall for a few days?"
	//"testabcdefghijklmnopqrstuvwxyzaabbccddeeffgghhiijjkkllmmnnooppqqrrssttuuvvwwxxyyzztteesstttestabcdefghijklmnopqrstuvwxyzaabbccddeeffgghhiijjkkllmmnnooppqqrrssttuuvvwwxxyyzztteesstt"
};
std::vector<std::string> vtransmission(transmission,arrayend(transmission));

inline int dinf(int x,int y,int xx,int yy)
{
	return std::max(abs(x-xx),abs(y-yy));
}

inline int d1(int x,int y,int xx,int yy)
{
	return abs(x-xx)+abs(y-yy);
}

int d22(int x, int y, int xx, int yy)
{
	return (x-xx)*(x-xx)+(y-yy)*(y-yy);
}
inline int fgetch()
{
	int kp = getch(); flushinp(); return kp;
}

const std::pair<int,int> directions[] = { std::make_pair(1,0), std::make_pair(1,1), std::make_pair(0,1), std::make_pair(-1,1),
									 std::make_pair(-1,0), std::make_pair(-1,-1), std::make_pair(0,-1), std::make_pair(1,-1)};
std::vector<std::pair<int,int>> vdirections(directions,directions+8);
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

std::vector<std::string> rlmessages;
std::vector<std::string> rlmessage_words;
int last_message_seen=0;
int last_word_seen = 0;
inline std::string padmsg(std::string msg)
{
	return msg;// + std::string(viewport_width-msg.length(),' ');
}
void add_message(std::string msg)
{
	//if(msg.length() > viewport_width) return 1;
	rlmessages.push_back(msg);
	std::string word_so_far = "";
	for(unsigned int i=0; i<msg.length();i++)
	{
		if(msg[i] == ' ' || msg[i] == '\t' || msg[i] == '\n')
		{
			rlmessage_words.push_back(word_so_far);
			word_so_far = "";
		}
		else
		{
			word_so_far = word_so_far + msg[i];
		}
	}
	rlmessage_words.push_back(word_so_far);
	//return 0;
}

void pretty_print_message_words()
{
	attron(COLOR_PAIR(COLOR_WHITE));
	int line_number = message_area_height-1;
	std::string line_so_far = "";
	int word_number = rlmessage_words.size()-1;
	int position = -1;
	std::string word;
	bool boundary_on_this_line = false;
	int boundary_position = -1;
	bool already_seen = (last_word_seen == rlmessage_words.size()-1);
	while(line_number >= 0 && word_number >= 0)
	{
		bool next_line = false;
		bool next_word = false;
		if(position == -1) // start a new word
		{
			word = rlmessage_words[word_number];
			position = word.length();
		}
		int line_chars_remaining = viewport_width - 1 - line_so_far.length();
		if(line_chars_remaining >= position+1) // whole remainder of word fits
		{
			line_so_far = word.substr(0,position) + ' ' + line_so_far;
			next_word = true;
		}
		else if(position >= viewport_width-1) // remainder of word couldn't possibly fit on an empty line, or exactly fits
		{
			line_so_far = word.substr(position-line_chars_remaining,line_chars_remaining) + ' ' + line_so_far;
			position -= line_chars_remaining;
			if(position == 0)
			{
				next_word = true;
			}
			next_line = true;
		}
		else // remainder of word would not fit on this line but would fit on an empty line
		{
			next_line = true;
		}
		if(next_word)
		{
			position = -1;
			word_number--;
			if(word_number == last_word_seen && last_word_seen != rlmessage_words.size()-1)
			{
				boundary_on_this_line = true;
				boundary_position = line_so_far.length();
				already_seen = true;
			}
		}


		if(next_line)
		{
			if(!already_seen)
			{
				attron(A_BOLD);
				mvaddstr(line_number,0,line_so_far.c_str());
				attroff(A_BOLD);
			}
			else
			{
				if(boundary_on_this_line)
				{
					attroff(A_BOLD);
					mvaddstr(line_number,0,line_so_far.substr(0,line_so_far.length()-boundary_position).c_str());
					attron(A_BOLD);
					mvaddstr(line_number,line_so_far.length()-boundary_position,line_so_far.substr(line_so_far.length()-boundary_position,boundary_position).c_str());
					attroff(A_BOLD);
					boundary_on_this_line = false;
				}
				else
				{
					attroff(A_BOLD);
					mvaddstr(line_number,0,line_so_far.c_str());
				}
			}
			line_number--;
			line_so_far = "";
		}
		
	}
	{
		if(!already_seen)
		{
			attron(A_BOLD);
			mvaddstr(line_number,0,line_so_far.c_str());
			attroff(A_BOLD);
		}
		else
		{
			if(boundary_on_this_line)
			{
				attroff(A_BOLD);
				mvaddstr(line_number,0,line_so_far.substr(0,line_so_far.length()-boundary_position).c_str());
				attron(A_BOLD);
				mvaddstr(line_number,line_so_far.length()-boundary_position,line_so_far.substr(line_so_far.length()-boundary_position,boundary_position).c_str());
				attroff(A_BOLD);
				boundary_on_this_line = false;
			}
			else
			{
				attroff(A_BOLD);
				mvaddstr(line_number,0,line_so_far.c_str());
			}
		}
		line_number--;
		line_so_far = "";
	}
	//last_word_seen  = rlmessage_words.size()-1;
	//if(last_word_seen < (int)rlmessage_words.size())
	//	attron(A_BOLD);
	//unsigned int colour_change_offset = 2*viewport_width;
	//int word_number = rlmessage_words.size()-1;
	//int word_position = -1;
	//for(int line = message_area_height-1; line >= 0; line--)
	//{
	//	std::string line_to_print;
	//	for(line_to_print = "";
	//		word_number >= 0 && (line_to_print.length() + rlmessage_words[word_number].length() < viewport_width
	//		                 || (word_position != -1));
	//		word_number--)
	//	{
	//		if(word_position == -1)
	//		{
	//			if((int)rlmessage_words[word_number].length() > viewport_width-1)
	//			{
	//				line_to_print = rlmessage_words[word_number].substr(rlmessage_words[word_number].length()-viewport_width+1+line_to_print.length(),viewport_width-line_to_print.length()-1)+' '+line_to_print;
	//				word_number++;
	//				word_position = rlmessage_words[word_number].length()-viewport_width+1+line_to_print.length();
	//			}
	//			else 
	//			{
	//				line_to_print = rlmessage_words[word_number] + ' ' + line_to_print;
	//				if(word_number == last_word_seen)
	//				{
	//					colour_change_offset = line_to_print.length();
	//				}
	//			}
	//		}
	//		else
	//		{
	//			if(word_position > viewport_width-1)
	//			{
	//				line_to_print = rlmessage_words[word_number].substr(word_position - viewport_width+1,viewport_width-1);
	//				word_number++;
	//				word_position = word_position - viewport_width+1;
	//			}
	//			else
	//			{
	//				line_to_print = rlmessage_words[word_number].substr(0,word_position);
	//				word_position = -1;
	//			}
	//		}
	//	}
	//	if(colour_change_offset != 2*viewport_width)
	//	{
	//		attroff(A_BOLD);
	//		mvaddstr(line,0,line_to_print.substr(0,line_to_print.length()-colour_change_offset).c_str());
	//		attron(A_BOLD);
	//		mvaddstr(line,line_to_print.length()-colour_change_offset,line_to_print.substr(line_to_print.length()-colour_change_offset).c_str());
	//		attroff(A_BOLD);
	//		colour_change_offset = 2*viewport_width;
	//	}
	//	else
	//	{
	//		mvaddstr(line,0,line_to_print.c_str());
	//	}
	//}
	////last_word_seen  = rlmessage_words.size();
	//attroff(A_BOLD);
}

void pretty_print(std::string s)
{
	attron(COLOR_PAIR(COLOR_WHITE));
	int current_line = 0;
	std::string line_so_far = "";
	std::string word_so_far = "";
	for(unsigned int i = 0; i<s.length();i++)
	{
		
		if(i == s.length()-1)
		{
			word_so_far += s[i];
			if(line_so_far.length() + word_so_far.length()+1 > viewport_width)
			{
				mvaddstr(current_line,0,(line_so_far+std::string(viewport_width-line_so_far.length(),' ')).c_str());
				mvaddstr(current_line+1,0,(word_so_far+std::string(viewport_width-word_so_far.length(),' ')).c_str());
				return;
			}
			else
			{
				line_so_far += " " + word_so_far;
				mvaddstr(current_line,0,(line_so_far+std::string(viewport_width-line_so_far.length(),' ')).c_str());
				return;
			}
		}
		else if(word_so_far.length() == viewport_width-1)
		{
			if(line_so_far.length())
			{
			mvaddstr(current_line,0,(line_so_far+std::string(viewport_width-line_so_far.length(),' ')).c_str());
			current_line++;
			line_so_far = "";
			}
			mvaddstr(current_line,0,(word_so_far + s[i]).c_str());
			current_line++;
			word_so_far = "";
		}
		else if(s[i] == ' ')
		{
			if(line_so_far.length() + word_so_far.length()+1 > viewport_width)
			{
				mvaddstr(current_line,0,(line_so_far+std::string(viewport_width-line_so_far.length(),' ')).c_str());
				line_so_far = word_so_far;
				word_so_far = "";
				current_line++;
			}
			else
			{
				line_so_far += " " + word_so_far;
				word_so_far = "";
			}
		}
		else if(s[i] == '\n')
		{
			if(line_so_far.length() + word_so_far.length() +1> viewport_width)
			{
				mvaddstr(current_line,0,(line_so_far+std::string(viewport_width-line_so_far.length(),' ')).c_str());
				line_so_far = word_so_far;
				word_so_far = "";
			}
			else
			{
				line_so_far += word_so_far;
				word_so_far = "";
				mvaddstr(current_line,0,(line_so_far+std::string(viewport_width-line_so_far.length(),' ')).c_str());
			}
			current_line++;
		}
		else
		{
			word_so_far += s[i];
		}
	}
}

void draw_text_wodge(std::string s)
{
	int lines = (int)ceil(((float)s.length())/viewport_width);
	attron(COLOR_PAIR(COLOR_WHITE));
	mvaddstr(lines-1,0,std::string(viewport_width,' ').c_str());
	for(int i=0;i<lines;i++)
	{
		mvaddstr(i,0,(s.substr(viewport_width*i,viewport_width).c_str()));
	}
	//getch();
}

void clear_area(int x,int y,int xr,int yr)
{
	for(int i=y;i<y+yr;i++)
		mvaddstr(i,x,std::string(xr,' ').c_str());
}

void draw_last_msg()
{
	//attron(COLOR_PAIR(COLOR_WHITE));
	clear_area(0,0,viewport_width,message_area_height+1);
	pretty_print_message_words();
	/*std::string s = "";
	for(unsigned int i = last_message_seen; i < rlmessages.size(); i++)
	{
		s += rlmessages[i]; s += " ";
	}
	pretty_print(s);
	last_message_seen = std::max(last_message_seen,(int)rlmessages.size()-2);*/
	last_word_seen  = (int)rlmessage_words.size()-1;
	//mvaddstr(0,0,padmsg(rlmessages.back()).c_str());
}

struct noise
{
	float volume;
	std::string describe;
	bool scary;
};
noise LE_noise = {25.0,"explosion",true};
noise HE_noise = {31.0,"big explosion",true};
noise screech = {15.0f,"screech",false};
std::vector<std::vector<noise>> walk_noise(ZOO_SIZE+1,std::vector<noise>(TERRAIN_TYPES));
std::vector<std::vector<noise>> run_noise(ZOO_SIZE+1,std::vector<noise>(TERRAIN_TYPES));
noise fake_human_noise[2] = {{10.0f,"\"HELLO?\"",false},{10.0,"\"I'M MADE OF FOOD!\"",false}};
noise generator_loud[2] = {{21.0f,"ROAR OF THUNDER",true},{21.0,"MERZBOW ON CRACK",true}};

struct species
{
	int id;
	int min_health, health_range;
	int damage;
	int vis_range;
	float smell_clarity;
	float hearing_thres;
	float hearing_adapt;
	int psychic_range;
	char ch;
	int colour;
	int walk_energy;
	int run_energy;
	int stamina;
	int size;
	int body_sections;
	int body_sections_tried;
	bool crit_known;
	bool damage_known;
	bool health_known;
	bool walk_known;
	bool run_known;
	bool size_known;
	noise still_noise;
	noise melee_noise;
	std::string syl1, syl2, name;
	std::string description;
	std::string crit_part_name;
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
	return centre_y-viewport_height/2+1;
}
inline int viewport_bottom_edge()
{
	return centre_y+viewport_height/2+1;
}

std::vector<species*> zoo;

species* average[3];
species* human_sp;

const char* const clusters_1[] = {"b","c","d","f","g","h","j","k","l","m","n","p","r","s","t","v","w","x","y","z"};
std::vector<std::string> vclusters_1(clusters_1,arrayend(clusters_1));
const char* const start_clusters_2[] = {"bh","bl","br","bw","by","bz","ch","cj","cl","cn","cr","cw","dh","dj","dl","dr",
	"dv","dw","dy","dz","fd","fj","fk","fl","fm","fn","fp","ft","fw","fy","gh","gj","gl","gn","gr","gv","gw",
	"gy","gz","hj","hl","hr","hy","hw","jh","jl","jn","jy","kf","kh","kj","kl","kn","kr","ks","kv","kw","ky",
	"kz","lh","lj","ll","lw","ly","mb","mf","mh","ml","mn","mp","mr","mw","my","nd","ng","nh","nj","nk","nr",
	"nt","nw","ny","pf","ph","pj","pl","pr","ps","pt","pw","py","qu","qv","qw","rh","rj","rw","ry","sc","sf",
	"sh","sj","sk","sl","sm","sn","sp","sr","st","sv","sw","sy","sz","th","tj","tl","tr","ts","tw","ty","tz",
	"vh","vj","vk","vl","vm","vn","vp","vr","vt","vw","vy","vz","wh","wr","wy","zb","zc","zd","zf","zg","zh",
	"zj","zk","zl","zm","zn","zp","zr","zs","zt","zv","zw","zy","zz"};
std::vector<std::string> vstart_clusters_2(start_clusters_2,arrayend(start_clusters_2));
const char* const start_clusters_3[] = {"chl","chr","scl","scr","skl","skr","spl","spr","str","str"};
std::vector<std::string> vstart_clusters_3(start_clusters_3,arrayend(start_clusters_3));
const char* const pure_vowels[] = {"a","aa","e","ee","i","oo","u","uu"};
std::vector<std::string> vpure_vowels(pure_vowels,arrayend(pure_vowels));
const char* const diphthongs[] = {"ae","ai","ao","au","aw","ay","ea","ei","eo","eu","ew","ey","ia","ie","io","iu","oa",
	"oe","oi","ou","ow","oy","ua","ue","ui","uo","uy","uw"};
std::vector<std::string> vdiphthongs(diphthongs,arrayend(diphthongs));
const char* const end_clusters_2[] = {"bb","bh","bl","br","bz","cc","ch","ck","cl","cr","cs","cr","cz","dd","dh",
	"dj","dl","dr","dt","dz","ff","fh","fl","fp","ft","fs","ft","gg","gh","gl","gr","gz","jj","kh","kk","kl",
	"kr","ks","kt","lb","lc","ld","lf","lg","lh","lj","lk","ll","lm","ln","lp","ls","lt","lv","lx","lz","mb",
	"mf","mh","mm","mp","ms","mz","nc","nd","nf","ng","nh","nj","nk","nn","ns","nt","nx","nz","pl","pp","pr",
	"ps","sh","sp","ss","st","vv","vh","vz","xx","zb","zd","zg","zh","zj","zl","zz"};
std::vector<std::string> vend_clusters_2(end_clusters_2,arrayend(end_clusters_2));
//const std::string end_clusters_3[] = {""};
std::string random_start_cluster()
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
std::string random_vowel()
{
	switch (rand()%5)
	{
	case 0: case 1: case 2:
		return pure_vowels[rand()%vpure_vowels.size()];
	default:
		return diphthongs[rand()%vdiphthongs.size()];
	}
}
std::string random_end_cluster()
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
std::string random_syllable()
{
	std::string syl = "";
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

void init_human_sp()
{
		species* human = new species();
	human->id = 0;
	human->health_range = 0; human->min_health = 100;
	human->ch = '@';
	human->psychic_range=0; human->hearing_thres = 1.0f;
	human->hearing_adapt = 0.5f;
	human->smell_clarity = 0.1f; human->vis_range = 8;
	human->syl1 = "hu"; human->syl2 = "man";
	human->name = "human";
	human->colour = COLOR_WHITE;
	human->damage = 1;
	human->walk_energy = 10;
	human->run_energy = 19;
	human->stamina = 1200;
	human->body_sections = 7;
	human->body_sections_tried = human->body_sections - 1;
	human->crit_part_name = "head";
	human->crit_known = true;
	human->size = 1;
	
	
	human->description = "A legendary advanced alien race whose technology is built specifically for murder, ecological destruction, deception, and gentle shaking. Members of this species must be killed immediately upon discovery to prevent catastrophic damage to the whole area.";
	zoo.push_back(human);
	human_sp = human;
}
void init_tutorial_species()
{
	init_human_sp();
	noise wn[TERRAIN_TYPES] = {{1.0,"footsteps",false},{1.5,"footsteps",false},{1.5f,"footsteps",false},{2.0,"footsteps",false},{0.0,"",false},
	{2.3f,"footsteps",false},{0.0,"",false},{0.0,"",false},{0.0,"",false},{0.0,"",false},{0.0,"",false},{0.0,"",false}};
	walk_noise[0] = std::vector<noise>(wn,wn+TERRAIN_TYPES);
	//delete [] wn;
	noise rn[TERRAIN_TYPES] = {{2.0,"footsteps",false},{3.0,"footsteps",false},{3.5f,"footsteps",false},{3.0,"footsteps",false},{0.0,"",false},
	{3.3f,"footsteps",false},{0.0,"",false},{0.0,"",false},{0.0,"",false},{0.0,"",false},{0.0,"",false},{0.0,"",false}};
	run_noise[0] = std::vector<noise>(rn,rn+TERRAIN_TYPES);
	//delete [] rn;
	{
		species* alien = new species();
		alien->damage_known = true;
		alien->health_known = true;
		alien->size_known = true;
		alien->walk_known = true;
		alien->run_known = true;
		alien->id = 1;
		alien->crit_known = false;
		alien->body_sections_tried = 0;
		alien->size = 0;
		alien->syl1 = "small";
		alien->syl2 = "alien";
		alien->ch = 's';
		alien->name = "small alien";
		alien->description = "A small, fast, weak, frail alien. We stole this specimen from a zoo on Mars. Actually we stole its great-great-grandparent, three days ago, and let it breed.";
		alien->colour = COLOR_MAGENTA;
		alien->crit_part_name = "splanchlet";
		alien->damage = 2;
		alien->min_health = 6;
		alien->health_range = 0;
		alien->body_sections = 4;
		alien->psychic_range = 0;
		alien->vis_range = 6;
		alien->hearing_thres = 1.0f;
		alien->hearing_adapt = 0.3f;
		alien->smell_clarity = 0.15f;

		alien->walk_energy = 13;
		alien->run_energy = 21;
		alien->stamina = 41*alien->run_energy;

		std::string walk_str = "scampering";
		std::string run_str = "scampering";
		float extra_noise = 7.0f;
		walk_noise[1] = std::vector<noise>(wn,wn+TERRAIN_TYPES);
		run_noise[1] = std::vector<noise>(rn,rn+TERRAIN_TYPES);
		for(int i=0;i<TERRAIN_TYPES;i++)
		{
			walk_noise[1][i].describe = walk_str;
			walk_noise[1][i].volume +=extra_noise;
			run_noise[1][i].describe = run_str;
			run_noise[1][i].volume +=extra_noise*1.6f;
		}
		zoo.push_back(alien);
	}
	{
		species* alien = new species();
		alien->damage_known = true;
		alien->health_known = true;
		alien->size_known = true;
		alien->walk_known = true;
		alien->run_known = true;
		alien->id = 2;
		alien->crit_known = false;
		alien->body_sections_tried = 0;
		alien->size = 0;
		alien->syl1 = "medium";
		alien->syl2 = "alien";
		alien->ch = 's';
		alien->name = "medium alien";
		alien->description = "It's about your size, and it walks and runs at about your speed. It can take several hits before it stops chasing you. It doesn't screw around. ";
		alien->colour = COLOR_CYAN;
		alien->crit_part_name = "splanch";
		alien->damage = 6;
		alien->min_health = 14;
		alien->health_range = 0;
		alien->body_sections = 7;
		alien->psychic_range = 10;
		alien->vis_range = 8;
		alien->hearing_thres = 8.0f;
		alien->hearing_adapt = 0.4f;
		alien->smell_clarity = 0.1f*(randfloat()+1.0f);

		alien->walk_energy = 10;
		alien->run_energy = 19;
		alien->stamina = 17*alien->run_energy;

		std::string walk_str = "trotting";
		std::string run_str = "galloping";
		float extra_noise = 9.0f;
		walk_noise[2] = std::vector<noise>(wn,wn+TERRAIN_TYPES);
		run_noise[2] = std::vector<noise>(rn,rn+TERRAIN_TYPES);
		for(int i=0;i<TERRAIN_TYPES;i++)
		{
			walk_noise[2][i].describe = walk_str;
			walk_noise[2][i].volume +=extra_noise;
			run_noise[2][i].describe = run_str;
			run_noise[2][i].volume +=extra_noise*1.6f;
		}
		zoo.push_back(alien);
	}
	{
		species* alien = new species();
		alien->damage_known = true;
		alien->health_known = true;
		alien->size_known = true;
		alien->walk_known = true;
		alien->run_known = true;
		alien->id = 1;
		alien->crit_known = false;
		alien->body_sections_tried = 0;
		alien->size = 0;
		alien->syl1 = "huge";
		alien->syl2 = "alien";
		alien->ch = 's';
		alien->name = "huge alien";
		alien->description = "If you took a mammoth on steroids and rearranged its bodyparts, maybe you'd end up with something like this alien's head. You don't want to let this get near you. Fortunately, it moves very slowly.";
		alien->colour = COLOR_YELLOW;
		alien->crit_part_name = "splanchum";
		alien->damage = 18;
		alien->min_health = 26;
		alien->health_range = 0;
		alien->body_sections = 12;
		alien->psychic_range = 24;
		alien->vis_range = 4;
		alien->hearing_thres = 11.0f;
		alien->hearing_adapt = 0.3f;
		alien->smell_clarity = 0.1f*(randfloat()+1.0f);

		alien->walk_energy = 6;
		alien->run_energy = 11;
		alien->stamina = 41*alien->run_energy;

		std::string walk_str = "thumping";
		std::string run_str = "charging";
		float extra_noise = 11.5f;
		walk_noise[3] = std::vector<noise>(wn,wn+TERRAIN_TYPES);
		run_noise[3] = std::vector<noise>(rn,rn+TERRAIN_TYPES);
		for(int i=0;i<TERRAIN_TYPES;i++)
		{
			walk_noise[3][i].describe = walk_str;
			walk_noise[3][i].volume +=extra_noise;
			run_noise[3][i].describe = run_str;
			run_noise[3][i].volume +=extra_noise*1.6f;
		}
		zoo.push_back(alien);
	}
	for(int i=4; i<ZOO_SIZE+1; i++)
	{
		species* alien = new species();
		alien->damage_known = false;
		alien->health_known = false;
		alien->size_known = false;
		alien->walk_known = false;
		alien->run_known = false;
		alien->id = i;
		alien->crit_known = false;
		alien->ch = ' ';
	}
}

void init_species()
{
	init_human_sp();
	noise wn[TERRAIN_TYPES] = {{1.0,"footsteps",false},{1.5,"footsteps",false},{1.5f,"footsteps",false},{2.0,"footsteps",false},{0.0,"",false},
	{2.3f,"footsteps",false},{0.0,"",false},{0.0,"",false},{0.0,"",false},{0.0,"",false},{0.0,"",false},{0.0,"",false}};
	walk_noise[0] = std::vector<noise>(wn,wn+TERRAIN_TYPES);
	//delete [] wn;
	noise rn[TERRAIN_TYPES] = {{2.0,"footsteps",false},{3.0,"footsteps",false},{3.5f,"footsteps",false},{3.0,"footsteps",false},{0.0,"",false},
	{3.3f,"footsteps",false},{0.0,"",false},{0.0,"",false},{0.0,"",false},{0.0,"",false},{0.0,"",false},{0.0,"",false}};
	run_noise[0] = std::vector<noise>(rn,rn+TERRAIN_TYPES);
	//delete [] rn;
	for (int spsp=1; spsp<ZOO_SIZE+1; spsp++)
	{
		species* alien = new species();
		alien->damage_known = false;
		alien->health_known = false;
		alien->size_known = false;
		alien->walk_known = false;
		alien->run_known = false;
		alien->id = spsp;
		alien->crit_known = false;
		alien->body_sections_tried = 0;
		int size = rand()%3;
		if(spsp == ZOO_SIZE) size = 2;
		else if(spsp==ZOO_SIZE - 1) size = 1;
		else if(spsp==ZOO_SIZE - 2) size = 0;
		alien->syl1 = random_syllable();
		while(true)
		{
			bool ok_name = true;
			for (int spspsp = 1; spspsp < spsp; spspsp++)
			{
				if(zoo[spspsp]->ch == alien->syl1[0])
				{
					ok_name = false;
					alien->syl1 = random_syllable();
				}
			}
			if(ok_name) break;
		}
		alien->syl2 = random_syllable();
		alien->name = alien->syl1+"-"+alien->syl2;
		alien->ch = alien->name[0];
		/*alien->colour = rand()%6+1;
		while(alien->colour == COLOR_GREEN)
		{
			alien->colour = rand()%6+1;
		}*/
		if(size == 0) alien->colour = COLOR_MAGENTA;
		if(size == 1) alien->colour = COLOR_CYAN;
		if(size == 2) alien->colour = COLOR_YELLOW;

		alien->size = size;
		std::string walk_str;
		std::string run_str;
		float extra_noise;
		alien->crit_part_name = random_syllable();
		switch(size)
		{
		case 0:
			alien->description = "A small alien.";
			alien->damage = rand()%2+2;
			alien->min_health = 4+rand()%4;
			alien->health_range = rand()%2+1;
			alien->body_sections = 3+rand()%4;

			if(rand()%12<1+difficulty/3)
			{ alien->psychic_range = rand()%20+10; }
			else
			{
				alien->psychic_range = 0;
			}

			switch(rand()%7)
			{
			case 0:
				alien->vis_range = rand()%6+6; break;
			case 1: case 2: case 3:
				alien->vis_range = rand()%4+4; break;
			case 4: case 5: case 6:
				alien->vis_range = rand()%3+3; break;
			}

			if(rand()%7 < 2){
				alien->hearing_thres = 1.0f+((float)(rand()%10))/3;
				alien->hearing_adapt = 0.3f;
			}
			else
			{
				alien->hearing_thres = -4.0f+(float)(rand()%300)*0.01f;
				alien->hearing_adapt = 1.5f;
			}

			switch(rand()%7)
			{
			case 0: case 1:
				alien->smell_clarity = (float)(rand()%4) +8.0f; break;
			case 2: case 3: case 4:
				alien->smell_clarity = randfloat()+1.0f; break;
			case 5: case 6:
				alien->smell_clarity = 0.1f*(randfloat()+1.0f); break;
			}

			alien->walk_energy = rand()%7+9;
			alien->run_energy = (3*alien->walk_energy)/2 + rand()%3;
			alien->stamina = 5*((rand()%4+6)*alien->run_energy + rand()%(2*alien->run_energy));

			switch(rand()%3)
			{
			case 0:
				walk_str = "scuttling"; break;
			case 1:
				walk_str = "scampering"; break;
			case 2:
				walk_str = "pattering"; break;
			}
			switch(rand()%3)
			{
			case 0:
				run_str = "scuttling"; break;
			case 1:
				run_str = "scampering"; break;
			case 2:
				run_str = "fluttering";
			}
			extra_noise = 7.0f;
			walk_noise[spsp] = std::vector<noise>(wn,wn+TERRAIN_TYPES);
			run_noise[spsp] = std::vector<noise>(rn,rn+TERRAIN_TYPES);
			for(int i=0;i<TERRAIN_TYPES;i++)
			{
				walk_noise[spsp][i].describe = walk_str;
				walk_noise[spsp][i].volume +=extra_noise;
				run_noise[spsp][i].describe = run_str;
				run_noise[spsp][i].volume +=extra_noise*1.6f;
			}
			break;
		case 1:
			alien->description = "A medium-sized alien.";
			alien->damage = rand()%6+4;
			alien->min_health = 10+rand()%6;
			alien->health_range = rand()%4+4;
			alien->body_sections = 6+rand()%8;

			if(rand()%12<2+(difficulty-3)/3)
			{ alien->psychic_range = rand()%30+20; }
			else
			{
				alien->psychic_range = 0;
			}

			switch(rand()%7)
			{
			case 0: case 1: case 2:
				alien->vis_range = rand()%6+6; break;
			case 3: case 4: case 5:
				alien->vis_range = rand()%4+4; break;
			case 6:
				alien->vis_range = rand()%2+2; break;
			}

			if(rand()%7 < 3)
			{
				alien->hearing_thres = 2.0f+0.05f*(rand()%20);
				alien->hearing_adapt = 0.25f;
			}
			else
			{
				alien->hearing_thres = -2.0f+0.1f*(rand()%10);
				alien->hearing_adapt = 0.7f;
			}

			switch(rand()%7)
			{
			case 0: case 1: case 2:
				alien->smell_clarity = (float)(rand()%4) +8.0f; break;
			case 3: case 4:
				alien->smell_clarity = randfloat()+1.0f; break;
			case 5: case 6:
				alien->smell_clarity = 0.1f*(randfloat()+1.0f); break;
			}

			alien->walk_energy = rand()%4+8;
			alien->run_energy = ((rand()%10+5)*alien->walk_energy)/5 + rand()%4;
			alien->stamina = 4*((rand()%5+7)*alien->run_energy + rand()%(2*alien->run_energy));
						walk_noise[spsp] = std::vector<noise>(wn,wn+TERRAIN_TYPES);

			switch(rand()%3)
			{
			case 0:
				walk_str = "footsteps"; break;
			case 1:
				walk_str = "cantering"; break;
			case 2:
				walk_str = "trotting"; break;
			}
			switch(rand()%2)
			{
			case 0:
				run_str = "galloping"; break;
			case 1: 
				run_str = "running"; break;
			}
			extra_noise = 9.0f;
			walk_noise[spsp] = std::vector<noise>(wn,wn+TERRAIN_TYPES);
			run_noise[spsp] = std::vector<noise>(rn,rn+TERRAIN_TYPES);
			for(int i=0;i<TERRAIN_TYPES;i++)
			{
				walk_noise[spsp][i].describe = walk_str;
				walk_noise[spsp][i].volume +=extra_noise;
				run_noise[spsp][i].describe = run_str;
				run_noise[spsp][i].volume +=extra_noise*1.6f;
			}
			break;
		case 2:
			alien->description = "A huge alien.";
			alien->damage = rand()%8+8;
			alien->min_health = 20+rand()%10;
			alien->health_range = rand()%7+5;
			alien->body_sections = 9+rand()%16;

			if(rand()%12<3+(difficulty/5))
			{ alien->psychic_range = rand()%30+20; }
			else
			{
				alien->psychic_range = 0;
			}

			switch(rand()%7)
			{
			case 0: case 1: case 2: case 3:
				alien->vis_range = rand()%6+6; break;
			case 4: case 5:
				alien->vis_range = rand()%4+4; break;
			case 6:
				alien->vis_range = rand()%2+2; break;
			}

			if(rand()%7 < 4)
			{
				alien->hearing_thres = 2.5f+0.1f*(rand()%15);
				alien->hearing_adapt = 0.1f;

			}
			else
			{
				alien->hearing_thres = -5.0f+0.03f*(rand()%100);
				alien->hearing_adapt = 1.0f;

			}

			switch(rand()%7)
			{
			case 0: case 1:
				alien->smell_clarity = (float)(rand()%4) +8.0f; break;
			case 2: case 3: case 4:
				alien->smell_clarity = randfloat()+1.0f; break;
			case 5: case 6:
				alien->smell_clarity = 0.1f*(randfloat()+1.0f); break;
			}

			alien->walk_energy = rand()%3+6;
			alien->run_energy = alien->walk_energy + rand()%6;
			alien->stamina = 2*((rand()%4+5)*alien->run_energy + rand()%(2*alien->run_energy));
			switch(rand()%3)
			{
			case 0:
				walk_str = "thumping"; break;
			case 1:
				walk_str = "pounding"; break;
			case 2:
				walk_str = "crashing"; break;
			}
			switch(rand()%1)
			{
			case 0:
				run_str = "charging"; break;
			}
			extra_noise = 11.5f;
			walk_noise[spsp] = std::vector<noise>(wn,wn+TERRAIN_TYPES);
			run_noise[spsp] = std::vector<noise>(rn,rn+TERRAIN_TYPES);
			for(int i=0;i<TERRAIN_TYPES;i++)
			{
				walk_noise[spsp][i].describe = walk_str;
				walk_noise[spsp][i].volume +=extra_noise;
				run_noise[spsp][i].describe = run_str;
				run_noise[spsp][i].volume +=extra_noise*1.6f;
			}
		}
		/*char buf[1000] = "";
		sprintf(buf,"A %s is a %s creature %s.",
			alien->name.c_str(),
			(std::string(alien->psychic_range?"psychic":"")+std::string(alien->psychic_range&&alien->sees?", ":"")+std::string(alien->sees?"sighted":"")).c_str(),
			(std::string(alien->smells||alien->hears?"with good ":"")+std::string(alien->hears?"hearing":"")+
			 std::string(alien->hears&&alien->smells?" and ":"")+std::string(alien->smells?"sense of smell":"")).c_str());
		alien->description = std::string(buf);*/
		alien->walk_energy += (difficulty-5)/3;
		alien->run_energy += (difficulty-5)/2;
		alien->damage += (difficulty - 5)/2;
		alien->hearing_thres -= (float)(difficulty-5)/3.0f;
		alien->min_health += (difficulty-5)/3;
		alien->vis_range += (difficulty-5)/4;
		if(rand()%10 > difficulty) alien->psychic_range += 2*difficulty;
		zoo.push_back(alien);
	}
}

struct actor
{
	species* sspecies;
	bool is_player;
	bool is_hologram;
	int health;
	int energy;
	bool running;
	int stamina;
	bool dead;
	bool certain;
	int x, y;
	int ai_x, ai_y;
	int runaway_x, runaway_y;
	float noise_this_turn;
	float sound_threshold;
	float most_noticeable_sound;
};
actor* current_target = NULL;
std::vector<actor*> visible_actors;

actor* p_ptr = NULL;
actor* map_occupants[MAP_SIZE][MAP_SIZE];
bool terrain_immovable(actor* a, int x, int y)
{
	if(!on_map(x,y)) return false;
	if (map_terrain[x][y] == WALL || map_terrain[x][y] == NW_SE|| map_terrain[x][y] == SW_NE || map_terrain[x][y] == TWINDOW
		|| map_terrain[x][y] == NS || map_terrain[x][y] == WE || map_terrain[x][y] == NOSE || map_terrain[x][y] == CPANEL) return true;
	if (map_occupants[x][y] != NULL && map_occupants[x][y] != a) return true;
	return false;
}

std::pair<int,int> random_occupiable_square_away_from_player()
{
	int target_x = rand()%MAP_SIZE; int target_y = rand()%MAP_SIZE;
	while((map_terrain[target_x][target_y] != DIRT && map_terrain[target_x][target_y] != FLOOR) || d22(target_x,target_y,p_ptr->x,p_ptr->y) <= (vis_range+2)*(vis_range+2))
	{
		target_x = rand()%MAP_SIZE; target_y = rand()%MAP_SIZE;
	}
	return std::make_pair(target_x,target_y);
}

//std::pair<int,int> random_occupiable_square()
//{
//	int target_x = rand()%MAP_SIZE; int target_y = rand()%MAP_SIZE;
//	while(map_terrain[target_x][target_y])
//	{
//		target_x = rand()%MAP_SIZE; target_y = rand()%MAP_SIZE;
//	}
//	return std::make_pair(target_x,target_y);
//}

struct item
{
	actor* projection;
	int colour;
	char ch;
	std::string name;
	std::string description;
	std::string base_description;
	int device_type;
	int wavelength;
	int power_remaining;
	int time_remaining;
	int species_stored;
	int current_action;
	int next_action;
	int configuration;
	int x;
	int y;
	bool player_has;
};

struct explosion
{
	int x; int y; int radius; int centre_damage;
};
std::vector<explosion> explosions_this_turn;
std::vector<item*> explode_this_turn;
std::vector<item*> active_devices;
item* rm_devices[WAVELENGTHS];
std::vector<item*> timer_devices;
std::vector<item*> next_timer_devices;
item* map_items[MAP_SIZE][MAP_SIZE];
std::vector<actor*> actors;
std::vector<item*> items;
std::vector<actor*> holograms;
std::vector<item*> devices;
std::vector<std::vector<std::vector<item*>>> dev_inv;//type,configuration,individual. Not joking.


const float ideal_clarity = 2.0;
std::string sound_this_turn = "";
std::string dir_this_turn = "";
void react_to_noise(actor* a,noise* n,int xx, int yy)
{
	if(a->is_player)
	{
		sound_this_turn = n->describe;
		if(on_map(xx,yy))
		{
			int dx = xx-a->x; int dy = yy-a->y;
			if(dx != 0)
			{
				float t = 180*atan(float(dy)/(float)dx)/PI;
				if(t<-67.5)
				{dir_this_turn = (dx>0?"North":"South");}
				else if(t<-22.5)
				{dir_this_turn = (dx>0?"NEast":"SWest");}
				else if(t<22.5)
				{dir_this_turn = (dx>0?"East":"West");}
				else if(t<67.5)
				{dir_this_turn = (dx>0?"SEast":"NWest");}
				else {dir_this_turn = (dx>0?"South":"North");}
			}
			else
			{
				if(dy == 0)
				{
					dir_this_turn = "here";
				}
				else {dir_this_turn = (dy>0)?"South":"North";}
			}
		}
		else{dir_this_turn = "???";}
	}
	else if(!a->is_hologram)
	{
		if(n->scary)
		{
			if(a->sspecies->size != 2)
			{
			a->runaway_x = xx;
			a->runaway_y = yy;
			}
		}
		else
		{
			if(!a->certain)
			{
				a->ai_x = xx;
				a->ai_y = yy;
			}
		}
	}
}

void make_noise(int x,int y,noise* n)
{
for(std::vector<actor*>::iterator a = actors.begin(); a != actors.end(); ++a)
{
	if(!(*a)->dead && !(*a)->is_hologram)
	{
		int d2 = d22(x,y,(*a)->x,(*a)->y);
		float rel_vol = n->volume - 2*log((float)d2+1);
		float clarity = (rel_vol - (*a)->sound_threshold);
		float clarity2 = abs(clarity-ideal_clarity);
		if(clarity > 0)
		{
			(*a)->sound_threshold  = std::max((*a)->sound_threshold,rel_vol - ideal_clarity);
		}
		if((*a)->most_noticeable_sound < clarity)
		{
			int d = (int)ceil(sqrt((float)d2)/2);
			int xx, yy;
			(*a)->sound_threshold = rel_vol - ideal_clarity;
			if(clarity2 <= ideal_clarity/4)
			{
				xx = x; yy = y;
			}
			else if(clarity2 <= ideal_clarity/2)
			{
				xx = x-d/2 + rand()%(d+1);
				yy = y-d/2 + rand()%(d+1);
			}
			else if(clarity2 <= ideal_clarity)
			{
				xx = x-d + rand()%(2*d+1);
				yy = y-d + rand()%(2*d+1);
			}
			else
			{
				xx=-500;
				yy=-500;
			}
			react_to_noise(*a,n,xx,yy);
		}

		
	}
}
}


int count_inv_devices(int dev_type)
{
	int ret = 0;
	for(int i=0; i<DEVICE_CONFIGURATIONS;i++)
	{
		ret += dev_inv[dev_type][i].size();
	}
	return ret;
}

int total_inv_devices()
{
	int ret = 0;
	for(int i=0;i<DEVICE_TYPES;i++)
		ret += count_inv_devices(i);
	return ret;
}

std::pair<int,int> nearby_home_for_item(int x,int y)
{
	if(map_items[x][y] == NULL && !(map_terrain[x][y]==WALL))
	{
		return std::make_pair(x,y);
	}
	for(int d=0;d<8;d++)
	{
		std::pair<int,int> dir = directions[d];
		int xx = x+dir.first; int yy =y+dir.second;
		if(map_items[xx][yy] == NULL && !(map_terrain[xx][yy]==WALL))
		{
			return std::make_pair(xx,yy);
		}
	}
	return std::make_pair(-1,-1);

}

void inv_remove(item* dev)
{
	kill_velement(dev_inv[dev->device_type][dev->configuration],dev);
}

void item_to_location(item* a, int x, int y)
{
	if(a->player_has)
	{
		inv_remove(a);
		a->player_has = false;
	}
	std::pair<int,int> place_here = nearby_home_for_item(x,y);
	if(place_here != std::make_pair(-1,-1))
	{
		map_items[place_here.first][place_here.second] = a;
		a->x = place_here.first; a->y = place_here.second;
	}
	else
	{
		add_message("Abnormal item placement! Maybe it's in a wall, maybe it just squashed another item.");
		//draw_last_msg();
		map_items[x][y] = a;
		a->x = x; a->y = y;
	}

}

void device_to_inventory(item* a)
{
	if(!a->player_has)
	{
		map_items[a->x][a->y] = NULL;
		a->player_has = true;
		dev_inv[a->device_type][a->configuration].push_back(a);
	}

}

void add_junk(std::string name, std::string description, char ch, int colour, int x, int y)
{
	item* it = new item();
	it->projection = NULL;
	it->colour = colour;
	it->ch = ch;
	it->device_type = NOT_DEVICE;
	it->name = name;
	it->description = description;
	it->base_description = "";
	it->player_has = false;
	items.push_back(it);
	item_to_location(it,x,y);

}

actor* add_actor(species* sp, bool p, int xx, int yy,bool hologram=false)
{
	actor* a = new actor();
	a->is_hologram = hologram;
	a->sspecies = sp;
	a->is_player = p;
	a->dead = false;
	a->running = false;
	a->stamina = sp->stamina;
	a->energy = 100*(int)(p);

	a->x = xx;
	a->y = yy;
	a->certain = false;
	a->ai_x = xx;
	a->ai_y = yy;
	a->most_noticeable_sound = 0.0f;
	a->runaway_x = -500;
	a->runaway_y = -500;
	actors.push_back(a);
	if(!hologram)
	{
	map_occupants[xx][yy] = a;
	}
	a->health = (sp->health_range? rand()%(sp->health_range):0)+sp->min_health;
	a->sound_threshold = a->sspecies->hearing_thres;
	if(hologram)
	{
		holograms.push_back(a);
	}
	return a;
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

void make_room(std::pair<int,int> centre, std::pair<int,int> size)
{
	int x=centre.first;int y=centre.second;int xsize=size.first;int ysize=size.second;
	std::vector<std::pair<int,int>> inner_walls;
	std::vector<std::pair<int,int>> outer_walls;
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
					{ outer_walls.push_back(std::make_pair(i+x-xsize/2,j+y-ysize/2)); }
				}
			}
			else
			{
				if(map_terrain[i+x-xsize/2][j+y-ysize/2] != WALL && map_terrain[i+x-xsize/2][j+y-ysize/2] != DOOR && map_terrain[i+x-xsize/2][j+y-ysize/2] != TWINDOW)
				{
					map_terrain[i+x-xsize/2][j+y-ysize/2] = FLOOR;
				}
				else if(map_terrain[i+x-xsize/2][j+y-ysize/2] == DOOR && rand()%3 == 0)
				{
					map_terrain[i+x-xsize/2][j+y-ysize/2] = FLOOR;
				}
				else if(!is_corner_b(i+x-xsize/2,j+y-ysize/2))
				{
					inner_walls.push_back(std::make_pair(i+x-xsize/2,j+y-ysize/2));
				}
			}
		}
	}
	if(inner_walls.size()>0) // this should really be done with shuffle
	{
		int door_id = rand()%inner_walls.size();
		std::pair<int,int> inner_door_choice = inner_walls[door_id];
		map_access[inner_door_choice.first+1][inner_door_choice.second] = true;
		map_access[inner_door_choice.first-1][inner_door_choice.second] = true;
		map_access[inner_door_choice.first][inner_door_choice.second+1] = true;
		map_access[inner_door_choice.first][inner_door_choice.second-1] = true;
		map_terrain[inner_door_choice.first][inner_door_choice.second] = DOOR;
		if(inner_walls.size() > 2)
		{
			int w1_id = rand()%inner_walls.size();
			while(w1_id == door_id)
			{
				w1_id = rand()%inner_walls.size();
			}
			std::pair<int,int> w1_choice = inner_walls[w1_id];
			map_terrain[w1_choice.first][w1_choice.second] = TWINDOW;
			if(inner_walls.size() > 4)
			{
				int w2_id = rand()%inner_walls.size();
				while(w2_id == door_id || w2_id == w1_id)
				{
					w2_id = rand()%inner_walls.size();
				}
				std::pair<int,int> w2_choice = inner_walls[w2_id];
				map_terrain[w2_choice.first][w2_choice.second] = TWINDOW;
				}
		}
	}
	if(outer_walls.size()>0)
	{
		int door_id = rand()%outer_walls.size();
		std::pair<int,int> outer_door_choice = outer_walls[door_id];
		map_terrain[outer_door_choice.first][outer_door_choice.second] = DOOR;
		map_access[outer_door_choice.first+1][outer_door_choice.second] = true;
		map_access[outer_door_choice.first-1][outer_door_choice.second] = true;
		map_access[outer_door_choice.first][outer_door_choice.second+1] = true;
		map_access[outer_door_choice.first][outer_door_choice.second-1] = true;
		if(outer_walls.size() > 2)
		{
			int w1_id = rand()%outer_walls.size();
			while(w1_id == door_id)
			{
				w1_id = rand()%outer_walls.size();
			}
			std::pair<int,int> w1_choice = outer_walls[w1_id];
			map_terrain[w1_choice.first][w1_choice.second] = TWINDOW;
			if(outer_walls.size() > 4)
			{
				int w2_id = rand()%outer_walls.size();
				while(w2_id == door_id || w2_id == w1_id)
				{
					w2_id = rand()%outer_walls.size();
				}
				std::pair<int,int> w2_choice = outer_walls[w2_id];
				map_terrain[w2_choice.first][w2_choice.second] = TWINDOW;
				}
		}
	}

}

std::pair<int,int> can_make_room(std::pair<int,int> xy,int xcentre,int ycentre)
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
			if (!on_map(i+x-xsize/2,j+y-ysize/2)) {return std::make_pair(0,0);}
			if (i+x-xsize/2 > xcentre+BUILDING_SIZE || i+x-xsize/2 < xcentre - BUILDING_SIZE) {return std::make_pair(0,0);}
			if (j+y-ysize/2 > ycentre+BUILDING_SIZE || j+y-ysize/2 < ycentre - BUILDING_SIZE) {return std::make_pair(0,0);}
			if(map_access[i+x-xsize/2][j+y-ysize/2]) {return std::make_pair(0,0);}
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
		return std::make_pair(xsize,ysize);
	}
	return std::make_pair(0,0);
}

std::pair<int,int> five_tries_to_make_room(std::pair<int,int> xy, int xcentre, int ycentre)
{
	for(int i=0;i<5;i++)
	{
		std::pair<int,int> ret = can_make_room(xy,xcentre, ycentre);
		if(ret != std::make_pair(0,0)) return ret;
	}
	return std::make_pair(0,0);
}

void draw_ship(int x,int y)
{
	ship_x = x;
	ship_y = y;
	map_terrain[x-4][y] = NOSE;
	map_terrain[x-3][y] = CPANEL;
	map_terrain[x-3][y-1] = SW_NE;
	map_terrain[x-3][y+1] = NW_SE;
	map_terrain[x-2][y-2] = SW_NE;
	map_terrain[x-2][y+2] = NW_SE;
	map_terrain[x-2][y-1] = CPANEL;
	map_terrain[x-2][y] = FLOOR;
	map_terrain[x-2][y+2] = NW_SE;
	map_terrain[x-2][y+1] = CPANEL;
	map_terrain[x-1][y-2] = WE;
	map_terrain[x-1][y+2] = WE;
	map_terrain[x][y-2] = WE;
	map_terrain[x][y+2] = WE;
	map_terrain[x+1][y-2] = WE;
	map_terrain[x+1][y+2] = WE;
	map_terrain[x+2][y-2] = NW_SE;
	map_terrain[x+2][y+2] = SW_NE;
	map_terrain[x+2][y-1] = NS;
	map_terrain[x+2][y+1] = NS;
	map_terrain[x+2][y] = DOOR;
	for(int i=x-1;i<x+2;i++)
	{
		for(int j=y-1;j<y+2;j++)
		{
				map_terrain[i][j] = FLOOR;
		}

	}
	if(map_terrain[x+3][y] == WALL)
	{
		map_terrain[x+3][y] = DOOR;
	}
	if(map_terrain[x+3][y] == FLOOR || map_terrain[x+3][y] == DOOR)
	{
		int i = x+3;
		while(map_terrain[i][y] == FLOOR || map_terrain[i][y] == DOOR) {i++;}
		if(map_terrain[i][y] != STREE && map_terrain[i][y] != BTREE && map_terrain[i][y] != DIRT)
		{
			map_terrain[i][y] = DOOR;
		}
	}

}
bool player_in_ship()
{
	bool player_present = false;
	for(int i=ship_x-1;i<ship_x +2;i++)
	{
		for(int j=ship_y-1;j<ship_y+2;j++)
		{
			actor* a = map_occupants[i][j];
			if(a != NULL)
			{
				if(a->is_player)
				{
					player_present = true;
				}
			}
		}

	}
	actor* a = map_occupants[ship_x-2][ship_y];
	if(a != NULL)
	{
		if(a->is_player)
		{
			player_present = true;
		}
	}
	return player_present;
}

void check_achievements()
{
	if(!player_in_ship()) {left_ship = true;}
	if(map_terrain[p_ptr->x][p_ptr->y] == FLOOR && !player_in_ship()) {entered_building_destroyed_transmitter = true;}
	if(destroyed_transmitter && player_in_ship()) {returned_to_ship = true;}
}

bool door_closable()
{
	//return (bool)(turn_count % 2);
	if (!transmitter_destroyed) return false;
	
	if(map_occupants[ship_x+2][ship_y] != NULL)
	{return false;}
	return player_in_ship();
}

bool win_condition()
{
	if (!transmitter_destroyed) return false;
	bool player_present = false;
	bool monster_present = false;
	for(int i=ship_x-1;i<ship_x +2;i++)
	{
		for(int j=ship_y-1;j<ship_y+2;j++)
		{
			actor* a = map_occupants[i][j];
			if(a != NULL)
			{
				if(a->is_player)
				{
					player_present = true;
				}
				else return false;
			}
		}

	}
	actor* a = map_occupants[ship_x-2][ship_y];
	if(a != NULL)
	{
		if(a->is_player)
		{
			player_present = true;
		}
		else return false;
	}
	if(map_occupants[ship_x+2][ship_y] != NULL)
	{return false;}
	return player_present;
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
			map_human_scent[x][y] = 0.001f*randfloat()+0.02f;
			if(x != 0 && x != MAP_SIZE-1 && y != 0 && y != MAP_SIZE-1)
			{
				switch(rand()%13)
				{
				case 0: case 1: case 2: case 3: case 4:
					map_terrain[x][y] = STREE; break;
				case 5: case 6: case 7:
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
		std::vector<std::pair<int,int>> room_centres;
		room_centres.push_back(std::make_pair(xcentre,ycentre));
		std::vector<std::pair<int,int>> room_sizes;
		room_sizes.push_back(std::make_pair(rand()%5 + 2,rand()%5+2));
		std::vector<int> fail_counts;
		fail_counts.push_back(0);
		make_room(room_centres[0],room_sizes[0]);
		int successive_failures = 0;
		int room_count = 0;
		while(successive_failures < 10 && room_count < 10)
		{
			int x_size = rand()%5 + 3;
			int y_size = rand()%5 + 3;
			if(room_centres.size() == 0 || room_sizes.size() != room_centres.size())
			{
				successive_failures++;
				continue;
			}
			int parent_choice = rand()%room_centres.size();
			std::pair<int,int> parent_centre = room_centres[parent_choice];
			std::pair<int,int> parent_size = room_sizes[parent_choice];
			std::vector<std::pair<int,int>> centre_choices;
			std::vector<std::pair<int,int>> size_choices;
			int choice_range_x = rand()%(x_size-2)+parent_size.first+2;
			int choice_range_y = rand()%(y_size-2)+parent_size.second+2;
			std::pair<int,int> candidate;
			std::pair<int,int> candidate_size;
			for(int x=0;x<choice_range_x;x++)
			{
				candidate=std::make_pair(x+parent_centre.first-choice_range_x/2,parent_centre.second-choice_range_y/2);
				candidate_size = five_tries_to_make_room(candidate,xcentre,ycentre);
				if(candidate_size != std::make_pair(0,0)) {centre_choices.push_back(candidate); size_choices.push_back(candidate_size);}

				candidate=std::make_pair(x+parent_centre.first-choice_range_x/2,choice_range_y-1+parent_centre.second-choice_range_y/2);
				candidate_size = five_tries_to_make_room(candidate,xcentre,ycentre);
				if(candidate_size != std::make_pair(0,0)) {centre_choices.push_back(candidate); size_choices.push_back(candidate_size);}
			}
			for(int y=1;y<choice_range_y;y++)
			{
				candidate = std::make_pair(parent_centre.first-choice_range_x/2,y+parent_centre.second-choice_range_y/2);
				candidate_size = five_tries_to_make_room(candidate,xcentre,ycentre);
				if(candidate_size != std::make_pair(0,0)) {centre_choices.push_back(candidate); size_choices.push_back(candidate_size);}

				candidate=std::make_pair(parent_centre.first-choice_range_x/2+choice_range_x-1,y+parent_centre.second-choice_range_y/2);
				candidate_size = five_tries_to_make_room(candidate,xcentre,ycentre);
				if(candidate_size != std::make_pair(0,0)) {centre_choices.push_back(candidate); size_choices.push_back(candidate_size);}
			}
			if(centre_choices.size() == 0 || size_choices.size() != centre_choices.size())
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

	bool is_ok = false;
	int xcentre = rand()%(MAP_SIZE-2*BUILDING_SIZE)+BUILDING_SIZE;
	int ycentre = rand()%(MAP_SIZE-2*BUILDING_SIZE)+BUILDING_SIZE;
	while(!is_ok)
	{
		is_ok = true;
		for(int j=0;j<buildings;j++)
		{
			if(abs(xcentre-building_xcentres[j])<=BUILDING_SIZE+10)
			{
				is_ok = false;
				xcentre = rand()%(MAP_SIZE-2*BUILDING_SIZE-2)+BUILDING_SIZE;
				ycentre = rand()%(MAP_SIZE-2*BUILDING_SIZE-2)+BUILDING_SIZE;
			}
		}
	}
	draw_ship(xcentre,ycentre);

	free(building_xcentres); free(building_ycentres);
}

void init_actors()
{
	p_ptr = add_actor(zoo[0],true,ship_x,ship_y);
	std::pair<int,int> ploc;// = random_occupiable_square();
	//ploc = random_occupiable_square_away_from_player();
	for(int i=0;i<10+difficulty;i++)
	{
		ploc = random_occupiable_square_away_from_player();
		add_actor(zoo[(rand()%ZOO_SIZE)+1],false,ploc.first,ploc.second);
	}

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

bool undiffusable(int x, int y)
{
	return (map_terrain[x][y] == WALL || map_terrain[x][y] == SW_NE || map_terrain[x][y] == NW_SE || map_terrain[x][y] == NS || map_terrain[x][y] == WE || map_terrain[x][y] == CPANEL || map_terrain[x][y] == NOSE || map_terrain[x][y] == TWINDOW);
}

bool blocks_shots(int x, int y)
{
	if(!on_map(x,y)) return true;
	return map_terrain[x][y] == WALL || map_terrain[x][y] == BTREE|| map_terrain[x][y] == NW_SE|| map_terrain[x][y] == SW_NE || map_terrain[x][y] == TWINDOW
		|| map_terrain[x][y] == NS || map_terrain[x][y] == WE || map_terrain[x][y] == NOSE || map_terrain[x][y] == CPANEL || map_terrain[x][y] == DOOR;
}

bool is_opaque(int x, int y)
{
	if(!on_map(x,y)) return true;
	return map_terrain[x][y] == WALL || map_terrain[x][y] == BTREE|| map_terrain[x][y] == NW_SE|| map_terrain[x][y] == SW_NE
		|| map_terrain[x][y] == NS || map_terrain[x][y] == WE || map_terrain[x][y] == NOSE || map_terrain[x][y] == CPANEL || map_terrain[x][y] == DOOR;
}

void update_device_desciption(item* dev)
{
	dev->description = dev->name + ". " + dev->base_description + ". "+ vconfig_names[dev->configuration];
	char desc[500] = "";
	if(dev->configuration == ON_TIMER)
	{
		sprintf(desc, ", %.1f seconds remaining.", (float)dev->time_remaining/10);
		dev->description += std::string(desc);
	}
	else if(dev->configuration == ON_REMOTE)
	{
		sprintf(desc, ", wavelength %d.", dev->wavelength+1);
		dev->description += std::string(desc);
	}
}

item* add_device(int type,int x,int y,bool in_inv)
{
	item* dev = new item();
	if(type == HOLOGRAM_PROJECTOR)
	{
		dev->projection = add_actor(zoo[0],false,-500,-500,true);
	}
	dev->device_type = type;
	dev->colour = COLOR_RED;
	dev->wavelength = -1;
	dev->time_remaining = -1;
	dev->species_stored = -1;
	dev->current_action = NULL_STIMULUS;
	dev->next_action = NULL_STIMULUS;
	dev->configuration = UNCONFIGURED;
	dev->power_remaining = 100;
	dev->player_has = false;
	switch (type)
	{
	case 0:
		dev->name = "low explosive";
		dev->ch = 'A';
		dev->description = "Inflicts damage on creatures and walls within a 3x3 area. Including you.";
		break;
	case 1:
		dev->name = "high explosive";
		dev->ch = 'B';
		dev->description = "Inflicts great damage on creatures and walls within a 5x5 area. Including you.";
		break;
	case 2:
		dev->name = "hologram projector";
		dev->description = "Projects holograms that confuse creatures that rely on sight. Such as you.";
		dev->ch = 'C';
		break;
	case 3:
		dev->name = "noise generator";
		dev->description = "Confuses or deafens creatures that rely on hearing. Like you.";
		dev->ch = 'D';
		break;
	case 4:
		dev->name = "scent generator";
		dev->ch = 'E';
		dev->description = "Confuses creatures with a sense of smell. For instance... your dog.";
		break;
	case 5:
		dev->name = "brain slice";
		dev->description = "Confuses creatures with a psychic sense. Like that guy from the infotainment datalinks.";
		dev->ch = 'F';
		break;
	default:
		delete dev;
		return NULL;
	}
	dev->base_description = dev->description;
	if (in_inv)
	{
		dev->x = p_ptr->x;
		dev->y = p_ptr->y;
		device_to_inventory(dev);
	}
	else
	{
		item_to_location(dev,x,y);
	}
	return dev;
}


void destroy_item(item* a)
{
	if (a->device_type != NOT_DEVICE)
	{
		if(a->player_has)
		{
			inv_remove(a);
		}
		else
		{
			map_items[a->x][a->y] = NULL;
		}
		if (a->configuration == ON_REMOTE)
		{
			rm_devices[a->wavelength] = NULL;
		}
		if (a->configuration == ON_TIMER)
		{
			kill_velement(timer_devices,a);
		}
		if(a->device_type == LOW_EXPLOSIVE)
		{
			explosion ex;
			ex.x = a->x;
			ex.y = a->y;
			ex.radius = 2;
			ex.centre_damage = 20;
			explosions_this_turn.push_back(ex);
		}
		if(a->device_type == HIGH_EXPLOSIVE)
		{
			explosion ex;
			ex.x = a->x;
			ex.y = a->y;
			ex.radius = 4;
			ex.centre_damage = 60;
			explosions_this_turn.push_back(ex);
		}
	}
	kill_velement(items,a);

}

void activate_device(item* dev)
{
	dev->current_action = dev->next_action;
	if(dev->device_type == LOW_EXPLOSIVE || dev->device_type == HIGH_EXPLOSIVE)
	{
		destroy_item(dev);
	}
}

void init_items()
{
	for(int i=0;i<WAVELENGTHS;i++)
	{rm_devices[i] = NULL;}
	ammo[PISTOL] = 60-difficulty;
	ammo[RIFLE] = 160 - 4*difficulty;
	ammo[CANNON] = 30-difficulty;
	dev_inv = std::vector<std::vector<std::vector<item*>>>(DEVICE_TYPES,(std::vector<std::vector<item*>>(DEVICE_CONFIGURATIONS)));
	item* dev;
	for(int i=0;i<5;i++)
	{
		dev = add_device(LOW_EXPLOSIVE,-1,-1,true);
		device_to_inventory(dev);
	}
	for(int i=0;i<3;i++)
	{
		dev = add_device(HIGH_EXPLOSIVE,-1,-1,true);
		device_to_inventory(dev);
	}
	for(int i=rand()%3+1;i<5;i++)
	{
		dev = add_device(HOLOGRAM_PROJECTOR,-1,-1,true);
		device_to_inventory(dev);
	}
	for(int i=rand()%3+1;i<5;i++)
	{
		dev = add_device(NOISE_GENERATOR,-1,-1,true);
		device_to_inventory(dev);
	}
	for(int i=rand()%3+1;i<5;i++)
	{
		dev = add_device(SCENT_GENERATOR,-1,-1,true);
		device_to_inventory(dev);
	}
	for(int i=rand()%3+1;i<5;i++)
	{
		dev = add_device(BRAIN_SLICE,-1,-1,true);
		device_to_inventory(dev);
	}
}

void init()
{
	rlmessages.push_back(" ");
	initscr();
	if(has_colors() == FALSE)
	{
		endwin();
		printf("Your terminal does not support colour\n");
		exit(1);
	}
    start_color();
	for(int i=0;i<8;i++)
	{
		init_pair(i,i,COLOR_BLACK);
	}
	noecho();
	cbreak();
	curs_set(0);
	keypad(stdscr,TRUE);
	mvaddstr(4,10,"Choose a difficulty level (0 to 9):");
	mvaddstr(5,13,"2 is too easy, 9 is too hard,");
	mvaddstr(6,10,"3 is ok for beginners, 7 is standard");
	//mvaddstr(8,12,"Press t for the tutorial");
	move(7,15);
	int kp = 0;
	while((kp - '0' < 0 || kp - '0' > 9) /*&& kp != 't' && kp != 'T'*/)
	{
		kp = getch();
	}
	//if (kp == 't' || kp == 'T')
	//{
	//	difficulty = -1;
	//	init_tutorial_species();
	//	init_tutorial_map();
	//	init_tutorial_actors();
	//	init_tutorial_items();
	//}
	//else
	{
		difficulty = kp - '0';
		if(difficulty > 6) difficulty += (difficulty-6)*3;
		//scratch = newpad(viewport_height,viewport_width);
		init_species();
		init_map();
		init_actors();
		init_items();
		add_message("The ship plays a cheerful tune, and the door opens. \"Automatic docking completed. Welcome to Spaceport X7/3A! Please enjoy your stay.\"");
		std::vector<std::pair<int,int>> transmitter_options;
		int acceptable_range = 70;
		while(transmitter_options.size() == 0)
		{
			for(int x = 0; x < MAP_SIZE; x++)
			{
				for(int y = 0; y < MAP_SIZE; y++)
				{
					if(map_terrain[x][y] == FLOOR && d22(x,y,ship_x, ship_y) > acceptable_range*acceptable_range)
						transmitter_options.push_back(std::make_pair(x,y));
				}
			}
			acceptable_range--;
		}

		std::pair<int,int> target_pair = transmitter_options[rand()%transmitter_options.size()];
		target_x = target_pair.first; target_y = target_pair.second;
		//target_x = rand()%MAP_SIZE; target_y = rand()%MAP_SIZE;
	
		/*while(map_terrain[target_x][target_y] != FLOOR || d22(ship_x,ship_y,target_x,target_y) <= squarei(acceptable_range))
		{
			target_x = rand()%MAP_SIZE; target_y = rand()%MAP_SIZE;
			if(rand()%4 != 0) acceptable_range--;
		}*/
		add_junk("radio transmitter","So this is what's been causing you so much trouble!",'?',COLOR_CYAN,target_x,target_y);
	}
}



inline void draw_ch(int x,int y,char ch, int col)
{
	attron(COLOR_PAIR(col));
	mvaddch(message_area_height+1+y-viewport_top_edge(),x-viewport_left_edge(),ch);
}
inline void draw_ch_bright(int x,int y,char ch, int col)
{
	attron(COLOR_PAIR(col));
	mvaddch(message_area_height+y+1-viewport_top_edge(),x-viewport_left_edge(),ch|A_BOLD);
}
std::string walk_description(species* sp)
{
	if(!sp->walk_known) return "    ";
	if(sp->walk_energy > human_sp->run_energy) return "vfst";
	else if(sp->walk_energy > human_sp->walk_energy) return "fast";
	else if(sp->walk_energy < human_sp->walk_energy) return "slow";
	return "avrg";
}
std::string run_description(species* sp)
{
	if(!sp->run_known) return "    ";
	if(sp->run_energy > human_sp->run_energy) return "fast";
	else if(sp->run_energy < human_sp->walk_energy) return "vslw";
	else if(sp->run_energy < human_sp->run_energy) return "slow";
	return "avrg";
}
std::string damage_description(species* sp)
{
	if(!sp->damage_known) return "    ";
	if(sp->damage <= 4) return "weak";
	if(sp->damage <= 8) return "strg";
	if(sp->damage <= 14) return "dngr";
	return "argh";
}
std::string health_description(species* sp)
{
	if(!sp->health_known) return "     ";
	if(sp->min_health <= 4) return "fethr";
	if(sp->min_health <= 8) return "frail";
	else if(sp->min_health <= 14) return " avrg";
	else if(sp->min_health <= 22) return "tough";
	else if(sp->min_health <= 36) return "v tgh";
	return "vvtgh";
}
std::string crit_description(species* sp)
{
	if(sp->crit_known) return "*";
	return " ";;
}
std::string size_description(species* sp)
{
	if(!sp->size_known) return "     ";
	return std::string(size_descriptors[sp->size]);
}
void draw_memory()
{
	attron(COLOR_PAIR(COLOR_WHITE));
	mvaddstr(10,viewport_width+1,"M size walk  run  dmg  hlth cr");
	for(int i=11;i<11+ZOO_SIZE;i++)
	{
		species* sp = zoo[i-10];
		if(sp->size_known)
		attron(COLOR_PAIR(sp->colour));
		else attron(COLOR_PAIR(COLOR_WHITE));
		mvaddstr(i,viewport_width+1,std::string(1,sp->ch).c_str());
		attron(COLOR_PAIR(COLOR_WHITE));
		mvaddstr(i, viewport_width+1+2, (size_description(sp)+" "+
			walk_description(sp)+" "+run_description(sp)+" "+damage_description(sp)+
			" "+health_description(sp)+" "+crit_description(sp)).c_str());
	}
}
void draw_stats()
{
	attron(COLOR_PAIR(COLOR_WHITE));

	//for(int i=3;i<11+ZOO_SIZE;i++)
	//{
	//	mvaddstr(i,viewport_width+1,std::string(disp_columns-viewport_width-1,' ').c_str());
	//}
	clear_area(viewport_width+1,3,disp_columns-viewport_width-1,15+ZOO_SIZE);
	mvprintw(3,(viewport_width+1),"HP: %d/%d %s    ",p_ptr->health,p_ptr->sspecies->min_health,
		p_ptr->running?"running":"       ");
	/*mvprintw(4,(viewport_width+1),(std::string("     ")+
		std::string(current_weapon==PISTOL?"________ ":"         ")+
		std::string(current_weapon==RIFLE?"_______ ":"        ")+
		std::string(current_weapon==CANNON?"________":"        ")).c_str());*/
	mvprintw(5,(viewport_width+1),"Ammo ");
	if(current_weapon == PISTOL) attron(COLOR_PAIR(COLOR_YELLOW)); else attron(COLOR_PAIR(COLOR_WHITE));
	mvprintw(5,(viewport_width+1+5),"X Pistol ");
		if(current_weapon == RIFLE) attron(COLOR_PAIR(COLOR_YELLOW)); else attron(COLOR_PAIR(COLOR_WHITE));
	mvprintw(5,(viewport_width+1+5+9),"Y Rifle ");
	if(current_weapon == CANNON) attron(COLOR_PAIR(COLOR_YELLOW)); else attron(COLOR_PAIR(COLOR_WHITE));
	mvprintw(5,(viewport_width+1+5+9+8),"Z Cannon");
	attron(COLOR_PAIR(COLOR_WHITE));

	mvprintw(6,(viewport_width+1),"          %3d     %3d       %2d",ammo[PISTOL],ammo[RIFLE],ammo[CANNON]);
	mvprintw(7,(viewport_width+1),"%10s ",mode_text[current_mode]);
	attron(COLOR_PAIR(COLOR_BLUE));
	printw(std::string(10-(10*p_ptr->stamina / p_ptr->sspecies->stamina),'*').c_str());
	attron(COLOR_PAIR(COLOR_WHITE));
	printw("%s stamina", std::string(10*p_ptr->stamina / p_ptr->sspecies->stamina,'*').c_str());
	mvprintw(8,(viewport_width+1),"Devic LEx HEx Hol Noi Sce Brn");
	mvprintw(9,(viewport_width+1),"Count  %2d  %2d  %2d  %2d  %2d  %2d",
		count_inv_devices(LOW_EXPLOSIVE),count_inv_devices(HIGH_EXPLOSIVE),
		count_inv_devices(HOLOGRAM_PROJECTOR),count_inv_devices(NOISE_GENERATOR),
		count_inv_devices(SCENT_GENERATOR),count_inv_devices(BRAIN_SLICE));
	mvprintw(11+ZOO_SIZE,(viewport_width+1),(sound_this_turn+(sound_this_turn!=""?" from ":"")+dir_this_turn).c_str());
	if(current_mode == WALK_MODE)
	{
	mvprintw(13+ZOO_SIZE,viewport_width+3,"Press ? for help.");
	}
	else if(current_mode == FIRE_MODE)
	{
		mvprintw(13+ZOO_SIZE,viewport_width+3,"Tab to target, f to fire");
	}
	else if(current_mode == THROW_MODE)
	{
		mvprintw(13+ZOO_SIZE,viewport_width+3,"Tab to target, t to throw");
	}
	if(cycle_advice)
	{
	mvprintw(15+ZOO_SIZE,viewport_width+1,vadvice[current_advice].c_str());
	}
	mvprintw(16+ZOO_SIZE,viewport_width+3,"Press M to toggle advice");
	if(door_closable())
	{
	mvprintw(17+ZOO_SIZE,viewport_width+5,"Press C to close the door");
	}
	draw_memory();
}

bool los_exists(int x1, int y1, int x2, int y2, bool (*opacity_test)(int,int))
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
				if (opacity_test(x1, y1)) return false;
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
				if (opacity_test(x1, y1)) return false;
			}
        }
    }
	return true;
}

bool symmetrical_los(int x1, int y1, int x2, int y2,bool (* opacity_test)(int,int))
{
	return (los_exists(x1, y1, x2, y2,opacity_test) || los_exists(x2, y2, x1, y1,opacity_test));
}

inline bool in_vis_range(int x, int y, int xx, int yy)
{
	return d22(x,y,xx,yy) <= vis_range*vis_range;
}

inline bool sym_los_in_range(int x1, int y1, int x2, int y2,bool (*opacity_test)(int,int))
{
	return symmetrical_los(x1,y1,x2,y2,opacity_test) && in_vis_range(x1,y1,x2,y2);
}

bool tile_visible(int x, int y)
{
	if (debug) return true;
	if(sym_los_in_range(p_ptr->x,p_ptr->y,x,y,is_opaque)) return true;
	for(unsigned int i=0; i<active_devices.size(); i++)
	{
		//if(active_devices[i]->device_type == HOLOGRAM_PROJECTOR) // put spy cameras on *all* devices
		{
			if(sym_los_in_range(active_devices[i]->x,active_devices[i]->y,x,y,is_opaque)) return true;
		}
	}
	return false;

}

void draw_tile_description(int x, int y)
{
	for(int i=0;i<3;i++)
	{
		mvaddstr(i,(viewport_width+1),std::string(disp_columns-(viewport_width+1),' ').c_str());
	}
	if (!map_seen[x][y] && !debug)
	{
		attron(COLOR_PAIR(COLOR_WHITE));
		mvprintw(0,(viewport_width+1),"%d, %d, unseen",x,y);
	}
	else
	{
		attron(COLOR_PAIR(terrain_colours[map_terrain[x][y]]));
		mvprintw(0,(viewport_width+1),"%d, %d, %s",x,y,terrain_descriptors[map_terrain[x][y]]);
		if (map_items[x][y] != NULL)
		{
			attron(COLOR_PAIR(map_items[x][y]->colour));
			mvaddstr(1,(viewport_width+1),("a "+map_items[x][y]->name).c_str());
		}
		if (map_occupants[x][y] != NULL)
		{
			//if(sym_los_in_range(p_ptr->x,p_ptr->y,x,y,is_opaque) || debug)
			if(tile_visible(x,y))
			{
				attron(COLOR_PAIR(map_occupants[x][y]->sspecies->colour));
				mvaddstr(2,(viewport_width+1),(map_occupants[x][y]->sspecies->name +
					(map_occupants[x][y]->running?", running":"         ")).c_str());
				if(current_mode == FIRE_MODE || current_mode == THROW_MODE)
				{
					attron(COLOR_PAIR(COLOR_WHITE));
					mvaddstr(2, disp_columns-16," ('i' for info)");
				}
			}
		}
	}
}
void undraw_tile_description()
{
	for (int i=0;i<3;i++)
	mvaddstr(i,(viewport_width+1),std::string(disp_columns-(viewport_width+1),' ').c_str());
}

char scent_strength(float scent)
{
	if (scent < 0.001) return '.';
	if (scent < 0.01) return ',';
	if (scent < 0.1) return ':';
	if (scent < 0.3) return ';';
	if (scent < 0.6) return '-';
	if (scent < 1.0) return '=';
	if (scent < 10.0) return '/';
	if(scent < 50.0) return '*';
	if (scent < 100.0) return '&';
	return '&';
	
	
}

void draw_tile(int x, int y, bool record_actors = false)
{
	if(!on_map(x,y))
	{draw_ch(x,y,' ',COLOR_WHITE); return;}
	char ch = terrain_chars[map_terrain[x][y]];
	int colour = terrain_colours[map_terrain[x][y]];
	if (!map_seen[x][y] && !debug)
	{draw_ch(x,y,' ',colour); return;}
	if(debug)
		ch = scent_strength(map_human_scent[x][y]);
	if (map_items[x][y] != NULL)
	{
		ch = map_items[x][y]->ch;
		colour = map_items[x][y]->colour;
	}
	//if(sym_los_in_range(p_ptr->x,p_ptr->y,x,y,is_opaque)|| debug)
	if(tile_visible(x,y))
	{
		if (map_occupants[x][y] != NULL)
		{
			actor* a = map_occupants[x][y];
			ch = a->sspecies->ch;
			if('a'<= ch && ch <= 'z' && a->running)
			{
				ch -= ('a'-'A');
			}
			colour = a->sspecies->colour;
			if(a->health < a->sspecies->min_health/4)
			{colour = COLOR_RED;}
			a->sspecies->size_known = true;
			if(a->running) a->sspecies->run_known = true;
			else a->sspecies->walk_known = true;
			if(a != p_ptr && record_actors)
			{
				visible_actors.push_back(a);
			}
		}
		draw_ch_bright(x,y,ch,colour);//ch = ch | A_BOLD;
	}
	else
	{
		draw_ch(x,y,ch,colour);
	}
}
//
//void draw_tile_dark(int x, int y,bool record_actors = false)
//{
//	if(!on_map(x,y))
//	{draw_ch(x,y,' ',COLOR_WHITE); return;}
//	char ch = terrain_chars[map_terrain[x][y]];
//	int colour = terrain_colours[map_terrain[x][y]];
//	if (!map_seen[x][y] && !debug)
//	{draw_ch(x,y,' ',colour); return;}
//	if(debug)
//		ch = scent_strength(map_human_scent[x][y]);
//	if (map_items[x][y] != NULL)
//	{
//		ch = map_items[x][y]->ch;
//		colour = map_items[x][y]->colour;
//	}
//	if (map_occupants[x][y] != NULL)
//	{
//		if(symmetrical_los(p_ptr->x,p_ptr->y,x,y) || debug)
//		{
//			ch = map_occupants[x][y]->sspecies->ch;
//			colour = map_occupants[x][y]->sspecies->colour;
//			map_occupants[x][y]->sspecies->size_known = true;
//			if(map_occupants[x][y]->running) map_occupants[x][y]->sspecies->run_known = true;
//			else map_occupants[x][y]->sspecies->walk_known = true;
//			if(map_occupants[x][y] != p_ptr && record_actors)
//			{
//				visible_actors.push_back(map_occupants[x][y]);
//			}
//
//		}
//	}
//	draw_ch(x,y,ch,colour);
//}

void draw_line(int x1, int y1, int x2, int y2, char ch)
{
    // if x1 == x2 or y1 == y2, then it does not matter what we set here
	int col = COLOR_BLUE;
	if(on_map(x2,y2))
	{
		if(map_occupants[x2][y2] != NULL && sym_los_in_range(x1,y1,x2,y2,blocks_shots))
		{
			col = COLOR_CYAN;
		}
	}
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
			if (blocks_shots(x1,y1)) {col = COLOR_RED;}
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
			if (blocks_shots(x1,y1)) {col = COLOR_RED;}
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

bool nearer_actor (actor* a,actor* b)
{ return (d22(a->x,a->y,p_ptr->x,p_ptr->y)<d22(b->x,b->y,p_ptr->x,p_ptr->y)); }

void draw_scene(bool record_actors=false)
{
	
	draw_tile_description(p_ptr->x,p_ptr->y);
	centre_x = std::min(p_ptr->x,MAP_SIZE - viewport_width/2-1);
	centre_x = std::max(centre_x,viewport_width/2);
	centre_y = std::min(p_ptr->y,MAP_SIZE - viewport_height/2-1);
	centre_y = std::max(centre_y,viewport_height/2-1);
	for(int x=viewport_left_edge();x<=viewport_right_edge(); x++)
	{
		for(int y=viewport_top_edge();y<viewport_bottom_edge(); y++)
		{
			draw_tile(x,y,record_actors);
		}
	}
	draw_stats();
	//for(int x=
	sort(visible_actors.begin(),visible_actors.end(),nearer_actor);
	draw_last_msg();
	wrefresh(stdscr);
}
int move_actor(actor* a, int dx, int dy)
{

	if (dx==0 && dy==0)
		return 1;
	int ox = a->x; int oy = a->y;
	int x = ox+dx; int y = oy+dy;
	if(!on_map(x,y)) return 1;
	if (terrain_immovable(a,x,y))
	{

		return 1;
	}
	if(a->is_player)
	{
		for(unsigned int i=0;i<devices.size();i++)
		{
			if(devices[i]->player_has)
			{
				devices[i]->x = p_ptr->x; devices[i]->y = p_ptr->y;
			}
		}
	}
	map_occupants[ox][oy] = NULL;
	map_occupants[x][y] = a;
	if(a->ai_x == a->x && a->ai_y == a->y)
	{
		a->ai_x = x; a->ai_y = y;
	}
	a->x = x; a->y = y;
	if(a->running)
	{
		make_noise(x,y,&run_noise[a->sspecies->id][map_terrain[x][y]]);
	}
	else
	{
		make_noise(x,y,&walk_noise[a->sspecies->id][map_terrain[x][y]]);
	}
	return 0;
}

void draw_info(int x,int y)
{
	current_mode = INFO_MODE;
	draw_stats();
	std::string s;
	//for(int i = 0; y<10; y++)
	//{
	//	mvaddstr(i,0,std::string(viewport_width,' ').c_str());
	//}
	if(map_items[x][y] != NULL)
	{
		if(map_items[x][y]->device_type != NOT_DEVICE)
		{
			update_device_desciption(map_items[x][y]);
		}
		//mvprintw(0,0,map_items[x][y]->description.c_str());
		s += map_items[x][y]->description;
	}
	if(map_occupants[x][y] != NULL)
	{
		actor* a = map_occupants[x][y];
	//mvprintw(0,0,map_occupants[x][y]->sspecies->description.c_str());
		s += a->sspecies->description;
		if(a->running) s += " It is running.";
		if(a->stamina < a->sspecies->stamina/4) s+= " It looks exhausted.";
		else if(a->stamina < a->sspecies->stamina/2) s += " It looks sweaty.";
		if(a->health > (3*a->sspecies->min_health)/4) s+= " It looks healthy.";
		else if(a->health > (a->sspecies->min_health/2)) s += "It looks slightly wounded.";
		else if(a->health > (a->sspecies->min_health/4)) s += "It looks heavily wounded.";
		else s += "It looks close to death.";
		if(a->sspecies->psychic_range) s+= " It is psychic.";
	    if(a->sspecies->smell_clarity >= 1.0f) s += " It has a good sense of smell.";
		else if(a->sspecies->smell_clarity != 0.0f) s += " It has a sense of smell.";
		if(a->sspecies->vis_range >= 8) s += " It can see well.";
		else if(a->sspecies->vis_range >= 4) s+= " It can just about see.";

	}
	pretty_print(s);
	getch();
	draw_scene();
}

void kill_actor(actor* a)
{
	map_occupants[a->x][a->y] = NULL;
	if(current_target == a) current_target = NULL;
	a->dead = true;
	//if(sym_los_in_range(p_ptr->x,p_ptr->y,a->x,a->y,is_opaque))
	if(tile_visible(a->x,a->y))
	{
		add_message("The "+a->sspecies->name+" dies.");
	}
	a->sspecies->health_known = true;
	add_junk(a->sspecies->name +" corpse","the bloody corpse of a "+a->sspecies->name,'%',a->sspecies->colour,a->x,a->y);
}


int calculate_score()
{
	int score = 0;
	score += died ? -1000 : 1000;
	score += left_ship ? 50 : -50;
	score += ran ? 75 : -75;
	score += entered_building_destroyed_transmitter ? 100 : -100;
	score += destroyed_transmitter ? 200 : -200;
	score += returned_to_ship ? 400 : -400;
	score += closed_doors ? 600 : -600;
	score += left_planet ? 800 : -800;

	score += count_inv_devices(LOW_EXPLOSIVE)*100;
	score += count_inv_devices(HIGH_EXPLOSIVE)*400;
	score += count_inv_devices(HOLOGRAM_PROJECTOR)*50;
	score += count_inv_devices(NOISE_GENERATOR)*50;
	score += count_inv_devices(SCENT_GENERATOR)*50;
	score += count_inv_devices(BRAIN_SLICE)*50;

	score += ammo[PISTOL]*5;
	score += ammo[RIFLE]*3;
	score += ammo[CANNON]*25;
	score -= turn_count;
	score *= difficulty;
	return score;
}

void add_score_str()
{
	check_achievements();
	char score_str[10] = "";
	sprintf(score_str,"%d",calculate_score());
	//itoa(calculate_score(),score_str,10);
	add_message("Your score is " + std::string(score_str) + ".");
}

void lose()
{
	died = true;
	
	add_message("You die. Press 'Q' to quit.");
	
	add_score_str();
	draw_scene();
	int kp = 0;
	while(kp != 'Q') kp = fgetch();
	endwin();
	exit(0);

}

void win()
{
	if(map_terrain[ship_x+2][ship_y] == DOOR)
	{
		add_message("You close the doors and fly away! Press Q to quit.");
	}
	else
	{
		add_message("You wipe some blood off the control panel and fly away! Press Q to quit.");
	}
	check_achievements();
	closed_doors = true;
	left_planet = true;
	add_score_str();
	map_terrain[ship_x+2][ship_y] = NS;
	draw_scene();
	int kp = 0;
	while(kp != 'Q') kp = fgetch();
	endwin();
	exit(0);

}

void hurt_actor(actor* a,int damage)
{
	if(!a->is_hologram)
	{
	a->health -= damage;
	if(a->health <= 0)
	{
		if (a->is_player) {lose();}
		else kill_actor(a);
	}
	}
}

void do_explosion(explosion ex)
{
	make_noise(ex.x,ex.y,&(ex.radius <3?LE_noise:HE_noise));
	add_message("BOOM!");
	int dam = 0;
	for(int i = -ex.radius; i<= ex.radius; i++)
	{
		for(int j = -ex.radius; j<= ex.radius; j++)
		{
			if(!on_map(ex.x+i,ex.y+j)) continue;
			int d = d22(0,0,i,j);
			dam = (d <= squarei(ex.radius)+1) ? (int)ceil((float)ex.centre_damage/(d+1)) : 0;
			if(dam)
			{
				if(map_items[ex.x+i][ex.y+j] != NULL)
				{
					if(map_items[ex.x+i][ex.y+j]->name == "radio transmitter")
					{
						transmitter_destroyed = true;
						map_items[ex.x+i][ex.y+j] = NULL;
						target_x = -500; target_y = -500;
						add_message("Perhaps the ship will stop pretending to be docked now!");
						destroyed_transmitter = true;

					}
				}
				if(map_occupants[ex.x+i][ex.y+j] != NULL)
				{
					if(map_occupants[ex.x+i][ex.y+j]->is_player)
					{
						add_message("You are hit by the explosion!");
					}
					hurt_actor(map_occupants[ex.x+i][ex.y+j],dam);
				}
			}
			switch(map_terrain[ex.x+i][ex.y+j])
			{
				case WALL:
					if(dam >= 7)
						map_terrain[ex.x+i][ex.y+j] = FLOOR; break;
				case BTREE:
					if(dam >= 11)
						map_terrain[ex.x+i][ex.y+j] = DIRT; break;
				case STREE:
					if(dam >= 3)
						map_terrain[ex.x+i][ex.y+j] = DIRT; break;
				case TWINDOW:
					if(dam >= 5)
						map_terrain[ex.x+i][ex.y+j] = FLOOR; break;
			}
		}
	}
}

void melee_attack(actor* attacker, actor* defender)
{
	add_message("The "+attacker->sspecies->name+" claws you.");
	attacker->sspecies->damage_known = true;
	hurt_actor(defender,attacker->sspecies->damage);
}

bool fire_weapon(int x1, int y1, int x2, int y2)
{
	if(los_exists(x1,y1,x2,y2,blocks_shots))
	{
		add_message("BANG!");
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
		actor* victim = map_occupants[x2][y2];
		bool crit = false;
		if(victim != NULL)
		{
			species* vsp = victim->sspecies;

			int bss = vsp->body_sections;
			int bst = vsp->body_sections_tried;
			int bsr = bss - bst; // body sections remaining. 1 if crit_known
			int bs = rand()%bsr; // aim at a body section that could be the crit section
			if(rand()%3) // miss
			{
				bs = rand()%bss; // hit anywhere
			}
			crit = (bs == 0);
			if(crit) {vsp->body_sections_tried = vsp->body_sections - 1;} // if you crit, you learn how
			else if(bs < bsr) {vsp->body_sections_tried += 1;}
			if(vsp->body_sections_tried == vsp->body_sections - 1) {vsp->crit_known = true;}
			if(vsp->body_sections_tried >= vsp->body_sections) {vsp->body_sections_tried = vsp->body_sections - 1;} // just make sure
		}
		
		if(current_weapon == CANNON)
		{
			if(crit)
			{
				add_message("The "+victim->sspecies->name+" is hit in the "+victim->sspecies->crit_part_name+"!!!");
				hurt_actor(victim,damage);
			}
			explosion ex;
			ex.x = x2; ex.y = y2;
			ex.centre_damage = damage; ex.radius = 1;
			do_explosion(ex);
		}
		else
		if(victim != NULL)
		{
			if(crit)
			{
				add_message("The "+victim->sspecies->name+" is hit in the "+victim->sspecies->crit_part_name+"!!!");
				hurt_actor(victim,2*damage);
			}
			else
			{
			add_message("The "+victim->sspecies->name+" is hit!");
			hurt_actor(victim,damage);
			}
			return true;
		}
	}
	else
	{
		add_message("Can't get a clear shot!"); 
		draw_last_msg();
		return false;
	}
	if(target_x == x2 && target_y == y2)
	{
		if(current_weapon == PISTOL || current_weapon == RIFLE)
		{
			add_message("Use explosives to destroy the transmitter!");
		}
	}
	return true;
}

void scent_walk(actor* a)
{
	std::pair<int,int> valid_dirs[8];
	int dir_count = 0;
	int best_x = 0; int best_y = 0;
	float best_scent = map_human_scent[a->x][a->y];
	random_shuffle(vdirections.begin(),vdirections.end());
	for(int d=0;d<8;d++)
	{
		std::pair<int,int> dir = directions[d];
		if(!terrain_immovable(a,a->x+dir.first,a->y+dir.second))
		{
			float detect = map_human_scent[a->x+dir.first][a->y+dir.second] + randfloat()*map_human_scent[a->x][a->y]*0.001f;
			if(detect >= best_scent)
			{
				best_scent = detect;
				best_x = dir.first; best_y = dir.second;
			}
		}
	}
	move_actor(a,best_x,best_y);
}

void random_walk(actor* a)
{
	std::pair<int,int> valid_dirs[8];
	int dir_count = 0;
	for(int d=0;d<8;d++)
	{
		std::pair<int,int> dir = directions[d];
		if(is_wall(a->x+dir.first,a->y+dir.second))
		{
			valid_dirs[dir_count] = dir;
			dir_count++;
		}
		if(dir_count != 0)
		{
			int choice = rand()%dir_count;
			move_actor(a,valid_dirs[choice].first,valid_dirs[choice].second);
		}
	}
}

void hologram_turn(actor* a)
{
	
}

std::pair<int,int> overshoot(int x,int y,int xx,int yy)
{
	int dx = xx-x; int dy = yy-y;
	return std::make_pair(xx+dx,yy+dy);
}
void set_ai_target(actor* a, std::pair<int,int> target)
{
	a->ai_x = target.first; a->ai_y = target.second;
}


void ai_turn(actor* a)
{
	if(dinf(a->x,a->y,p_ptr->x,p_ptr->y) == 1)
	{
		a->ai_x = p_ptr->x; a->ai_y = p_ptr->y;
		melee_attack(a,p_ptr);
		return;
	}
	bool uncertain = true;

	if(rand()%10==0) {a->runaway_x = -500; a->runaway_y = -500;}
	bool not_scared = (a->runaway_x == -500 && a->runaway_y == -500);

	if(a->sspecies->psychic_range)
	{
		std::vector<std::pair<int,int>> psychables;
		
		for (unsigned int i=0;i<devices.size();i++)
		{
			if(devices[i]->device_type == BRAIN_SLICE && devices[i]->current_action == HUMAN_STIMULUS)
			{
				float d2 = sqrt((float)d22(a->x,a->y,devices[i]->x,devices[i]->y));
				if(d2<=(float)a->sspecies->psychic_range && randfloat() < squaref(1.0f + ((float)a->sspecies->psychic_range/6 - d2) /
					(float)(a->sspecies->psychic_range - a->sspecies->psychic_range/6)))
				{
					psychables.push_back(std::make_pair(devices[i]->x,devices[i]->y));
				}
			}
		}
		float d2 = sqrt((float)d22(a->x,a->y,p_ptr->x,p_ptr->y));
		if(d2<=(float)a->sspecies->psychic_range && randfloat() < squaref(1.0f + ((float)a->sspecies->psychic_range/6 - d2) /
			(float)(a->sspecies->psychic_range - a->sspecies->psychic_range/6)))
		{
			psychables.push_back(std::make_pair(p_ptr->x,p_ptr->y));
		}
		if (psychables.size())
		{
			std::pair<int,int> pt = psychables[rand()%psychables.size()];
			set_ai_target(a,overshoot(a->x,a->y,pt.first,pt.second));
			//a->ai_x = p_ptr->x; a->ai_y = p_ptr->y;
			uncertain = false;
			a->certain = true;
		}
	}
	if(uncertain/* && d22(a->x,a->y,p_ptr->x,p_ptr->y) <= squarei(a->sspecies->vis_range)*/)
	{
		std::vector<actor*> visible;
		if(d22(a->x,a->y,p_ptr->x,p_ptr->y) <= squarei(a->sspecies->vis_range))
		{
			if(symmetrical_los(a->x,a->y,p_ptr->x,p_ptr->y,is_opaque))
			{
				visible.push_back(p_ptr);
			}
		}
		for(unsigned int i = 0; i<holograms.size();i++)
		{
			if(d22(a->x,a->y,holograms[i]->x,holograms[i]->y) <= squarei(a->sspecies->vis_range))
			{
				if(symmetrical_los(a->x,a->y,holograms[i]->x,holograms[i]->y,is_opaque))
				{
					visible.push_back(holograms[i]);
				}
			}
		}
		if(visible.size())
		{
			actor* vis_target = visible[rand()%visible.size()];

			//if(symmetrical_los(a->x,a->y,vis_target->x,vis_target->y,is_opaque)) //already checked this once
			{
				set_ai_target(a,overshoot(a->x,a->y,vis_target->x,vis_target->y));
				if((vis_target->running ? vis_target->sspecies->run_energy : vis_target->sspecies->walk_energy) >= a->sspecies->walk_energy
					&& a->stamina > a->sspecies->stamina/4)
				{
					a->running = true;
				}
				uncertain = false;
				a->certain = true;
			}
		}
	}
	//if(uncertain && a->sspecies->hearing && rand()%10 < 3)
	//{
	//	a->ai_x = p_ptr->x; a->ai_y = p_ptr->y;
	//	uncertain = false;
	//}
	if(a->x==a->ai_x && a->y ==a->ai_y)
	{
		a->running = false;
		a->certain = false;
		if(a->sspecies->smell_clarity)
		{
			scent_walk(a); return;
		}
		random_walk(a); return;
	}
	if(!uncertain && rand()%4==0)
	{
	make_noise(a->x,a->y,&screech);
	}
	int dx = (a->x) - (a->ai_x);
	int dy = (a->y) - (a->ai_y);
	int best = d1(a->x,a->y,a->ai_x,a->ai_y)+2;
	int bestscare = d1(a->x,a->y,a->runaway_x,a->runaway_y);
	int bestdx = 0; int bestdy = 0;
	for(int d=0;d<8;d++)
	{
		std::pair<int,int> dir = directions[d];
		int ddx = dir.first; int ddy = dir.second;
		if (!terrain_immovable(a,a->x+ddx,a->y+ddy))
		{
			int dist = d1(a->x+ddx,a->y+ddy,a->ai_x,a->ai_y);
			int scaredist = d1(a->x+ddx,a->y+ddy,a->runaway_x,a->runaway_y);
			if (dist < best && (not_scared || scaredist <= bestscare))
			{best = dist; bestdx = ddx; bestdy = ddy;}
			else if (dist == best)
			{
				int ddist = dinf(a->x+ddx,a->y+ddy,a->ai_x,a->ai_y);
				int ddist2 = dinf(a->x+bestdx,a->y+bestdy,a->ai_x,a->ai_y);
				int ddistscare = dinf(a->x+ddx,a->y+ddy,a->runaway_x,a->runaway_y);
				int ddistscare2 = dinf(a->x+ddx,a->y+ddy,a->runaway_x,a->runaway_y);
				if (ddist < ddist2 && (not_scared || ddistscare >= ddistscare2))
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
				if (!map_seen[x][y])
				{
					if(tile_visible(x,y))
						map_seen[x][y] = true;
				}
			}
		}
	}
}

//std::string signal_strength(int x,int y)
//{std::string ss = "The radio signal is ";
//	int d2 = d22(x,y,target_x,target_y);
//	if((float)sqrt((float)d2) < 9+rand()%3)
//		return ss+ "coming from very close by!";
//	else if ((float)sqrt((float)d2) < 18 + rand()%6)
//		return ss+"very strong!";
//	else if ((float)sqrt((float)d2) < 36 + rand()%12)
//		return ss+"quite strong.";
//	else if ((float)sqrt((float)d2) < 72 + rand()%24)
//		return ss+ "quite weak.";
//	else if ((float)sqrt((float)d2) < 400)
//		return ss+"very weak...";
//	return "The radio transmitter is destroyed. Home time!";
//}
std::string signal_receive(int x,int y)
{
	if(target_x==-500) return "The radio transmitter is destroyed. Home time!";
	std::string ss = vtransmission[rand()%vtransmission.size()];
	float d2 = sqrt((float)d22(x,y,target_x,target_y));
	float signal = squaref(1.0f + ((float)10 - d2) /90.0f);
	std::string tt = "";
	for(unsigned int i=0;i<ss.length();i++)
		tt += (randfloat()<signal)?ss[i]:'*';
	return "\""+tt+"\"";
}

int select_device()
{
	attron(COLOR_PAIR(COLOR_WHITE));
	clear_area(0,0,viewport_width,4);
	mvaddstr(0,0, padmsg("Which kind of device? Q, q or ESC to cancel.").c_str());
	mvaddstr(1,0, padmsg(std::string(count_inv_devices(LOW_EXPLOSIVE)?"a. Low explosive, ":"") +
		std::string(count_inv_devices(HIGH_EXPLOSIVE)?"b. High explosive, ":"")).c_str());
	mvaddstr(2,0,padmsg(std::string(count_inv_devices(HOLOGRAM_PROJECTOR)?"c. Hologram projector,":"")+
		std::string(count_inv_devices(NOISE_GENERATOR)?"d. Noise generator, ":"")).c_str());
	mvaddstr(3,0,padmsg(std::string(count_inv_devices(SCENT_GENERATOR)?"e. Scent generator, ":"")+
		std::string(count_inv_devices(BRAIN_SLICE)?"f. Brain slice":"")).c_str());
	bool unacceptable = true;
	int device_num;
	int kp;
	while(unacceptable)
	{
		kp = fgetch();
		if(kp >= 'a' && kp <= 'f')
		{
			device_num = kp - 'a';
			if(count_inv_devices(device_num))
			{unacceptable = false;}
		}
		if(kp == 'q' || kp == 'Q' || kp == KEY_ESC) 
		{
				device_num = -1;
				unacceptable = false;
		}
	}
	draw_scene();
	return device_num;
}

int select_configuration(int dev_type)
{
	attron(COLOR_PAIR(COLOR_WHITE));
	clear_area(0,0,viewport_width,2);
	mvaddstr(0,0,padmsg("Use a preconfigured device? Q or ESC to cancel.").c_str());
	mvaddstr(1,0,std::string(viewport_width,' ').c_str());
	mvprintw(1,0,"(a) %d unconfigured, (b) %d timer, (c) %d remote",dev_inv[dev_type][UNCONFIGURED].size(),
		dev_inv[dev_type][ON_TIMER].size(),dev_inv[dev_type][ON_REMOTE].size());
	
	bool unacceptable = true;
	int config;
	while(unacceptable)
	{
		int kp = fgetch();
		config = kp - 'a';
		if(0 <= config && config <= DEVICE_CONFIGURATIONS)
		{
			if(dev_inv[dev_type][config].size()) unacceptable = false;
		}
		else if(kp == 'q' || kp == 'Q' || kp == KEY_ESC)
		{
			config = -1; unacceptable = false;
		}
	}
	draw_scene();
	return config;
}

item* selectghjk_individual_device_from_type_and_config(int type, int config)
{
	bool unacceptable = true;
	std::vector<item*>* options = &dev_inv[type][config];
	attron(COLOR_PAIR(COLOR_WHITE));
	clear_area(0,0,viewport_width,options->size());
	for(unsigned int i=0;i<options->size();i++)
	{
		mvprintw(i,0,"(%c) %s with %d seconds remaining",'a'+i, (*options)[i]->name.c_str(), (*options)[i]->time_remaining);
	}

	int kp = fgetch();
	while(unacceptable)
	{
		unacceptable = false;
	}
	draw_scene();
	return (*options)[kp-'a'];
}

int ask_for_time()
{
	clear_area(0,0,viewport_width,1);
	mvaddstr(0,0,"Choose delay (0 to 9 seconds). q or ESC to cancel");
	int t = -2;
	while(t == -2)
	{
		int kp = fgetch();
		if(kp == 'q' || kp == 'Q' || kp == KEY_ESC)
		{
			t = -1;
		}
		else if(kp >='0' && kp <= '9')
		{
			t = kp - '0';
		}
	}
	draw_scene();
	return t*10;
}

int ask_for_wavelength()
{
	clear_area(0,0,viewport_width,3);
	mvaddstr(0,0,"Use which wavelength? Choosing a wavelength");
	mvaddstr(1,0,"already in use will destroy the existing link.");
	mvaddstr(2,0,"q or ESC to cancel");
	for (int i=0;i<WAVELENGTHS;i++)
	{
		item* a = rm_devices[i];
		if (a==NULL)
		{
			mvprintw(i+3,0,"(%d) no device detected",i+1);
		}
		else
		{
			mvprintw(i+3,0,"(%d) %s", a->name.c_str());
		}
	}
	int w = -2;
	while(w == -2)
	{
		int kp = fgetch();
		if(kp == 'q' || kp == 'Q' || kp == KEY_ESC)
		{
			w = -1;
		}
		else if(kp >'0' && kp <= '0'+WAVELENGTHS)
		{
			w = kp - '1';
		}
	}
	draw_scene();
	return w;
}

void set_unconfigured(item* dev)
{
	if(dev->player_has && dev->configuration != UNCONFIGURED)
	{
		inv_remove(dev);
		dev_inv[dev->device_type][UNCONFIGURED].push_back(dev);
	}
	if(dev->configuration == ON_TIMER)
	{
		dev->time_remaining = -1;
		kill_velement(timer_devices,dev);
	}
	else if(dev->configuration == ON_REMOTE)
	{
		rm_devices[dev->wavelength] = NULL;
		dev->wavelength = -1;
	}
	dev->configuration = UNCONFIGURED;
}

void set_timer(item* dev,int t)
{
	if(dev->configuration != ON_TIMER)
	{
		set_unconfigured(dev);
		timer_devices.push_back(dev);
		//if(dev->player_has)
		//{
		//	//inv_remove(dev);
		//	//dev_inv[dev->device_type][ON_TIMER].push_back(dev);
		//}
	}
	//if(dev->configuration == ON_REMOTE)
	//{
	//	rm_devices[dev->wavelength] = NULL;
	//	dev->wavelength = -1;
	//}
	if(dev->device_type == NOISE_GENERATOR)
	{
		dev->next_action = rand()%2+1;
	}
	dev->configuration = ON_TIMER;
	dev->time_remaining = t;
}

void set_remote(item* dev,int w)
{
	if(dev->player_has && dev->configuration != ON_REMOTE)
	{
		set_unconfigured(dev);
		inv_remove(dev);
		dev_inv[dev->device_type][ON_REMOTE].push_back(dev);
	}
	//if(dev->configuration == ON_TIMER)
	//{
	//	dev->time_remaining = -1;
	//	kill_velement(timer_devices,dev);
	//}
	dev->configuration = ON_REMOTE;
	dev->wavelength = w;
}

bool reconfigure(item* a)
{
	update_device_desciption(a);

	bool unacceptable = true;
	bool cancel = false;
	while(unacceptable)
	{
		std::string s = a->description+" Reconfigure? Space or ("+std::string(1,a->configuration +'a')+
			". Q or ESC to cancel.";
		if(a->configuration != UNCONFIGURED)
		{s += std::string(" (a) remove ")+(a->configuration==ON_TIMER?"timer":"remote control");}
		if(a->configuration != ON_TIMER)
		{s += std::string(" (b) use a timer instead");}
		if(a->configuration != ON_REMOTE)
		{s += std::string(" (c) use a remote control instead");}
		s += ".";
		pretty_print(s);
		/*clear_area(0,0,viewport_width,5);
		mvaddstr(0,0,a->description.c_str());
		mvprintw(1,0,"Reconfigure? Space or (%c) to leave unchanged",a->configuration +'a');
		mvaddstr(2,0,"q or ESC to cancel");
		int i=3;
		if(a->configuration != UNCONFIGURED)
		{
			mvprintw(i,0,"(a) remove %s",a->configuration==ON_TIMER?"timer":"remote control");
			i++;
		}
		if(a->configuration != ON_TIMER)
		{
			mvprintw(i,0,"(b) use a timer instead");
			i++;
		}
		if(a->configuration != ON_REMOTE)
		{
			mvprintw(i,0,"(c) use a remote control instead");
		}*/
		int kp = fgetch();
		draw_scene();
		if(kp == 'q' || kp == 'Q' || kp == KEY_ESC)
		{
			unacceptable = false;
			cancel = true;
		}
		else if(kp == a->configuration + 'a' || kp == ' ')
		{
			unacceptable = false;
		}
		else if(kp == 'a') 
		{
			set_unconfigured(a);
			unacceptable = false;
		}
		else if( kp == 'b')
		{
			draw_scene();
			int t = ask_for_time();
			
			if (t >= 0)
			{
				set_timer(a,t);
				unacceptable = false;
			}
		}
		else if (kp == 'c')
		{
			draw_scene();
			int w = ask_for_wavelength();
			if(w>=0)
			{
				set_remote(a,w);
				unacceptable = false;
			}
		}
	}
	draw_scene();
	return cancel;
}

item* select_individual_device()
{
	if(total_inv_devices())
	{

		int device_type = 999;
		while(device_type != -1)
		{
			device_type = select_device();
			if(device_type >= 0)
			{
				
				/*int t = ask_for_time();
			
				if (t >= 0)*/
				{
					item* dev = dev_inv[device_type][UNCONFIGURED].back();
					/*set_timer(dev,t);
					dev_inv[device_type][UNCONFIGURED].pop_back();*/
					return dev;
				}
			}
				//else
				//{
				//	return NULL;
				//}
				/*}
			{
				int device_configuration = 999;
				while(device_configuration != -1)
				{
					device_configuration = select_configuration(device_type);
					if(device_configuration != -1)
					{
						item* dev;
						if(device_configuration == ON_TIMER)
						{
							dev = select_individual_device_from_type_and_config(device_type,device_configuration);
						}
						else
						{
							dev = dev_inv[device_type][device_configuration].back();
							dev_inv[device_type][device_configuration].pop_back();
						}
						if(dev != NULL)
						{
							if(!reconfigure(dev))
							{return dev;}
							else 
							{return NULL;}

						}
					}
				}
				if(device_configuration == -1) break;
			}*/
		}
	}
	else
	{
		add_message("Nothing to throw!"); draw_last_msg();
	}
	return NULL;
}

int autotarget()
{
	int tab_ind = -1;
	if(current_target == NULL || current_target == p_ptr)
	{
		if(visible_actors.size() == 0)
		{
			current_target = p_ptr;
		}
		else
		{
			current_target = visible_actors[0];
			tab_ind = 0;
		}
	}
	if(!symmetrical_los(current_target->x,current_target->y,p_ptr->x,p_ptr->y,is_opaque))
	{
		if(visible_actors.size() == 0)
		{
			current_target = p_ptr;
		}
		else
		{
			current_target = visible_actors[0];
			tab_ind = 0;
		}
	}
	return tab_ind;
}

void player_turn(actor* a)
{
	update_map_seen();
	//draw_scene(true);
	int turn = -1;
	while(turn)
	{
		draw_scene(true);
		int kp;
		if(!forcefed_help)
		{
			kp = '?';
			//forcefed_help = true;
		}
		else
		{
			kp = fgetch();
		}
		switch(kp)
		{
		case KEY_UP: case 'k': case '8': case KEY_A2:
			turn = move_actor(a,0,-1); break;
		case KEY_DOWN: case 'j':case '2': case KEY_C2:
			turn = move_actor(a,0,1); break;
		case KEY_LEFT: case 'h': case '4': case KEY_B1:
			turn = move_actor(a,-1,0); break;
		case KEY_RIGHT: case 'l': case '6': case KEY_B3:
			turn = move_actor(a,1,0); break;
		case 'y':case '7': case KEY_A1:
			turn = move_actor(a,-1,-1); break;
		case 'u':case '9': case KEY_A3:
			turn = move_actor(a,1,-1); break;
		case 'b':case '1': case KEY_C1:
			turn = move_actor(a,-1,1); break;
		case 'n':case '3': case KEY_C3:
			turn = move_actor(a,1,1); break;
		case '.': case '5': case KEY_B2:
			turn = 0; break;
		case '?':
			{
				int kp = '?';
				int page = 0;
				while(kp == '?')
				{
					switch(page)
				{
					case 4:
					page = 0;
					case 0:
				pretty_print
				(
				/*"01234567890123456789012345678901234567890123456789"*/
				  "                                                  "
				  "      Find and destroy the radio transmitter      "
				  "            so that you can escape!               "
				  "                                                  "
				  "Movement: y/7  k/8  u/9  Fire:             f      "
				  "hjklyubn     \\  |  /     Throw:            t      "
				  "42867913      \\ | /      Run:              R      "
				  "          h/4--5/.--l/6  Check radio:      r      "
				  "              / | \\      Wield pistol      X      "
				  "             /  |  \\     Wield rifle       Y      "
				  "          b/1  j/2  n/3  Wield cannon      Z      "
				  "                         Mute advice       M      "
				  "                         Close ship doors  C      "
				  "               fire or //Autotarget:     tab      "
				  "               throw   ||Manual target: movement  "
				  "               mode    \\\\Info on tile:     i      "
				  "                         Quit game:        Q      "
				  "                         See this help:    ?      "
				  "                                                  "
				  "        Press ? again for more info.              "
				  "   You might also want to read the README.        "
				  "                                                  "
				); break;
				case 1:
				pretty_print
					(
				/*"01234567890123456789012345678901234567890123456789"*/
				  "Your guns:                           f to fire    "
				  " Pistol (low damage),                             "
				  " Rifle (med damage),                              "
				  " Cannon (high splash damage)                      "
				  "Your devices:                        t to throw   "
				  " Low explosives (rad. 2),                         "
				  " High explosives (rad. 4),                        "
				  " Distractions:                                    "
				  "  Hologram (C), Noise (D), Scent (E),             "
				  "  Brain slice (F) (for psychic aliens)            "
				  " Set a timer from 0 to 9 when throwing.           "
				  " All of these have very useful spy cameras.       "
				  "                                                  "
				  "       Press ? again for more info.               "
				  "                                                  "
					);break;
				case 2: pretty_print
			(
				/*"01234567890123456789012345678901234567890123456789"*/
				  "Monster memory:                                   "
				  "No-one has seen this planet or these aliens       "
				  "before and lived to tell the tale.                "
				  "To survive, you keep a note of your observations  "
				  "In the status panel.                              "
				  "The first column is size (small, medium, huge),   "
				  "The second is walk speed (slow, average, fast)    "
				  "   (relative to your walk speed)                  "
				  "The third is max run speed (slow, average, fast)  "
				  "   (relative to your max run speed)               "
				  "   (as stamina runs out, you get slower)          "
				  "The fourth is damage (weak, strong, dangerous)    "
				  "The last is health (frail or tough)               "
				  "Frail aliens are very easy to kill.               "
				  "Some tough aliens are easier to kill than others  "
				  "                                                  "
				  "     Press ? again for more info.                 "
				  "                                                  "
			);break;
				case 3:
					pretty_print
			(
				/*"01234567890123456789012345678901234567890123456789"*/
				  " Your ship is claiming, despite the surrounding   "
				  "   trees and hostile aliens, to be safely docked "
				  "    at a space station.                           "
				  "  You need to fix this malfunction and get off    "
				  "    this planet.                                  "
				  "  The ship uses automated radio signals to dock,  "
				  "    so there must be a radio transmitter here.    "
				  "  You can use your portable radio to try to find  "
				  "    the source of the transmissions,              "
				  "    then disable it with explosives and escape.   "
				  "                                                  "
				  "       Press ? again for more info.               "
				  "                                                  "
			);break;
				}
				page++;
				kp = fgetch();
				draw_scene();
				//clear_area(0,0,viewport_width,message_area_height+1);
				if(!forcefed_help && page != 4)
				{
					kp = '?';
					if(page == 3) {forcefed_help = true;}
				}
				}
			}break;
		case 'X':
			if(current_weapon != PISTOL) {turn = 0; add_message("You wield your pistol."); current_weapon = PISTOL;} break;
		case 'Y':
			if(current_weapon != RIFLE) {turn = 0; add_message("You wield your automatic rifle!"); current_weapon = RIFLE;} break;
		case 'Z':
			if(current_weapon != CANNON) {turn = 0; add_message("You get out the plasma cannon!!"); current_weapon = CANNON;} break;
		case 'r':
			turn = 0; add_message(signal_receive(p_ptr->x,p_ptr->y)); /*draw_last_msg();*/ break;
		case 'g':
			if(count_inv_devices(BRAIN_SLICE) > 0)
			{
				add_message("You wash a brain slice with Tx-PBS three times with gentle shaking.");
				draw_last_msg();
			}
			break;
		case 'Q':
			pretty_print("Are you sure you want to quit? press Y if you are.");
			if(fgetch() == 'Y')
			{
				endwin();
				exit(0);
			}
			break;
		case 'd':break;
			debug = !debug; break;
		case ' ':
			last_message_seen = rlmessages.size();
			last_word_seen  = rlmessage_words.size()-1;
			draw_last_msg(); break;
		case 'R':
			p_ptr->running = !p_ptr->running; draw_stats(); break;
		case 't':
			{

				current_mode = THROW_MODE;
				draw_scene();
				
				int tab_ind = autotarget();
				int lx = current_target->x; int ly = current_target->y;
				item* dev = select_individual_device();
				//item* dev = dev_inv[device_type][UNCONFIGURED].back();
				int t = ask_for_time();
				if(dev == NULL) break;
			while(current_mode==THROW_MODE)
			{
				draw_scene();
				draw_tile_description(lx,ly);
				draw_line(p_ptr->x,p_ptr->y,lx,ly,'*');
				clear_area(0,0,viewport_width,1);
				attron(COLOR_PAIR(COLOR_WHITE));
				mvaddstr(0,0,"Tab or movement to target, t to throw");
				int kpp = fgetch();
				undraw_line(p_ptr->x,p_ptr->y,lx,ly);
				undraw_tile_description();
				switch(kpp)
				{
				case '\t':
					if(visible_actors.size() > 0)
					{
						tab_ind = (tab_ind + 1)%visible_actors.size();
						lx = visible_actors[tab_ind]->x;
						ly = visible_actors[tab_ind]->y;
					}
					break;
				case KEY_UP: case 'k': case '8': case KEY_A2:
					ly = std::max(ly-1,viewport_top_edge()); break;
				case KEY_DOWN: case 'j': case '2': case KEY_C2:
					ly = std::min(ly+1,viewport_bottom_edge()); break;
				case KEY_LEFT: case 'h': case '4': case KEY_B1:
					lx = std::max(lx-1,viewport_left_edge()); break;
				case KEY_RIGHT: case 'l': case '6': case KEY_B3:
					lx = std::min(lx+1,viewport_right_edge()); break;
				case 'y': case '7': case KEY_A1:
					lx = std::max(lx-1,viewport_left_edge()); ly = std::max(ly-1,viewport_top_edge()); break;
				case 'u': case '9': case KEY_A3:
					lx = std::min(lx+1,viewport_right_edge()); ly = std::max(ly-1,viewport_top_edge()); break;
				case 'b': case '1': case KEY_C1:
					lx = std::max(lx-1,viewport_left_edge()); ly = std::min(ly+1,viewport_bottom_edge()); break;
				case 'n': case '3': case KEY_C3:
					lx = std::min(lx+1,viewport_right_edge()); ly = std::min(ly+1,viewport_bottom_edge()); break;
				case 't':
					{
						if(los_exists(p_ptr->x,p_ptr->y,lx,ly,blocks_shots))
						{
							//if(dev != NULL)
							{
								item_to_location(dev,lx,ly);
								inv_remove(dev);
								set_timer(dev,t);
								turn = 0;
								current_mode = WALK_MODE;
								dev->player_has = false;
							}
						}
						else
						{
							add_message("There's too much in the way!");
							draw_last_msg();
						}
					}
					break;
				case KEY_ESC:
					 current_mode = WALK_MODE; draw_scene(); break;
				case 'i':
					draw_info(lx,ly);
					current_mode = THROW_MODE; draw_stats();
					draw_line(p_ptr->x,p_ptr->y,lx,ly,'*');break;
				default: ;
				}
			}
			}
			break;
		case 'M':
			cycle_advice = !cycle_advice;
			draw_stats();
			break;
		case 'C':
			if(door_closable())
			{
				if(map_terrain[ship_x+2][ship_y] == DOOR)
				{
					map_terrain[ship_x+2][ship_y] = NS;
					turn = 0;
					add_message("You tell the computer to close the doors.");
					closed_doors = true;
				}
				else
				{
					add_message("Not going to open them again. No way.");
				}
			}
			else {add_message("Can't close the doors right now.");}
			break;
		case 'f':
		{
			current_mode = FIRE_MODE;
			draw_scene();
			int tab_ind = autotarget();
			int lx = current_target->x; int ly = current_target->y;
				
			
			while (current_mode == FIRE_MODE)
			{
				draw_tile_description(lx,ly);
				draw_line(p_ptr->x,p_ptr->y,lx,ly,'*');
				int kpp = fgetch();
				undraw_line(p_ptr->x,p_ptr->y,lx,ly);
				undraw_tile_description();
				
				switch(kpp)
				{
				case '\t':
					if(visible_actors.size() > 0)
					{
						tab_ind = (tab_ind + 1)%visible_actors.size();
						lx = visible_actors[tab_ind]->x;
						ly = visible_actors[tab_ind]->y;
					}
					break;
				case KEY_UP: case 'k': case '8': case KEY_A2:
					ly = std::max(ly-1,viewport_top_edge()); break;
				case KEY_DOWN: case 'j': case '2': case KEY_C2:
					ly = std::min(ly+1,viewport_bottom_edge()); break;
				case KEY_LEFT: case 'h': case '4': case KEY_B1:
					lx = std::max(lx-1,viewport_left_edge()); break;
				case KEY_RIGHT: case 'l': case '6': case KEY_B3:
					lx = std::min(lx+1,viewport_right_edge()); break;
				case 'y': case '7': case KEY_A1:
					lx = std::max(lx-1,viewport_left_edge()); ly = std::max(ly-1,viewport_top_edge()); break;
				case 'u': case '9': case KEY_A3:
					lx = std::min(lx+1,viewport_right_edge()); ly = std::max(ly-1,viewport_top_edge()); break;
				case 'b': case '1': case KEY_C1:
					lx = std::max(lx-1,viewport_left_edge()); ly = std::min(ly+1,viewport_bottom_edge()); break;
				case 'n': case '3': case KEY_C3:
					lx = std::min(lx+1,viewport_right_edge()); ly = std::min(ly+1,viewport_bottom_edge()); break;
				case 'f':
					if(ammo[current_weapon] == 0)
					{add_message("Can't fire this weapon, no ammo.");draw_last_msg();}
					else if(lx==p_ptr->x && ly==p_ptr->y)
					{
						add_message("Don't shoot yourself!");
						draw_last_msg();
					}
					else
					{
						if(fire_weapon(p_ptr->x,p_ptr->y,lx,ly))
						{
							current_mode = WALK_MODE;
							turn = 0;
						}
					}
					break;
				case KEY_ESC:
					 current_mode = WALK_MODE; 
					 draw_scene();
					 break;
				case 'i':
					draw_info(lx,ly);
					current_mode = FIRE_MODE; draw_stats();
					draw_line(p_ptr->x,p_ptr->y,lx,ly,'*');
				default:;
				}
			}
			if(map_occupants[lx][ly] != NULL)
			{
				current_target = map_occupants[lx][ly];
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
					std::pair<int,int> dir = directions[d];
					int xx = x+dir.first; int yy = y+dir.second;
					if(!undiffusable(xx,yy))
					{
						spare_scent_map[x][y] += map_human_scent[xx][yy];
						count++;
					}
				}
				map_human_scent[x][y] = 0.9999f*spare_scent_map[x][y]/count;
			}
		}
	}
}

int play_turn()
{
		if(turn_count%100 == 50)
		{
			current_advice = rand()%vadvice.size();
		}
		for(std::vector<actor*>::iterator a = actors.begin(); a != actors.end(); ++a)
		{
			actor* act = (*a);
			if (act->sspecies->name == "human" && !act->is_player && !act->is_hologram)
			{
				map_human_scent[act->x][(*a)->y] += 0.7f; //non-player humans get an easier time >:)
			}
			if(!act->dead)
			{
				act->energy += act->sspecies->walk_energy;
				if(act->running)
				{
					if(act == p_ptr) ran = true;
					int stamina_consumed = (act->stamina * (act->sspecies->run_energy - act->sspecies->walk_energy))/act->sspecies->stamina;
					act->energy += std::min(stamina_consumed,act->stamina);
					act->stamina -=  std::min(stamina_consumed,act->stamina);
					if (stamina_consumed == 0)
					{
						if(act->is_player)
						{
							add_message("You are too exhausted to continue running.");
						}
						act->running = false;
					}
				}
				else if(act->energy > 100)
				{
					act->stamina += act->sspecies->stamina/70;
					act->stamina = std::min(act->stamina,act->sspecies->stamina);
				}
				if (act->is_player)
				{
					if(act->energy > 100)
					{
						if(win_condition())
						{
							win();
							
						}
						if(door_closable() && !door_closable_told)
						{
							add_message("Press 'C' to tell the computer to close the door (you can't take off with aliens in the ship, though).");
							door_closable_told = true;
						}
						act->energy -= 100;
						player_turn(act);
						check_achievements();

						act->most_noticeable_sound = 0.0;
						sound_this_turn = "";
						dir_this_turn = "";
						act->sound_threshold = std::max(act->sound_threshold-act->sspecies->hearing_adapt,act->sspecies->hearing_thres);

						visible_actors.clear();
					}
					if(!debug && !(turn_count %5))
					{
						map_human_scent[act->x][act->y] += 1000.0;
						diffuse_scent_human();
					}
					for(std::vector<item*>::iterator dev = timer_devices.begin(); dev != timer_devices.end();)
					{
						if((*dev)->time_remaining > 0)
						{
							(*dev)->time_remaining -= 1;
							++dev;
							//next_timer_devices.push_back(*dev);
						}
						else
						{
							if((*dev)->device_type == LOW_EXPLOSIVE || (*dev)->device_type == HIGH_EXPLOSIVE)
							{
								explode_this_turn.push_back(*dev);
							}
							else
							{
								active_devices.push_back(*dev);
								(*dev)->current_action = (*dev)->next_action;
							}
							dev = timer_devices.erase(dev);
						}
					}
					for(std::vector<item*>::iterator dev = active_devices.begin(); dev != active_devices.end(); ++dev)
					{
						if((*dev)->device_type == SCENT_GENERATOR)
						{
							map_human_scent[(*dev)->x][(*dev)->y] += ((*dev)->power_remaining*1200)/100;
							if(rand()%7 < 2) (*dev)->power_remaining -= 1;
							(*dev)->power_remaining = std::max(0,(*dev)->power_remaining);
						}
						else if((*dev)->device_type == HOLOGRAM_PROJECTOR)
						{
							actor* pr = (*dev)->projection;
							if(pr->x != -500)
							{
								map_occupants[pr->x][pr->y] = NULL;
							}
							int x = rand()%9;
							int y = rand()%9;
							while(terrain_immovable(pr,(*dev)->x+x-4,(*dev)->y+y-4))
							{
								x = rand()%9;
								y = rand()%9;
							}
							map_occupants[(*dev)->x+x-4][(*dev)->y+y-4] = pr;
							pr->x = (*dev)->x+x-4;
							pr->y = (*dev)->y+y-4;
							

						}
						else if((*dev)->device_type == NOISE_GENERATOR)
						{
							if((*dev)->current_action == HUMAN_STIMULUS)
							{
							make_noise((*dev)->x,(*dev)->y,&fake_human_noise[rand()%2]);
							}
							else if((*dev)->current_action == LOUD_STIMULUS)
							{
								make_noise((*dev)->x,(*dev)->y,&generator_loud[rand()%2]);
							}
						}
						else if((*dev)->device_type == BRAIN_SLICE)
						{
							(*dev)->current_action = HUMAN_STIMULUS;
						}
					}
					for(std::vector<item*>::iterator dev = explode_this_turn.begin(); dev != explode_this_turn.end(); ++dev)
					{
						set_unconfigured(*dev);
						//remove(timer_devices.begin(),timer_devices.end(),*dev);
						activate_device(*dev);
					}
					explode_this_turn.erase(explode_this_turn.begin(),explode_this_turn.end());
					for(std::vector<explosion>::iterator ex = explosions_this_turn.begin(); ex != explosions_this_turn.end(); ex++)
					{
						do_explosion(*ex);
					}
					explosions_this_turn.erase(explosions_this_turn.begin(),explosions_this_turn.end());
				}
				else if (act->is_hologram)
				{
					if(act->energy > 100)
					{
						act->energy -= 100;
						if(on_map(act->x,act->y))
							hologram_turn(act);
					}
				}
				else
				{
					if(act->energy > 100)
					{
						act->energy -= 100;
						if(!debug)
						{
						ai_turn(act);
						}
						act->most_noticeable_sound = 0.0;
						act->sound_threshold = std::max(act->sound_threshold-act->sspecies->hearing_adapt,act->sspecies->hearing_thres);
					}
				}
			}
		}
		if(rand()%4000+150+turn_count >4000 && rand()%(45-6*(difficulty-5)) == 0)
		{
			std::pair<int,int> loc = random_occupiable_square_away_from_player();

			add_actor(zoo[rand()%ZOO_SIZE+1],false,loc.first,loc.second);
		}
	return 0;
}

int main()
{
	srand((unsigned int)time(NULL));
	init();
#ifdef KILL_ESCAPE
	ESCDELAY=0;
#endif
	int end_game = 0;
	for(turn_count = 0;!end_game; turn_count++)
	{
		end_game = play_turn();
	}
	endwin();
}
