#include "stdafx.h"
#include "ColorfulHexa.h"
#include <cmath>
#include <sstream>
#include <ios>
#include <iomanip>

void Gamemain::change_mode() {
	switch (mode) {
		case M_TITLE:
			mode = M_CHOOSE;
			break;
		case M_CHOOSE:
			t_load = thread([this]() {bmsm.load_music(render);});
			t_load.detach();
			step_choose = 0;
			mode = M_LOADING;
			break;
		case M_LOADING:
			play_start = timer->get_time();
			play_time = 0l;
			mode = M_PLAY;
			break;
		case M_PLAY:mode = M_RESULT; break;
		case M_RESULT:
			Mix_HaltChannel(-1);
			bmsm.reset();
			qr.reset();
			pushing_long.fill(0);
			judge.fill(0);
			score = 0;
			combo = 0;
			max_combo = 0;
			colors = 0;
			colorful_gauge = 0.0f;
			curr_judge = -1;
			mode = M_TITLE;
			break;
	}
	return;
}

void Gamemain::calc_score() {
	score = (long)(100000 / bmsm.get_num_note()*(judge[0] * S_PERFECT + judge[1] * S_GREAT + judge[2] * S_GOOD));
	score += (long)(10000 / bmsm.get_num_note() *max_combo);
	score += (long)(10000.0f * colorful_gauge/100);
	/*
	if (colorful_gauge >= 1.0f) {
		if (colors < 5) {
			colors++;
			colorful_gauge = 0.0f;
		} else colorful_gauge = 1.0f;
	}
	*///no color back
	///*
	colors = (int)colorful_gauge;
	if(colorful_gauge>=6.0f){
	colorful_gauge=6.0f;
	colors = 5;
	}
	//*///color back
}

void Gamemain::draw_note(long count,int lane,int color,long end_count){
	if (lane < 0 || lane >= 6)return;

	float dis_note_in,dis_note_out;
	if (count < bmsm.time_to_count(play_time) && end_count < bmsm.time_to_count(play_time))return;

	if (end_count > 0) {
		dis_note_in = (0.3f + 0.7f*max(count - bmsm.time_to_count(play_time), 0) / (bmsm.time_to_count(play_time + bmsm.time_draw_start) - bmsm.time_to_count(play_time)))*scr_h/2;
		dis_note_out = (min(0.3f + 0.7f*(end_count - bmsm.time_to_count(play_time)) / (bmsm.time_to_count(play_time + bmsm.time_draw_start) - bmsm.time_to_count(play_time)), 1)/2+NOTE_W)*scr_h;
		rect_note.h = (dis_note_out - dis_note_in)*sin(pi/3)+1;
		rect_note.w = rect_note.h*size_ln[0];
		rect_note.x = scr_w / 2 + sin(pi*lane / 3)*dis_note_in-rect_note.w;
		rect_note.y = scr_h / 2 - cos(pi*lane / 3)*dis_note_in-rect_note.h;
		center.x = rect_note.w;
		center.y = rect_note.h;
		SDL_RenderCopyEx(render, ln[color][0], NULL, &rect_note, 60 * (lane+0.5f), &center, SDL_FLIP_NONE);
		//first triangle

		rect_note.x = scr_w / 2 + sin(pi*(lane+1) / 3)*dis_note_in;
		rect_note.y = scr_h / 2 - cos(pi*(lane+1) / 3)*dis_note_in - rect_note.h;
		center.x = 0;
		center.y = rect_note.h;
		SDL_RenderCopyEx(render, ln[color][0], NULL, &rect_note, 60 * (lane+0.5f), &center, SDL_FLIP_HORIZONTAL);
		//second triangle

		rect_note.w = dis_note_in+1;
		rect_note.h = (dis_note_out - dis_note_in)*sin(pi / 3);
		rect_note.x = scr_w / 2 + sin(pi*lane / 3)*dis_note_in;
		rect_note.y = scr_h / 2 - cos(pi*lane / 3)*dis_note_in - rect_note.h;
		center.y = rect_note.h;
		SDL_RenderCopyEx(render, ln[color][1], NULL, &rect_note, 60 * (lane+0.5f), &center, SDL_FLIP_NONE);
		//center rect
		//SDL_RenderDrawRect(render, &rect_note);

		if(count>bmsm.time_to_count(play_time))draw_note(count, lane, 6);
		if (end_count < bmsm.time_to_count(play_time + bmsm.time_draw_start))draw_note(end_count, lane, 6);
		return;
	}
	
	dis_note_in = (0.3f + 0.7f*(count - bmsm.time_to_count(play_time)) / (bmsm.time_to_count(play_time + bmsm.time_draw_start) - bmsm.time_to_count(play_time))) / 2 * scr_h;
	dis_note_out = ((0.3f + 0.7f*(count - bmsm.time_to_count(play_time)) / (bmsm.time_to_count(play_time + bmsm.time_draw_start) - bmsm.time_to_count(play_time))) / 2 + NOTE_W)*scr_h;

	rect_note.w = int(sin(pi / 3)*dis_note_out);
	rect_note.h = abs(dis_note_out - int(cos(pi/3)*dis_note_in));
	rect_note.x = scr_w/2+int(sin(lane*pi / 3)*dis_note_out);
	rect_note.y = scr_h/2-int(cos(lane*pi / 3)*dis_note_out);

	SDL_RenderCopyEx(render, note[color], NULL, &rect_note, 60 * lane, &lt, SDL_FLIP_NONE);
}

