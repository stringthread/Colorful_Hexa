#pragma once

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "SDL2main.lib")
#pragma comment(lib,"SDL2_ttf.lib")
#pragma comment(lib,"SDL2_image.lib")
#pragma comment(lib,"SDL2_mixer.lib")

#include "resource.h"
#include <SDL.h>
#include "SDL_ttf.h"
#include "SDL_image.h"
#include "SDL_mixer.h"
#include <Windows.h>
#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <algorithm>
#include <thread>
#include <regex>

using namespace std;

const int FPS = 60;
extern int scr_w,scr_h;
int min_l(initializer_list<int> args);
int max_l(initializer_list<int> args);
float min_l(initializer_list<float> args);
float max_l(initializer_list<float> args);
int SDL_RenderCopyAlpha(SDL_Renderer *render, SDL_Texture *texture,const SDL_Rect *srcrect,const SDL_Rect *dstrect, unsigned int alpha);//alpha can be 0-255

class Timer {
private:
	const double frametime = 1.0 / (double)FPS * 1000.0;
	bool high;// high performance timer available
	LARGE_INTEGER freq, start_time, current_time, play_time;// freq: System count frequency 
	unsigned long frames;
public:
	void start();
	void sleep();
	void close();
	LONGLONG get_time() { return play_time.QuadPart; }
};

class QR_Reader{
private:

public:
	int init();
	bool read();
	void reset();
	void close();
};

class BMS_Manager {
private:
	class Note {
	protected:
		string data;
		int bar;
		long count;
		long end_count;//-1 means 'not a long note'
	public:
		bool pushed=false;
		Note(string data, int bar, long count, long end_count=-1l) {
			this->count = count;
			this->bar = bar;
			this->data = data;
			this->end_count = end_count;
		}
		Note(){}
		Note(const Note& n) {
			count = n.count;
			bar = n.bar;
			data = n.data;
			end_count = n.end_count;
			pushed = n.pushed;
		}
		~Note(){}
		long get_count() { return count; }
		long get_end_count() { return end_count; }
		int get_bar() { return bar; }
		string get_data() { return data; }
		bool is_ln() { return end_count != -1; }
		bool operator<(BMS_Manager::Note& n) {
			return count < n.get_count();
		}
		bool operator>(BMS_Manager::Note& n) {
			return count > n.get_count();
		}//for sorting
	};

	class Header {
	public:
		int num = 0;//number of levels
		vector<string> bmsfile;
		string dir;//end with '\'
		string title;
		string genre;
		string artist;
		vector<string> bpm;
		string midifile;
		vector<int> playlevel;
		vector<int> rank;
		int volwav;
		vector<long> total;
		vector<int> bar_num;
		SDL_Texture* stagefile=NULL;
		Header() {}
		Header(const Header& h){
			num = h.num;
			bmsfile = h.bmsfile;
			dir = h.dir;
			title = h.title;
			genre = h.genre;
			artist = h.artist;
			bpm = h.bpm;
			midifile = h.midifile;
			playlevel = h.playlevel;
			rank = h.rank;
			volwav = h.volwav;
			total = h.total;
			bar_num = h.bar_num;
			stagefile = h.stagefile;
		}
		~Header(){}
		int load(string dir,string file,bool is_first,SDL_Renderer* render);
		void sort();
	private:
		const string cmd[10] = {
			"GENRE",
			"TITLE",
			"ARTIST",
			"BPM",
			"MIDIFILE",
			"PLAYLEVEL",
			"RANK",
			"VOLWAV",
			"TOTAL",
			"STAGEFILE"
		};
	};

