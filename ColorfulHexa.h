#pragma once

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "SDL2.lib")
#pragma comment(lib, "SDL2main.lib")
#pragma comment(lib,"SDL2_ttf.lib")
#pragma comment(lib,"SDL2_image.lib")
#pragma comment(lib,"SDL2_mixer.lib")
#pragma comment(lib,"libcurl.lib")

#include "resource.h"
#include <SDL.h>
#include "SDL_ttf.h"
#include "SDL_image.h"
#include "SDL_mixer.h"
#include "curl/curl.h"
#include <Windows.h>
#include <string>
#include <vector>
#include <array>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <algorithm>
#include <thread>
#include <regex>

using namespace std;

const int FPS = 60;
const float pi = 3.1416f;
extern int scr_w,scr_h;
int min_l(initializer_list<int> args);
int max_l(initializer_list<int> args);
float min_l(initializer_list<float> args);
float max_l(initializer_list<float> args);
int SDL_RenderCopyAlpha(SDL_Renderer *render, SDL_Texture *texture,const SDL_Rect *srcrect,const SDL_Rect *dstrect, unsigned int alpha);//alpha can be 0-255

class Timer {
private:
	double frametime = 1.0 / (double)FPS * 1000.0;
	bool high;// high performance timer available
	LARGE_INTEGER freq, start_time, current_time, play_time;// freq: System count frequency 
	unsigned long frames;
public:
	void start();
	void sleep();
	void close();
	LONGLONG get_time() { return play_time.QuadPart; }
	long get_frame() { return frames; }
	void set_frametime(double time) { frametime = time; }
};

class QR_Reader{
private:
	FILE *zbar;
	char buff[256];
	string /*userid="",*/tmp_str="";
	bool data;//dummy
	string post_rank[3] = {"easy" ,"normal","hard" };
	Timer timer;
	CURL *curl;
	CURLcode res;
	int err_count = 0;
public:
	string userid = "";
	bool inited,read,reading;
	int init(string curr_dir);
	void read_start();
	void reset();
	void check_hidden(bool is_hidden[2]);
	int send(int rank,int song,int score);
	static size_t callback_send(char* ptr, size_t size, size_t nmemb, bool* stream);
	static size_t callback(char* ptr, size_t size, size_t nmemb, bool* stream);
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
		string notesdesigner="Anonymous";
		vector<string> bpm;
		string midifile;
		vector<int> playlevel;
		vector<int> rank;
		int volwav;
		vector<long> total;
		vector<int> bar_num;
		SDL_Texture* stagefile=NULL;
		Mix_Chunk *bgm=NULL;
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
		const string cmd[12] = {
			"GENRE",
			"TITLE",
			"ARTIST",
			"BPM",
			"MIDIFILE",
			"PLAYLEVEL",
			"RANK",
			"VOLWAV",
			"TOTAL",
			"STAGEFILE",
			"NOTESDESIGNER",
			"WAV04"
		};
	};
	
	static const int NUM_CHANNEL = 21;
	const float MIN_SPEED = 0.5f;
	const float MAX_SPEED = 5.0f;
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
	bool hidden[2] = { false,false };//0:kouka,1:myth. hide when false
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
			if (index < 0) {
				index += header.size();
				continue;
			} else if (index >= header.size()) {
				index -= header.size();
				continue;
			}
			/*if (index == 21 && !hidden[0]) {
				index=(dis > 0) ? (hidden[1] ? 22 : 0) : 20;
			}
			else if (index == 22 && !hidden[1]) {
				index=(dis > 0) ? 0 : (hidden[0] ? 21 : 20);
			}*/
			break;
		}
		return header[index].stagefile;
	}
	int get_num_note() { return num_note; }
	void change_sel(int v) { 
		sel += v;
		if (sel >= (int)header.size())sel = 0;
		if (sel < 0)sel = header.size() - 1;
		if(sel==21&&!hidden[0]){
			sel=(v > 0) ? (hidden[1]?22:0) : 20;
		}
		if(sel==22&&!hidden[1]){
			sel=(v > 0) ? 0 : (hidden[0]?21:20);
		}
	}
	void change_lv(int v) {
		lv += v;
		if (lv >= header[sel].num)lv = 0;
		if (lv < 0)lv = header[sel].num - 1;
	}
	void reset_lv() { lv = 0; }
	void change_speed(float v) { 
		speed += v;
		if (speed < MIN_SPEED)speed = MIN_SPEED;
		if (speed > MAX_SPEED)speed = MAX_SPEED;
		time_draw_start = 1000l / speed;
	}
	void reset_speed() {
		speed = 1.0f;
		time_draw_start = 1000l / speed;
	}
	int get_sel() { return sel; }
	string get_title() {
		return header[sel].title;
	}
	string get_composer() { return header[sel].artist; }
	string get_notesdesigner() { return "Notes Designer: "+header[sel].notesdesigner; }
	void play_bgm() {
		Mix_PlayChannel(1,header[sel].bgm,-1);
	}
	int get_lv() { return lv; }
	int get_playlevel(int a_lv=-1) {
		return header[sel].playlevel[(a_lv==-1)?lv:a_lv];
	}
	float get_speed() { return speed; }
	void get_notes(vector<ENote> en[],long time);
	void set_pushed(int channel, int index) {
		if (music[channel].size() <=index)return;
		music[channel][index].pushed = true;
	}
	Mix_Chunk* get_wav(string k) { return wav[k]; }
	long get_total_count() { return bar_count.back(); }
	long time_to_count(long time);
	void set_next_note(unsigned int channel) {
		if (channel < NUM_CHANNEL)next_note[channel]++;
	}
};