bool Gamemain::check_judge(long count,bool add_judge) {
	long now_count = bmsm.time_to_count(play_time);
	long judge_count[4] = {
		bmsm.time_to_count(play_time + R_PERFECT * 1000 / FPS) - now_count,
		bmsm.time_to_count(play_time + R_GREAT * 1000 / FPS) - now_count,
		bmsm.time_to_count(play_time + R_GOOD * 1000 / FPS) - now_count,
		bmsm.time_to_count(play_time + R_MISS * 1000 / FPS) - now_count,
	};
	long dis_count = abs(count - now_count);
	curr_timing=T_NONE;
	for (int i= 0; i < 4; i++) {
		if (dis_count > judge_count[i])continue;
		if (!add_judge)return i!=3;
		judge[i]++;
		curr_judge = i;
		//judgement

		if (i == 0)curr_timing = T_JUST;
		else if (count > now_count)curr_timing = T_FAST;
		else curr_timing = T_LATE;
		//timing

		if (i == 3) {
			combo = 0;
		}
		else {
			combo++;
			if (combo > max_combo)max_combo = combo;
		}
		//combo

		colorful_gauge += COL_GAUGE[i];
		if (colorful_gauge < 0.0f)colorful_gauge = 0.0f;
		//colorful gauge
		return true;
	}
	return false;
}

string Gamemain::digit(int data, int n) {
	ostringstream oss;
	oss << setw(n) << setfill('0') << data;
	return oss.str();
}

string Gamemain::digit(float data, int n) {
	ostringstream oss;
	oss << fixed << setprecision(n) << data;
	return oss.str();
}