	static const int NUM_CHANNEL = 20;
	const float MIN_SPEED = 0.5f;
	const float MAX_SPEED = 2.0f;
	vector<Header> header;
	vector<Note> music[NUM_CHANNEL];
	unordered_map<string,Mix_Chunk*> wav;
	unordered_map<string,SDL_Texture*> bmp;
	unordered_map<string,int> bpm;//index(in BMS) -> data
	vector<long> bar_count;
	int bar_last = 1;
	int num_note=0;
	int sel = 0;
	int lv = 0;
	float speed = 1.0f;
	int error=0;
	int next_note[NUM_CHANNEL] = {0};
	vector<long> time_bpm_change;//for time_to_count function
	void set_time_bpm_change();
public:
	static const long BAR_STANDARD = 960l;
	long time_draw_start = 1000l;
	SDL_Rect rect_stagefile;
	class ENote : public Note {
	private:
		int index;
	public:
		ENote(Note n, int index) {
			data = n.get_data();
			count = n.get_count();
			end_count = n.get_end_count();
			bar = n.get_bar();
			pushed = n.pushed;
			this->index = index;
		}
		ENote(const ENote& n){
			data = n.data;
			count = n.count;
			end_count = n.end_count;
			bar = n.bar;
			pushed = n.pushed;
			index = n.index;
		}
		~ENote(){}
		int get_index() { return index; }
	};
	bool header_loaded,music_loaded;//check thread ending
	BMS_Manager(SDL_Renderer* render);
	int load_music(SDL_Renderer *render);
	void reset();
	SDL_Texture* get_stagefile(int dis = 0) {
		int index = sel + dis;
		while(true){
			if (index < 0)index = header.size() + index;
			else if (index >= header.size())index -= header.size();
			else break;
		}
		return header[index].stagefile;
	}
	int get_num_note() { return num_note; }
	void change_sel(int v) { 
		sel += v;
		if (sel >= (int)header.size())sel = 0;
		if (sel < 0)sel = header.size() - 1;
	}
	void change_lv(int v) {
		lv += v;
		if (lv >= header[sel].num)lv = 0;
		if (lv < 0)lv = header[sel].num - 1;
	}
	void change_speed(float v) { 
		speed += v;
		if (speed < MIN_SPEED)speed = MIN_SPEED;
		if (speed > MAX_SPEED)speed = MAX_SPEED;
		time_draw_start = 1000l / speed;
	}
	int get_sel() { return sel; }
	int get_lv() { return lv; }
	float get_speed() { return speed; }
	void get_notes(vector<ENote> en[],long time);
	void set_pushed(int channel, int index) {
		if (music[channel].size() <=index)return;
		music[channel][index].pushed = true;
	}
	Mix_Chunk* get_wav(string k) { return wav[k]; }
	long get_total_count() { return bar_count.back()+BAR_STANDARD; }
	long time_to_count(long time);
	void set_next_note(unsigned int channel) {
		if (channel < NUM_CHANNEL)next_note[channel]++;
	}
};

class Gamemain {
private:
	const float pi = 3.1416f;

	Timer* timer;
	SDL_Renderer* render;
	SDL_Window* window;
	enum Mode{M_TITLE,M_CHOOSE,M_LOADING,M_PLAY,M_RESULT};
	Mode mode=M_TITLE;
	BMS_Manager bmsm;
	QR_Reader qr;
	int input[6] = {};//x~m key in clockwise from top right
	SDL_Rect rect_center;
	SDL_Point center;
	TTF_Font* font;
	SDL_Color f_color = { 255,160,0,255 };
	SDL_Texture *border;

	bool any_pressed() {
		return input[0] == -1 || input[1] == -1 || input[2] == -1 || input[3] == -1 || input[4] == -1 || input[5] == -1;
	}

	void change_mode();

	int title();

	int choose();

	thread t_load;
	int step_choose = 0;
	SDL_Rect rect_stagefile, rect_r, rect_l;

	int loading();

	int play();

	LONGLONG play_start;
	long play_time;
	vector<BMS_Manager::ENote> notes[7];//BGM,Lane1-6
	int ch;
	SDL_Texture *btn[6], *btn_p[6];
	SDL_Rect rect_btn[6];
	SDL_Rect rect_score = { int(scr_w*0.05f),int(scr_h*0.05f),int(scr_w*0.3f),int(scr_h*0.1f) };
	SDL_Rect rect_combo = { int(scr_w*0.75f),int(scr_h*0.05f),int(scr_w*0.2f),int(scr_h*0.1f) };
	SDL_Rect rect_back = {0,int(scr_h*0.8f),scr_w,int(scr_h*0.2f)};
	SDL_Rect rect_col;
	const float NOTE_W = 0.005f;//per scr_h
	SDL_Texture *note[7], *ln[6][3];//note[6] : long note ends
	float size_ln[3];
	SDL_Rect rect_note;
	SDL_Surface* s_letter;
	SDL_Texture* letter;

	array<bool,6> pushing_long;//long note pushed or not

	unsigned long score = 0;
	unsigned int combo = 0;
	unsigned int max_combo = 0;
	int colors = 0;
	float colorful_gauge = 0.0f;

	const int R_PERFECT = 5;
	const int R_GREAT = 10;
	const int R_GOOD = 15;
	const int R_MISS = 30;
	//border line in frames

	const float S_PERFECT = 1.0f;
	const float S_GREAT = 0.8f;
	const float S_GOOD = 0.5f;
	//score rate

	const float COL_GAUGE[4] = { 0.005f,0.002f,0.0f,-0.03f };//colorful gauge rate

	const int COLOR_HEX[6][3] = { {0xff,0xaa,0xaa},{0xff,0xcc,0xaa},{0xaa,0xff,0xaa},{0xaa,0xcc,0xff},{0xaa,0xaa,0xff},{0xff,0xaa,0xff} };

	int curr_judge=-1;
	array<unsigned int,6> judge = {};//perfect,great,good,miss
	enum Timing{ T_NONE,T_FAST,T_LATE };
	Timing curr_timing = T_NONE;
	bool check_judge(long count);
	void calc_score();
	void draw_note(long count,int lane,int color,long end_count=-1l);
	string digit(int data, int digit);
	string digit(float data, int digit);
	//variables for play

	int result();

public:
	Gamemain(Timer* timer,SDL_Renderer* render,SDL_Window* window):bmsm(render) {
		this->timer = timer;
		this->render = render;
		this->window = window;
	}
	void init();
	int loop();
	void close();
};