class Letter_Scroll {
private:
	SDL_Rect src = {0,0,0,0}, dst,org_dst;
	SDL_Renderer *render;
	SDL_Texture *tex;
	TTF_Font *font;
	SDL_Color color;
	string str;
	int v,tex_w,tex_h;
	float rate_v,dst_w;
	bool scroll;
public:
	void init(SDL_Renderer *a_render, TTF_Font *a_font, SDL_Color a_color, const SDL_Rect *a_dst, string a_str, float f_vel);//f_vel:rate by height
	void set_text(string a_str);
	void draw();
	void close();
};

class Text_Draw {
private:
	string text;
	SDL_Rect rect;
	SDL_Texture *tex;
	SDL_Renderer *render;
	TTF_Font *font;
public:
	void init(SDL_Renderer *a_ren,TTF_Font *a_font) {
		render = a_ren;
		font = a_font;
	}
	void set_left(string a_text, int x,int y, unsigned int h,const SDL_Color col);
	void set_center(string a_text, int x_cen,int y_cen, unsigned int h, const SDL_Color col);
	void draw();
	void close();
};

class Gamemain {
private:
	Timer* timer;
	SDL_Renderer* render;
	SDL_Window* window;
	enum Mode{M_TITLE,M_CHOOSE,M_LOADING,M_PLAY,M_RESULT};
	Mode mode=M_TITLE;
	long latency1=0l, latency2=0l;//1:judgement from note, 2:bgm from note
	BMS_Manager bmsm;
	QR_Reader qr;
	int input[6] = {};//ujnbgy
	string curr_dir;
	SDL_Rect rect_center,rect_bottom,rect_title_logo;
	SDL_Point center;
	TTF_Font* font;
	SDL_Color f_color = { 210,210,210,SDL_ALPHA_OPAQUE };
	SDL_Texture *border,*title_logo;
	int tex_w, tex_h;
	string text_lv[3] = { " EASY ","NORMAL"," HARD " };
	Mix_Music *bgm_title, *bgm_result;

	bool any_pressed() {
		return input[0] == -1 || input[1] == -1 || input[2] == -1 || input[3] == -1 || input[4] == -1 || input[5] == -1;
	}

	void change_mode();

	int title();

	int choose();

	thread t_load;
	int step_choose = 0;
	SDL_Rect rect_title,rect_composer,rect_stagefile, rect_r, rect_l,rect_arr_r_0,rect_arr_l_0,rect_easy,rect_normal,rect_hard,rect_notesdesigner;
	SDL_Rect rect_arr_l[3],rect_arr_r[3];
	SDL_Texture *arr, *easy, *normal, *hard,*black_back;
	SDL_Point p_lv_easy, p_lv_normal, p_lv_hard,p_speed_conf;
	Text_Draw lv_easy, lv_normal, lv_hard,cap_speed,speed_conf;
	Letter_Scroll letter_title, composer, notesdesigner;

	int loading();

	Mix_Chunk* bgm_ready;

	int play();

	LONGLONG play_start;
	long play_time;
	vector<BMS_Manager::ENote> notes[8];//BGM,Lane1-6,bar line
	int ch;
	SDL_Texture *btn[6], *btn_p[6], *btn_col[6],*letter_judge[4],*letter_timing[3];
	SDL_Rect rect_btn[6];
	SDL_Rect rect_score = { int(scr_w*0.03f),int(scr_h*0.02f),int(scr_w*0.23f),int(scr_h*0.078f) };
	SDL_Rect rect_combo = { int(scr_w*0.8f),int(scr_h*0.02f),int(scr_w*0.15f),int(scr_h*0.078f) };
	SDL_Rect rect_col,rect_judge,rect_timing;
	const float NOTE_W = 0.015f;//per scr_h
	SDL_Texture *note[7], *ln[6][2];//note[6] : long note ends
	SDL_Texture *bar_line;
	float size_ln[3];
	SDL_Rect rect_note;
	SDL_Point lt = { 0,0 };
	SDL_Surface* s_letter;
	SDL_Texture* letter;