int Gamemain::title(){
	if (bmsm.header_loaded) {
		if (any_pressed() || qr.read()) {
			change_mode();
		}
		SDL_SetRenderDrawColor(render, 0xaa, 0xaa, 0xff, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(render);
		//draw title screen
	} else {
		SDL_SetRenderDrawColor(render, 0xff, 0xaa, 0xff, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(render);
		//draw opening screen
	}
	return 0;
}

int Gamemain::choose(){
	SDL_RenderCopy(render,bmsm.get_stagefile(),NULL,&rect_stagefile);
	switch (step_choose) {
		case 0:
			if (input[0] == -1 || input[5] == -1)step_choose++;
			if (input[1]==1 || (input[1] >= 60 && (input[1] - 60) % 20 == 0)) {
				bmsm.change_sel(1);
			}
			if (input[4]==1 || (input[4] >= 60 && (input[4] - 60) % 20 == 0)) bmsm.change_sel(-1);
			
			SDL_RenderCopyAlpha(render, bmsm.get_stagefile(-1), NULL, &rect_l,180);
			SDL_RenderCopyAlpha(render, bmsm.get_stagefile(1), NULL, &rect_r, 180);
			//draw choose song screen
			break;
			//choose song
		case 1:
			if (input[2] == -1 || input[3] == -1)step_choose--;
			if (input[0] == -1 || input[5] == -1)step_choose++;
			if (input[1]==1 || (input[1] >= 60 && (input[1] - 60) % 20 == 0)) bmsm.change_lv(1);
			if (input[4]==1 || (input[4] >= 60 && (input[4] - 60) % 20 == 0)) bmsm.change_lv(-1);

			s_letter = TTF_RenderText_Blended(font, (" "+to_string(bmsm.get_lv())+" ").c_str(), f_color);
			letter = SDL_CreateTextureFromSurface(render, s_letter);
			SDL_FreeSurface(s_letter);
			SDL_RenderCopy(render, letter, NULL, &rect_center);
			SDL_DestroyTexture(letter);
			//draw choose lv screen
			break;
			//choose lv
		case 2:
			if (input[2] == -1 || input[3] == -1)step_choose--;
			if (input[0] == -1 || input[5] == -1)step_choose++;
			if (input[1]==1 || (input[1] >= 60 && (input[1] - 60) % 20 == 0)) bmsm.change_speed(0.25f);
			if (input[4]==1 || (input[4] >= 60 && (input[4] - 60) % 20 == 0)) bmsm.change_speed(-0.25f);

			s_letter = TTF_RenderText_Blended(font, digit(bmsm.get_speed(),2).c_str(), f_color);
			letter = SDL_CreateTextureFromSurface(render, s_letter);
			SDL_FreeSurface(s_letter);
			SDL_RenderCopy(render, letter, NULL, &rect_center);
			SDL_DestroyTexture(letter);
			//draw choose speed screen
			break;
			//choose speed
		case 3:
			if (input[2] == -1 || input[3] == -1) {
				step_choose--;
				break;
			}
			if (any_pressed()) {
				step_choose = 0;
				change_mode();//start loading data
				break;
			}

			//draw confirm screen
			break;
			//confirm
	}
	return 0;
}

int Gamemain::loading() {
	if (bmsm.music_loaded) {
		if (any_pressed()) {
			bmsm.music_loaded = false;
			change_mode();
		}
		SDL_SetRenderDrawColor(render, 0xff, 0xaa, 0xaa,SDL_ALPHA_OPAQUE);
		SDL_RenderClear(render);
		//draw play start screen
	}else{

		//draw loading screen
	}
	return 0;
}

int Gamemain::play(){
	play_time = (long)(timer->get_time() - play_start);
	if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_ESCAPE] == 1)change_mode();
	bmsm.get_notes(&notes[0],play_time);
	for (int i = 0; i < 7; i++) {
		if (notes[i].size() == 0)continue;
		for (int j = 0; j < notes[i].size(); j++) {
			if (i == 0 && !notes[i][j].pushed) {
				if(bmsm.time_to_count(play_time/*-120l*/)<2*bmsm.BAR_STANDARD){
					ch = Mix_GroupAvailable(7);
					if (ch != -1)goto play;
				}
				ch = Mix_GroupAvailable(6);
				if (ch == -1) {
					ch = Mix_GroupOldest(6);
					if (Mix_Playing(ch)==1)Mix_HaltChannel(ch);
				}
				play:
				Mix_PlayChannel(ch, bmsm.get_wav(notes[i][j].get_data()), 0);
				bmsm.set_pushed(0, notes[i][j].get_index());
			}
			//bgm
			if (notes[i][j].get_count() < bmsm.time_to_count(play_time - R_GOOD * 1000 / FPS) && notes[i][j].get_end_count() < bmsm.time_to_count(play_time - R_GOOD * 1000 / FPS)) {
				if (i != 0) {
					if (!notes[i][j].pushed) {
						combo = 0;
						colorful_gauge += COL_GAUGE[3];
						if (colorful_gauge < 0.0f)colorful_gauge = 0.0f;
						curr_judge = 3;
						curr_timing = T_LATE;
					}
					if (notes[i][j].is_ln() && pushing_long[i - 1])pushing_long[i-1] = false;
					bmsm.set_next_note(notes[i][j].is_ln() ? i + 13 : i + 7);
				}
				continue;
			}
			if(notes[i][j].pushed&&notes[i][j].get_count() < bmsm.time_to_count(play_time)&& notes[i][j].get_end_count() < bmsm.time_to_count(play_time)) {
				bmsm.set_next_note(notes[i][j].is_ln() ? i + 13 : i + 7);
				continue;
			}
			//check if note in range

			if (i == 0)continue;
			if (input[i-1] == 1) {
				if (notes[i][j].pushed)continue;
				if (notes[i][j].get_end_count() != -1) {
					if(check_judge(notes[i][j].get_count()))pushing_long[i - 1] = true;
				}
				else if (check_judge(notes[i][j].get_count()))bmsm.set_pushed(i + 7, notes[i][j].get_index());
			} else if (input[i - 1] == -1 && pushing_long[i-1]) {
				pushing_long[i - 1] = false;
				bmsm.set_pushed(i + 13, notes[i][j].get_index());
				if (notes[i][j].get_end_count() != -1) {
					if (!check_judge(notes[i][j].get_end_count(),false)) {
						combo = 0;
						judge[3]++;
						colorful_gauge += COL_GAUGE[3];
						if (colorful_gauge < 0.0f)colorful_gauge = 0.0f;
					}
				}
			}
			break;
		}
	}
	calc_score();
	if (bmsm.get_total_count() < bmsm.time_to_count(play_time))change_mode();
	//calculation

	SDL_SetRenderDrawColor(render, COLOR_HEX[colors][0], COLOR_HEX[colors][1], COLOR_HEX[colors][2], SDL_ALPHA_OPAQUE);
	bg_1.draw();
	bg_2.draw();
	//rect_col.h = (int)(1.732f/3*2*rect_col.w*colorful_gauge);//no color back
	rect_col.h = (int)(1.732f / 3 * 2*rect_col.w*(colorful_gauge-colors));//color back
	rect_col.y = scr_h / 2 + (int)(1.732f/3*rect_col.w) - rect_col.h;
	SDL_RenderFillRect(render,&rect_col);

	for (int i= 0; i < 6; i++) {
		for (int j = 0; j < notes[i+1].size(); j++) {
			draw_note(notes[i+1][j].get_count(),i,colors,notes[i+1][j].get_end_count());
		}
		SDL_RenderCopy(render,(input[i] > 0) ? btn_p[i] : btn[i], NULL,  &rect_btn[i]);
		if(colorful_gauge>=i+1)SDL_RenderCopy(render, btn_col[i], NULL, &rect_btn[i]);
	}

	if (curr_judge >= 0)SDL_RenderCopy(render, letter_judge[curr_judge], NULL, &rect_judge);
	if (curr_timing >= 0)SDL_RenderCopy(render, letter_timing[curr_timing], NULL, &rect_timing);

	s_letter = TTF_RenderUTF8_Blended(font, digit((int)score, 6).c_str(), f_color);
	letter = SDL_CreateTextureFromSurface(render, s_letter);
	SDL_FreeSurface(s_letter);
	SDL_RenderCopy(render,letter, NULL, &rect_score);
	SDL_DestroyTexture(letter);
	s_letter = TTF_RenderUTF8_Blended(font, digit((int)combo,3).c_str(), f_color);
	letter = SDL_CreateTextureFromSurface(render, s_letter);
	SDL_FreeSurface(s_letter);
	SDL_RenderCopy(render,letter, NULL, &rect_combo);
	SDL_DestroyTexture(letter);
	//draw play screen
	return 0;
}