	array<bool,6> pushing_long;//long note pushed or not
	array<int, 6> ln_judge;

	unsigned long score = 0;
	unsigned int combo = 0;
	unsigned int max_combo = 0;
	int colors = 0;
	float colorful_gauge = 0.0f;

	const int R_PERFECT = 5;
	const int R_GREAT = 10;
	const int R_GOOD = 15;
	const int R_MISS = 20;
	//border line in frames

	const float S_PERFECT = 1.0f;
	const float S_GREAT = 0.8f;
	const float S_GOOD = 0.5f;
	//score rate

	const float COL_GAUGE[4] = { 1.0f,0.4f,0.0f,-2.0f };//colorful gauge rate
	float unit_col_gauge=0.0f;

	const int COLOR_HEX[6][3] = { {0xff,0xaa,0xaa},{0xff,0xcc,0xaa},{0xff,0xff,0xaa},{0xaa,0xff,0xaa},{0xaa,0xaa,0xff},{0xff,0xaa,0xff} };

	int curr_judge=-1;
	array<unsigned int,6> judge = {};//perfect,great,good,miss
	enum Timing{ T_NONE=-1,T_JUST=0,T_FAST=1,T_LATE=2 };
	Timing curr_timing = T_NONE;
	bool check_judge(long count, bool add_judge = true,int *judge_container=NULL);
	void calc_score();
	void draw_note(long count,int lane,int color,long end_count,bool is_bar);
	string digit(int data, int digit);
	string digit(float data, int digit);
	class BG {
	private:
		SDL_Rect src;
		const float MOVE_SPEED = 0.2f;//per frame
		float angle = 0.0f;
		float x, y;
		int w,h;
		SDL_Renderer *render;
		SDL_Texture *image;
	public:
		void init(SDL_Renderer *render, string path) {
			SDL_Surface *tmp;
			this->render = render;
			tmp = IMG_Load(path.c_str());
			image = SDL_CreateTextureFromSurface(render, tmp);
			SDL_FreeSurface(tmp);
			SDL_QueryTexture(image, NULL, NULL, &w, &h);
			src.x = (int)((float)w*0.125f);
			src.y = (int)((float)h*0.125f);
			src.w = (int)((float)w*0.75f);
			src.h = (int)((float)h*0.75f);
			x = src.x; y = src.y;
			angle = float(rand() % 200) / 100.0f*pi;
		}
		void draw() {
			x += MOVE_SPEED * cos(angle);
			y -= MOVE_SPEED * sin(angle);
			if (x < 0) {
				x = 0.0f;
				angle = (float(rand() % 100) / 100.0f-0.5f)*pi;
			} else if (x> w-src.w) {
				x = w - src.w;
				angle = (float(rand() % 100) / 100.0f+0.5f)*pi;
			}
			if (y < 0) {
				y = 0.0f;
				if(x<=0)angle = (float(rand() % 50) / 100.0f-0.5f)*pi;
				else if(x>=w-src.w)angle = (float(rand() % 50) / 100.0f + 1.0f)*pi;
				else angle = (float(rand() % 100) / 100.0f +1.0f)*pi;
			} else if (y > h-src.h) {
				y = h - src.h;
				if(x<=0)angle = (float(rand() % 50) / 100.0f)*pi;
				else if(x>=w-src.w)angle = (float(rand() % 50) / 100.0f + 0.5f)*pi;
				angle = (float(rand() % 100) / 100.0f)*pi;
			}
			src.x = (int)x;
			src.y = (int)y;
			SDL_RenderCopy(render, image, &src, NULL);
		}
	};
	BG bg_1, bg_2;
	//for play

	int result();
	int range_rank[9] = { 115000,110000,105000,100000,95000,90000,85000,80000,65000 };
	int rank;
	SDL_Texture *letter_rank[10];
	SDL_Rect rect_res_score = { 0,int(scr_h*0.2f),scr_w,int(scr_h*0.075f) }, rect_rank = { 0,int(scr_h*0.325f),scr_w,int(scr_h*0.125f) }, rect_res_combo = { 0,int(scr_h*0.485f),scr_w,int(scr_h*0.05f) }, rect_perfect = { 0,int(scr_h*0.6f),scr_w,int(scr_h*0.05f) }, rect_great = { 0,int(scr_h*0.66),scr_w,int(scr_h*0.05f) }, rect_good = { 0,int(scr_h*0.72f),scr_w,int(scr_h*0.05f) }, rect_miss = { 0,int(scr_h*0.78f),scr_w,int(scr_h*0.05f) }, rect_lv_res= { int(scr_w*0.74f),int(scr_h*0.02f),int(scr_w*0.23f),int(scr_h*0.078f) };

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