int Gamemain::result(){
	if (any_pressed() && !qr.read())change_mode();
	SDL_SetRenderDrawColor(render, 0xaa, 0xff, 0xaa, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(render);
	//draw result screen
	return 0;
}

void Gamemain::init(){
	TTF_Init();
	char c_curr_dir[256] = {};
	string curr_dir;
	GetModuleFileName(NULL, &c_curr_dir[0], 256);
	curr_dir=regex_replace(c_curr_dir, regex("\\\\[^\\\\]+$"), "");
	font = TTF_OpenFont((curr_dir + "\\font\\font.ttf").c_str(), 200);
	//font

	rect_btn[0].x = scr_w / 2; rect_btn[0].y = int(0.35f*scr_h); rect_btn[0].w = int(0.15f*scr_w); rect_btn[0].h = int(0.1125f*scr_h);
	rect_btn[1].x = int(0.575f*scr_w); rect_btn[1].y = int(0.425f*scr_h); rect_btn[1].w = int(0.075f*scr_w); rect_btn[1].h = int(0.15f*scr_h);
	rect_btn[2].x = rect_btn[0].x; rect_btn[2].y = int(0.5375f*scr_h); rect_btn[2].w = rect_btn[0].w; rect_btn[2].h = rect_btn[0].h;
	rect_btn[3].x = int(0.35f*scr_w); rect_btn[3].y = rect_btn[2].y; rect_btn[3].w = rect_btn[0].w; rect_btn[3].h = rect_btn[0].h;
	rect_btn[4].x = rect_btn[3].x; rect_btn[4].y = rect_btn[1].y; rect_btn[4].w = rect_btn[1].w; rect_btn[4].h = rect_btn[1].h;
	rect_btn[5].x = rect_btn[3].x; rect_btn[5].y = rect_btn[0].y; rect_btn[5].w = rect_btn[0].w; rect_btn[5].h = rect_btn[0].h;
	rect_center = { int(scr_w*0.3f),int(scr_h*0.4f),int(scr_w*0.4f),int(scr_h*0.2f) };
	rect_stagefile.w = rect_stagefile.h = min(scr_w, scr_h) / 2;
	rect_stagefile.x = (scr_w - rect_stagefile.w) / 2;
	rect_stagefile.y = (scr_h - rect_stagefile.h) / 2;
	rect_r.w = rect_l.w = rect_stagefile.x*0.75f;
	rect_r.x = scr_w - rect_r.w; rect_l.x = 0;
	rect_r.h = rect_l.h = rect_stagefile.w*0.6f;
	rect_r.y = rect_l.y = (scr_h - rect_r.h) / 2;
	rect_col.x = rect_btn[4].x + rect_btn[4].w; rect_col.w = rect_btn[1].x - rect_col.x;
	SDL_Surface* tmp=NULL;
	tmp = IMG_Load((string(curr_dir) + "\\image\\border.png").c_str());
	border = SDL_CreateTextureFromSurface(render, tmp);
	SDL_FreeSurface(tmp);
	for (int i= 0; i < 6; i++) {
		tmp = IMG_Load((string(curr_dir) + "\\image\\btn_" + to_string(i)+".png").c_str());
		btn[i] = SDL_CreateTextureFromSurface(render,tmp);
		SDL_FreeSurface(tmp);
		tmp = IMG_Load((string(curr_dir) + "\\image\\btn_p_" + to_string(i)+".png").c_str());
		btn_p[i] = SDL_CreateTextureFromSurface(render, tmp);
		SDL_FreeSurface(tmp);
		tmp = IMG_Load((string(curr_dir) + "\\image\\btn_col_" + to_string(i) + ".png").c_str());
		btn_col[i] = SDL_CreateTextureFromSurface(render, tmp);
		SDL_FreeSurface(tmp);
		tmp = IMG_Load((string(curr_dir) + "\\image\\n_" + to_string(i) + ".png").c_str());
		note[i] = SDL_CreateTextureFromSurface(render, tmp);
		SDL_FreeSurface(tmp);
		for (int j = 0; j < 2; j++) {
			tmp = IMG_Load((string(curr_dir) + "\\image\\ln_" + to_string(i) + "_" + to_string(j) + ".png").c_str());
			ln[i][j] = SDL_CreateTextureFromSurface(render, tmp);
			SDL_FreeSurface(tmp);
		}
	}
	int tex_w, tex_h;
	for (int i = 0; i < 3; i++) {
		SDL_QueryTexture(ln[0][i], NULL, NULL, &tex_w, &tex_h);
		size_ln[i] = float(tex_w) / float(tex_h);
	}
	tmp = IMG_Load((string(curr_dir) + "\\image\\ln_end.png").c_str());
	note[6] = SDL_CreateTextureFromSurface(render, tmp);
	SDL_FreeSurface(tmp);

	tmp = IMG_Load((string(curr_dir) + "\\image\\Perfect.png").c_str());
	letter_judge[0] = SDL_CreateTextureFromSurface(render, tmp);
	SDL_FreeSurface(tmp);
	tmp = IMG_Load((string(curr_dir) + "\\image\\Great.png").c_str());
	letter_judge[1] = SDL_CreateTextureFromSurface(render, tmp);
	SDL_FreeSurface(tmp);
	tmp = IMG_Load((string(curr_dir) + "\\image\\Good.png").c_str());
	letter_judge[2] = SDL_CreateTextureFromSurface(render, tmp);
	SDL_FreeSurface(tmp);
	tmp = IMG_Load((string(curr_dir) + "\\image\\Miss.png").c_str());
	letter_judge[3] = SDL_CreateTextureFromSurface(render, tmp);
	SDL_FreeSurface(tmp);
	rect_judge.x = int(scr_w*0.4f);
	rect_judge.w = int(scr_w*0.2f);
	SDL_QueryTexture(letter_judge[0], NULL, NULL, &tex_w, &tex_h);
	rect_judge.h = rect_judge.w*tex_h / tex_w;
	rect_judge.y = scr_h / 2 - rect_judge.h;
	bg_1.init(render, (string(curr_dir) + "\\image\\bg_1.png").c_str());
	bg_2.init(render, (string(curr_dir) + "\\image\\bg_2.png").c_str());

	tmp = IMG_Load((string(curr_dir) + "\\image\\Just.png").c_str());
	letter_timing[0] = SDL_CreateTextureFromSurface(render, tmp);
	SDL_FreeSurface(tmp);
	tmp = IMG_Load((string(curr_dir) + "\\image\\Fast.png").c_str());
	letter_timing[1] = SDL_CreateTextureFromSurface(render, tmp);
	SDL_FreeSurface(tmp);
	tmp = IMG_Load((string(curr_dir) + "\\image\\Late.png").c_str());
	letter_timing[2] = SDL_CreateTextureFromSurface(render, tmp);
	SDL_FreeSurface(tmp);
	rect_timing.x = int(scr_w*0.4f);
	rect_timing.w = int(scr_w*0.2f);
	SDL_QueryTexture(letter_timing[0], NULL, NULL, &tex_w, &tex_h);
	rect_timing.h = rect_timing.w*tex_h / tex_w;
	rect_timing.y = scr_h / 2;
	//image


	Mix_AllocateChannels(72);
	Mix_GroupChannels(0, 1, 0);
	Mix_GroupChannels(2, 3, 1);
	Mix_GroupChannels(4, 5, 2);
	Mix_GroupChannels(6, 7, 3);
	Mix_GroupChannels(8, 9, 4);
	Mix_GroupChannels(10, 11, 5);
	//group for button sound
	Mix_GroupChannels(12, 61, 6);
	Mix_GroupChannels(62, 71, 7);//for long sound
	//group for bgm
	//mixer

	timer->start();
}

int Gamemain::loop() {//quit when you don't return 0
	{
		const Uint8 *curr_key_state = SDL_GetKeyboardState(NULL);
		if (curr_key_state[SDL_SCANCODE_U]) {
			input[0]++;
		} else {
			input[0] = (input[0]>0)?-1:0;
		}
		if (curr_key_state[SDL_SCANCODE_J]) {
			input[1]++;
		} else {
			input[1] = (input[1] > 0) ? -1 : 0;
		}
		if (curr_key_state[SDL_SCANCODE_N]) {
			input[2]++;
		} else {
			input[2] = (input[2] > 0) ? -1 : 0;
		}
		if (curr_key_state[SDL_SCANCODE_B]) {
			input[3]++;
		} else {
			input[3] = (input[3] > 0) ? -1 : 0;
		}
		if (curr_key_state[SDL_SCANCODE_G]) {
			input[4]++;
		} else {
			input[4] = (input[4] > 0) ? -1 : 0;
		}
		if (curr_key_state[SDL_SCANCODE_Y]) {
			input[5]++;
		} else {
			input[5] = (input[5] > 0) ? -1 : 0;
		}
	}
	//get button input(u,j,n,b,g,y->input[0]~input[5])

	SDL_SetRenderDrawColor(render, 0x33, 0x33, 0x33, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(render);
	SDL_RenderCopy(render, border, NULL, NULL);

	switch (mode) {
	case M_TITLE:title(); break;
	case M_CHOOSE:choose(); break;
	case M_LOADING:loading(); break;
	case M_PLAY:play(); break;
	case M_RESULT:result(); break;
	}
	SDL_RenderPresent(render);

	return 0;
}

void Gamemain::close(){
	for (int i = 0; i < 6; i++) {
		SDL_DestroyTexture(btn[i]);
		SDL_DestroyTexture(btn_p[i]);
	}
	for (int i = 0; i < 4; i++)SDL_DestroyTexture(letter_judge[i]);
	for (int i = 0; i < 3; i++)SDL_DestroyTexture(letter_timing[i]);
	bmsm.reset();
	qr.close();
	TTF_CloseFont(font);
	TTF_Quit();
}
