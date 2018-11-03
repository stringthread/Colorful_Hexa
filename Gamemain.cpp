#include "stdafx.h"
#include "ColorfulHexa.h"
#include <cmath>
#include <sstream>
#include <ios>
#include <iomanip>

void Gamemain::change_mode() {
	switch (mode) {
		case M_TITLE:
			//Mix_HaltMusic();
			//bmsm.play_bgm();
			mode = M_CHOOSE;
			break;
		case M_CHOOSE:
			t_load = thread([this]() {bmsm.load_music(render);});
			t_load.detach();
			step_choose = 0;
			Mix_HaltMusic();
			mode = M_LOADING;
			break;
		case M_LOADING:
			Mix_PlayChannel(-1, bgm_ready, 0);
			play_time = -4000l;
			play_start = timer->get_time()-play_time;
			mode = M_PLAY;
			break;
		case M_PLAY:
			Mix_HaltChannel(-1);
			Mix_PlayMusic(bgm_result, -1);
			mode = M_RESULT;
			break;
		case M_RESULT:
			Mix_HaltChannel(-1);
			bmsm.reset();
			qr.reset();
			qr.read_start();
			pushing_long.fill(0);
			judge.fill(0);
			score = 0;
			combo = 0;
			max_combo = 0;
			colors = 0;
			colorful_gauge = 0.0f;
			curr_judge = -1;
			Mix_PlayMusic(bgm_title, -1);
			mode = M_TITLE;
			break;
	}
	return;
}

void Gamemain::calc_score() {
	score = (long)((judge[0] * S_PERFECT + judge[1] * S_GREAT + judge[2] * S_GOOD)*100000.0f / bmsm.get_num_note());
	score += (long)(max_combo*10000.0f / bmsm.get_num_note());
	score += (long)(10000.0f * colorful_gauge/6);
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
	//colors = 5;
	}
	//*///color back
}

void Gamemain::draw_note(long count,int lane,int color,long end_count,bool is_bar){
	if (!is_bar&&(lane < 0 || lane >= 6))return;

	float dis_note_in,dis_note_out;
	if (count < bmsm.time_to_count(play_time) && end_count < bmsm.time_to_count(play_time))return;

	if (is_bar) {
		rect_note.h = (0.3f + 0.7f*(count - bmsm.time_to_count(play_time)) / (bmsm.time_to_count(play_time + bmsm.time_draw_start) - bmsm.time_to_count(play_time))) * scr_h;
		rect_note.w = rect_note.h*sin(pi / 3);
		rect_note.x = (scr_w - rect_note.w) / 2;
		rect_note.y = (scr_h - rect_note.h) / 2;
		SDL_RenderCopyAlpha(render, bar_line, NULL, &rect_note,128);
		return;
	}

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

		if(count>bmsm.time_to_count(play_time))draw_note(count, lane, 6,-1,false);
		if (end_count < bmsm.time_to_count(play_time + bmsm.time_draw_start))draw_note(end_count, lane, 6,-1,false);
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

bool Gamemain::check_judge(long count,bool add_judge,int *judge_container) {
	long now_count = bmsm.time_to_count(play_time-latency1);
	long judge_count[4] = {
		bmsm.time_to_count(play_time-latency1 + R_PERFECT * 1000 / FPS) - now_count,
		bmsm.time_to_count(play_time-latency1 + R_GREAT * 1000 / FPS) - now_count,
		bmsm.time_to_count(play_time-latency1 + R_GOOD * 1000 / FPS) - now_count,
		bmsm.time_to_count(play_time-latency1 + R_MISS * 1000 / FPS) - now_count,
	};
	long dis_count = abs(count - now_count);
	curr_timing=T_NONE;
	for (int i= 0; i < 4; i++) {
		if (dis_count > judge_count[i])continue;
		if(judge_container!=NULL)*judge_container = i;
		if (!add_judge) {
			return i < 2;
		}//perfect or great
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

		colorful_gauge += unit_col_gauge*COL_GAUGE[i];
		if (colorful_gauge < 0.0f)colorful_gauge = 0.0f;
		//colorful gauge
		return true;
	}
	if(judge_container!=NULL)*judge_container = -1;
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
		if (!qr.inited) {
			auto th = thread([this]{ qr.init(curr_dir); });
			th.detach();
		} else if (!qr.reading && !qr.read)qr.read_start();
		if (any_pressed() || qr.read) {
			if (qr.read)qr.check_hidden(&bmsm.hidden[0]);
			change_mode();
		}
		if(Mix_PlayingMusic()==0)Mix_PlayMusic(bgm_title, -1);
		SDL_RenderCopyAlpha(render, title_logo, NULL, &rect_title_logo, 180 + 75 * cos(0.001*timer->get_time()));
		//draw title screen
	} else {
		SDL_SetRenderDrawColor(render, 0xff, 0xaa, 0xff, SDL_ALPHA_OPAQUE);
		SDL_RenderClear(render);
		//draw opening screen
	}
	return 0;
}

int Gamemain::choose(){
	if (step_choose < 2) {
		letter_title.draw();
		composer.draw();
		SDL_RenderCopy(render, bmsm.get_stagefile(), NULL, &rect_stagefile);
		SDL_RenderCopy(render, bmsm.get_stagefile(-1), NULL, &rect_l);
		SDL_RenderCopy(render, bmsm.get_stagefile(1), NULL, &rect_r);
		SDL_RenderCopy(render, easy, NULL, &rect_easy);
		SDL_RenderCopy(render, normal, NULL, &rect_normal);
		SDL_RenderCopy(render, hard, NULL, &rect_hard);
		lv_easy.draw();
		lv_normal.draw();
		lv_hard.draw();
		notesdesigner.draw();
	}
	if (step_choose == 0){
		if (input[0] == -1 || input[5] == -1)step_choose++;
		SDL_RenderCopy(render, arr, NULL, &rect_arr_l_0);
		SDL_RenderCopyEx(render, arr, NULL, &rect_arr_r_0, 0, NULL, SDL_FLIP_HORIZONTAL);
		if (input[1] == 1 || (input[1] >= 60 && (input[1] - 60) % 20 == 0)) {
			bmsm.change_sel(1);
			letter_title.set_text(bmsm.get_title());
			composer.set_text(bmsm.get_composer());
			lv_easy.set_center(to_string(bmsm.get_playlevel(0)), p_lv_easy.x, p_lv_easy.y, scr_h*0.08f, f_color);
			lv_normal.set_center(to_string(bmsm.get_playlevel(1)), p_lv_normal.x, p_lv_normal.y, scr_h*0.08f, f_color);
			lv_hard.set_center(to_string(bmsm.get_playlevel(2)), p_lv_hard.x, p_lv_hard.y, scr_h*0.08f, f_color);
			notesdesigner.set_text(bmsm.get_notesdesigner());
			//Mix_HaltMusic();
			//bmsm.play_bgm();
		}
		if (input[4] == 1 || (input[4] >= 60 && (input[4] - 60) % 20 == 0)) {
			bmsm.change_sel(-1);
			letter_title.set_text(bmsm.get_title());
			composer.set_text(bmsm.get_composer());
			lv_easy.set_center(to_string(bmsm.get_playlevel(0)), p_lv_easy.x, p_lv_easy.y, scr_h*0.08f, f_color);
			lv_normal.set_center(to_string(bmsm.get_playlevel(1)), p_lv_normal.x, p_lv_normal.y, scr_h*0.08f, f_color);
			lv_hard.set_center(to_string(bmsm.get_playlevel(2)), p_lv_hard.x, p_lv_hard.y, scr_h*0.08f, f_color);
			notesdesigner.set_text(bmsm.get_notesdesigner());
			//Mix_HaltMusic();
			//bmsm.play_bgm();
		}
	}else if(step_choose==1){
		if (input[2] == -1 || input[3] == -1)step_choose--;
		if (input[0] == -1 || input[5] == -1)step_choose++;
		SDL_RenderCopy(render, arr, NULL, &rect_arr_l[bmsm.get_lv()]);
		SDL_RenderCopyEx(render, arr, NULL, &rect_arr_r[bmsm.get_lv()], 0, NULL, SDL_FLIP_HORIZONTAL);
		if (input[1] == 1 || (input[1] >= 60 && (input[1] - 60) % 20 == 0)) {
			bmsm.change_lv(1);
		}
		if (input[4] == 1 || (input[4] >= 60 && (input[4] - 60) % 20 == 0)) {
			bmsm.change_lv(-1);
		}
	} else if (step_choose == 2) {
		if (input[2] == -1 || input[3] == -1)step_choose--;
		if (input[0] == -1 || input[5] == -1) {
			step_choose++;
			speed_conf.set_center("Speed: "+digit(bmsm.get_speed(),2),p_speed_conf.x,p_speed_conf.y,scr_h*0.05f,f_color);
		}
		if (input[1] == 1 || (input[1] >= 60 && (input[1] - 60) % 20 == 0)) bmsm.change_speed(0.25f);
		if (input[4] == 1 || (input[4] >= 60 && (input[4] - 60) % 20 == 0)) bmsm.change_speed(-0.25f);

		SDL_RenderCopyAlpha(render, black_back, NULL, NULL,16);
		cap_speed.draw();
		s_letter = TTF_RenderText_Blended(font, digit(bmsm.get_speed(), 2).c_str(), f_color);
		letter = SDL_CreateTextureFromSurface(render, s_letter);
		SDL_FreeSurface(s_letter);
		rect_center.w = rect_center.h*1.5f;
		rect_center.x = (scr_w - rect_center.w) / 2;
		SDL_RenderCopy(render, letter, NULL, &rect_center);
		SDL_DestroyTexture(letter);
		SDL_RenderCopy(render, arr, NULL, &rect_arr_l_0);
		SDL_RenderCopyEx(render, arr, NULL, &rect_arr_r_0, 0, NULL, SDL_FLIP_HORIZONTAL);
		//draw choose speed screen
	}else if(step_choose==3){
		if (input[2] == -1 || input[3] == -1) {
			step_choose--;
			return 0;
		}
		if (any_pressed()) {
			step_choose = 0;
			change_mode();//start loading data
		}
		letter_title.draw();
		composer.draw();
		SDL_RenderCopy(render, bmsm.get_stagefile(), NULL, &rect_stagefile);
		switch (bmsm.get_lv()) {
			case 0:
			SDL_RenderCopy(render, easy, NULL, &rect_easy);
			lv_easy.draw();
			break;
			case 1:
			SDL_RenderCopy(render, normal, NULL, &rect_normal);
			lv_normal.draw();
			break;
			case 2:
			SDL_RenderCopy(render, hard, NULL, &rect_hard);
			lv_hard.draw();
			break;
		}
		speed_conf.draw();
	}
	return 0;
}

int Gamemain::loading() {
	if (bmsm.music_loaded) {
		unit_col_gauge = 12.0f/bmsm.get_num_note();
		bmsm.music_loaded = false;
		change_mode();
		//draw play start screen
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
				if (notes[i][j].get_count()<=bmsm.time_to_count(play_time-latency2)) {
					if (bmsm.time_to_count(play_time-latency1) < 2 * bmsm.BAR_STANDARD) {
						ch = Mix_GroupAvailable(7);
						if (ch != -1)goto play;
					}
					ch = Mix_GroupAvailable(6);
					if (ch == -1) {
						ch = Mix_GroupOldest(6);
						if (Mix_Playing(ch) == 1)Mix_HaltChannel(ch);
					}
				play:
					Mix_PlayChannel(ch, bmsm.get_wav(notes[i][j].get_data()), 0);
					bmsm.set_pushed(0, notes[i][j].get_index());
				}
			}
			//bgm

			if (i == 0)continue;
			if (input[i-1] == 1) {
				if (notes[i][j].pushed)continue;
				if (notes[i][j].is_ln()) {
					if(ln_judge[i-1]=check_judge(notes[i][j].get_count(),true,&ln_judge[i-1]))pushing_long[i - 1] = true;
				}
				else if (check_judge(notes[i][j].get_count()))bmsm.set_pushed(i + 7, notes[i][j].get_index());
			} else if (input[i - 1] == -1 && pushing_long[i-1]&&notes[i][j].is_ln()) {
				pushing_long[i - 1] = false;
				bmsm.set_pushed(i + 14, notes[i][j].get_index());
				if (!check_judge(notes[i][j].get_end_count(), false)&&notes[i][j].get_end_count()>bmsm.time_to_count(play_time)) {
					combo = 0;
					judge[3]++;
					colorful_gauge += unit_col_gauge*COL_GAUGE[3];
					if (colorful_gauge < 0.0f)colorful_gauge = 0.0f;
					curr_judge = 3;
					curr_timing = T_FAST;
					if (ln_judge[i - 1] >= 0 && ln_judge[i - 1] < 3) {
						if(judge[ln_judge[i-1]]>0)judge[ln_judge[i - 1]]--;
						colorful_gauge -= unit_col_gauge*COL_GAUGE[ln_judge[i - 1]];
						if (colorful_gauge < 0.0f)colorful_gauge = 0.0f;
						ln_judge[i - 1] = -1;
					}
				}
			}
			if (notes[i][j].get_count() < bmsm.time_to_count(play_time - R_GOOD * 1000 / FPS)) {
				if (i != 0) {
					if (notes[i][j].get_end_count() < bmsm.time_to_count(play_time - R_GOOD * 1000 / FPS)) {
						if (notes[i][j].is_ln() && pushing_long[i - 1])pushing_long[i - 1] = false;
						else if (!notes[i][j].pushed) {
							combo = 0;
							judge[3]++;
							colorful_gauge += unit_col_gauge*COL_GAUGE[3];
							if (colorful_gauge < 0.0f)colorful_gauge = 0.0f;
							curr_judge = 3;
							curr_timing = T_LATE;
							bmsm.set_pushed(notes[i][j].is_ln() ? (i + 14) : (i + 7), notes[i][j].get_index());
						}
						bmsm.set_next_note(notes[i][j].is_ln() ? i + 14 : i + 7);
					}
				}
				continue;
			}
			if (notes[i][j].pushed&&notes[i][j].get_count() < bmsm.time_to_count(play_time) && notes[i][j].get_end_count() < bmsm.time_to_count(play_time)) {
				bmsm.set_next_note(notes[i][j].is_ln() ? i + 14 : i + 7);
				continue;
			}
			//check if note in range
			break;
		}
	}
	calc_score();
	if (bmsm.get_total_count() < bmsm.time_to_count(play_time-2000))change_mode();
	//calculation

	SDL_SetRenderDrawColor(render, COLOR_HEX[colors][0], COLOR_HEX[colors][1], COLOR_HEX[colors][2], SDL_ALPHA_OPAQUE);
	//rect_col.h = (int)(1.732f/3*2*rect_col.w*colorful_gauge);//no color back
	rect_col.h = (int)(1.732f / 3 * 2*rect_col.w*(colorful_gauge-colors));//color back
	rect_col.y = scr_h / 2 + (int)(1.732f/3*rect_col.w) - rect_col.h;
	SDL_RenderFillRect(render,&rect_col);

	for (int i= 0; i <= 6; i++) {
		for (int j = 0; j < notes[i+1].size(); j++) {
			draw_note(notes[i+1][j].get_count(),i,(colors==6)?i:colors,notes[i+1][j].get_end_count(),i==6);
		}
		if (i == 6)continue;
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
	if (any_pressed())change_mode();
	if (qr.read) {
		qr.send(bmsm.get_lv(),bmsm.get_sel(),score);
		qr.reset();
	}
	for (rank = 0; rank < 9;) {
		if (score >= range_rank[rank])break;
		else rank++;
	}

	s_letter = TTF_RenderUTF8_Blended(font, ("Lv."+digit((int)bmsm.get_playlevel(), 2)).c_str(), f_color);
	letter = SDL_CreateTextureFromSurface(render, s_letter);
	SDL_FreeSurface(s_letter);
	SDL_RenderCopy(render, letter, NULL, &rect_score);
	SDL_DestroyTexture(letter);
	s_letter = TTF_RenderUTF8_Blended(font, text_lv[bmsm.get_lv()].c_str(), f_color);
	letter = SDL_CreateTextureFromSurface(render, s_letter);
	SDL_FreeSurface(s_letter);
	SDL_RenderCopy(render, letter, NULL, &rect_lv_res);
	SDL_DestroyTexture(letter);
	s_letter = TTF_RenderUTF8_Blended(font, ("Score: "+digit((int)score,6)).c_str(), f_color);
	letter = SDL_CreateTextureFromSurface(render, s_letter);
	SDL_FreeSurface(s_letter);
	SDL_QueryTexture(letter, NULL, NULL, &tex_w, &tex_h);
	rect_res_score.w = rect_res_score.h*tex_w / tex_h;
	rect_res_score.x = (scr_w - rect_res_score.w) / 2;
	SDL_RenderCopy(render, letter, NULL, &rect_res_score);
	SDL_DestroyTexture(letter);
	SDL_RenderCopy(render, letter_rank[rank], NULL, &rect_rank);
	s_letter = TTF_RenderUTF8_Blended(font, ("Max Combo: " + digit((int)max_combo, 3)).c_str(), f_color);
	letter = SDL_CreateTextureFromSurface(render, s_letter);
	SDL_FreeSurface(s_letter);
	SDL_QueryTexture(letter, NULL, NULL, &tex_w, &tex_h);
	rect_res_combo.w = rect_res_combo.h*tex_w / tex_h;
	rect_res_combo.x = (scr_w - rect_res_combo.w) / 2;
	SDL_RenderCopy(render, letter, NULL, &rect_res_combo);
	SDL_DestroyTexture(letter);
	s_letter = TTF_RenderUTF8_Blended(font, ("Perfect: " + digit((int)judge[0], 3)).c_str(), f_color);
	letter = SDL_CreateTextureFromSurface(render, s_letter);
	SDL_FreeSurface(s_letter);
	SDL_QueryTexture(letter, NULL, NULL, &tex_w, &tex_h);
	rect_perfect.w = rect_perfect.h*tex_w / tex_h;
	rect_perfect.x = (scr_w - rect_perfect.w) / 2;
	SDL_RenderCopy(render, letter, NULL, &rect_perfect);
	SDL_DestroyTexture(letter);
	s_letter = TTF_RenderUTF8_Blended(font, ("Great: " + digit((int)judge[1], 3)).c_str(), f_color);
	letter = SDL_CreateTextureFromSurface(render, s_letter);
	SDL_FreeSurface(s_letter);
	SDL_QueryTexture(letter, NULL, NULL, &tex_w, &tex_h);
	rect_great.w = rect_great.h*tex_w / tex_h;
	rect_great.x = (scr_w - rect_great.w) / 2;
	SDL_RenderCopy(render, letter, NULL, &rect_great);
	SDL_DestroyTexture(letter);
	s_letter = TTF_RenderUTF8_Blended(font, ("Good: " + digit((int)judge[2], 3)).c_str(), f_color);
	letter = SDL_CreateTextureFromSurface(render, s_letter);
	SDL_FreeSurface(s_letter);
	SDL_QueryTexture(letter, NULL, NULL, &tex_w, &tex_h);
	rect_good.w = rect_good.h*tex_w / tex_h;
	rect_good.x = (scr_w - rect_good.w) / 2;
	SDL_RenderCopy(render, letter, NULL, &rect_good);
	SDL_DestroyTexture(letter);
	s_letter = TTF_RenderUTF8_Blended(font, ("Miss: " + digit((int)judge[3], 3)).c_str(), f_color);
	letter = SDL_CreateTextureFromSurface(render, s_letter);
	SDL_FreeSurface(s_letter);
	SDL_QueryTexture(letter, NULL, NULL, &tex_w, &tex_h);
	rect_miss.w = rect_miss.h*tex_w / tex_h;
	rect_miss.x = (scr_w - rect_miss.w) / 2;
	SDL_RenderCopy(render, letter, NULL, &rect_miss);
	SDL_DestroyTexture(letter);
	//draw result screen
	return 0;
}

void Gamemain::init(){
	SDL_Surface* tmp = NULL;
	TTF_Init();
	char c_curr_dir[256] = {};
	GetModuleFileName(NULL, &c_curr_dir[0], 256);
	curr_dir=regex_replace(c_curr_dir, regex("\\\\[^\\\\]+$"), "");
	font = TTF_OpenFont((curr_dir + "\\font\\font.otf").c_str(), 200);
	//font

	rect_btn[0].x = scr_w / 2; rect_btn[0].y = int(0.35f*scr_h); rect_btn[0].w = int(0.15f*scr_w); rect_btn[0].h = int(0.1125f*scr_h);
	rect_btn[1].x = int(0.575f*scr_w); rect_btn[1].y = int(0.425f*scr_h); rect_btn[1].w = int(0.075f*scr_w); rect_btn[1].h = int(0.15f*scr_h);
	rect_btn[2].x = rect_btn[0].x; rect_btn[2].y = int(0.5375f*scr_h); rect_btn[2].w = rect_btn[0].w; rect_btn[2].h = rect_btn[0].h;
	rect_btn[3].x = int(0.35f*scr_w); rect_btn[3].y = rect_btn[2].y; rect_btn[3].w = rect_btn[0].w; rect_btn[3].h = rect_btn[0].h;
	rect_btn[4].x = rect_btn[3].x; rect_btn[4].y = rect_btn[1].y; rect_btn[4].w = rect_btn[1].w; rect_btn[4].h = rect_btn[1].h;
	rect_btn[5].x = rect_btn[3].x; rect_btn[5].y = rect_btn[0].y; rect_btn[5].w = rect_btn[0].w; rect_btn[5].h = rect_btn[0].h;
	rect_center = { int(scr_w*0.3f),int(scr_h*0.4f),int(scr_w*0.4f),int(scr_h*0.2f) };
	rect_bottom= { int(scr_w*0.4f),int(scr_h*0.65f),int(scr_w*0.15f),int(scr_h*0.15f) };
	rect_stagefile.w = rect_stagefile.h = scr_h*0.3f;
	rect_stagefile.x = (scr_w - rect_stagefile.w) / 2;
	rect_stagefile.y = scr_h*0.3f;
	rect_r.w = rect_l.w = scr_w*0.1f;
	rect_r.x = scr_w - rect_r.w; rect_l.x = 0;
	rect_r.h = rect_l.h = scr_h*0.2f;
	rect_r.y = rect_l.y = scr_h*0.35f;
	rect_arr_r_0.w = rect_arr_l_0.w = scr_w * 0.05f;
	rect_arr_r_0.h = rect_arr_l_0.h = scr_h * 0.05f;
	rect_arr_r_0.y = rect_arr_l_0.y = scr_h * 0.45f - rect_arr_r_0.h / 2;
	rect_arr_l_0.x = scr_w*0.15f; rect_arr_r_0.x = scr_w*0.85f - rect_arr_r_0.w;
	rect_easy.y = rect_normal.y = rect_hard.y = scr_h * 0.625f;
	rect_easy.h = rect_normal.h = rect_hard.h = scr_h * 0.05f;
	rect_easy.w = rect_normal.w = rect_hard.w = scr_h * 0.125f;
	rect_easy.x = scr_h * 0.075f; rect_normal.x = scr_w * 0.5f - rect_normal.w / 2; rect_hard.x = scr_w-(rect_easy.x+rect_hard.w);
	rect_notesdesigner.x = scr_w * 0.2f;
	rect_notesdesigner.y = scr_h * 0.8f;
	rect_notesdesigner.w = scr_w * 0.6f;
	rect_notesdesigner.h = scr_h * 0.03f;
	rect_title = { int(scr_w*0.25f),int(scr_h*0.1f),scr_w/2,int(scr_h*0.1f) };
	rect_composer = { int(scr_w*0.15f),int(scr_h*0.22f),int(scr_w*0.7f),int(scr_h*0.05f) };
	rect_arr_l[0].w = rect_arr_l[1].w = rect_arr_l[2].w = rect_arr_l[3].w = rect_arr_r[0].w = rect_arr_r[1].w = rect_arr_r[2].w = rect_arr_r[3].w =scr_w * 0.03f;
	rect_arr_l[0].h = rect_arr_l[1].h = rect_arr_l[2].h = rect_arr_l[3].h = rect_arr_r[0].h = rect_arr_r[1].h = rect_arr_r[2].h = rect_arr_r[3].h =scr_h * 0.03f;
	rect_arr_l[0].y = rect_arr_l[1].y = rect_arr_l[2].y = rect_arr_l[3].y = rect_arr_r[0].y = rect_arr_r[1].y = rect_arr_r[2].y = rect_arr_r[3].y= scr_h * 0.635f;
	rect_arr_l[0].x = rect_easy.x - scr_w * 0.05f;
	rect_arr_l[1].x = rect_arr_l[0].x+rect_normal.x - rect_easy.x;
	rect_arr_l[2].x = rect_arr_l[0].x + rect_hard.x-rect_easy.x;
	rect_arr_r[0].x = rect_easy.x + rect_easy.w + scr_w * 0.01f;
	rect_arr_r[1].x = rect_arr_r[0].x + rect_normal.x - rect_easy.x;
	rect_arr_r[2].x = rect_arr_r[0].x + rect_hard.x - rect_easy.x;
	p_lv_easy = {int(scr_w*0.1425f),int(scr_h*0.725f)};
	p_lv_normal = { int(scr_w*0.5f),int(scr_h*0.725f) };
	p_lv_hard = { int(scr_w*0.8375f),int(scr_h*0.725f) };
	p_speed_conf.x = scr_w/2;
	p_speed_conf.y = scr_h * 0.82f;
	speed_conf.init(render, font);

	tmp = IMG_Load((string(curr_dir) + "\\data\\arrow_r.png").c_str());
	arr = SDL_CreateTextureFromSurface(render, tmp);
	SDL_FreeSurface(tmp);
	tmp = IMG_Load((string(curr_dir) + "\\data\\easy.png").c_str());
	easy = SDL_CreateTextureFromSurface(render, tmp);
	SDL_FreeSurface(tmp);
	tmp = IMG_Load((string(curr_dir) + "\\data\\normal.png").c_str());
	normal = SDL_CreateTextureFromSurface(render, tmp);
	SDL_FreeSurface(tmp);
	tmp = IMG_Load((string(curr_dir) + "\\data\\hard.png").c_str());
	hard = SDL_CreateTextureFromSurface(render, tmp);
	SDL_FreeSurface(tmp);
	tmp = IMG_Load((string(curr_dir) + "\\data\\black_back.png").c_str());
	black_back = SDL_CreateTextureFromSurface(render, tmp);
	SDL_FreeSurface(tmp);
	//choose

	rect_col.x = rect_btn[4].x + rect_btn[4].w; rect_col.w = rect_btn[1].x - rect_col.x;
	tmp = IMG_Load((string(curr_dir) + "\\data\\border.png").c_str());
	border = SDL_CreateTextureFromSurface(render, tmp);
	SDL_FreeSurface(tmp);
	tmp = IMG_Load((string(curr_dir) + "\\data\\title_logo.png").c_str());
	title_logo = SDL_CreateTextureFromSurface(render, tmp);
	SDL_FreeSurface(tmp);
	SDL_QueryTexture(title_logo, NULL,NULL, &tex_w, &tex_h);
	rect_title_logo.w = scr_w * 0.8f;
	rect_title_logo.h = rect_title_logo.w*tex_h / tex_w;
	rect_title_logo.x = (scr_w-rect_title_logo.w)/2;
	rect_title_logo.y = scr_h * 0.55f - rect_title_logo.h / 2;
	tmp = IMG_Load((string(curr_dir) + "\\data\\bar_line.png").c_str());
	bar_line = SDL_CreateTextureFromSurface(render, tmp);
	SDL_FreeSurface(tmp);
	for (int i= 0; i < 6; i++) {
		tmp = IMG_Load((string(curr_dir) + "\\data\\btn_" + to_string(i)+".png").c_str());
		btn[i] = SDL_CreateTextureFromSurface(render,tmp);
		SDL_FreeSurface(tmp);
		tmp = IMG_Load((string(curr_dir) + "\\data\\btn_p_" + to_string(i)+".png").c_str());
		btn_p[i] = SDL_CreateTextureFromSurface(render, tmp);
		SDL_FreeSurface(tmp);
		tmp = IMG_Load((string(curr_dir) + "\\data\\btn_col_" + to_string(i) + ".png").c_str());
		btn_col[i] = SDL_CreateTextureFromSurface(render, tmp);
		SDL_FreeSurface(tmp);
		tmp = IMG_Load((string(curr_dir) + "\\data\\n_" + to_string(i) + ".png").c_str());
		note[i] = SDL_CreateTextureFromSurface(render, tmp);
		SDL_FreeSurface(tmp);
		for (int j = 0; j < 2; j++) {
			tmp = IMG_Load((string(curr_dir) + "\\data\\ln_" + to_string(i) + "_" + to_string(j) + ".png").c_str());
			ln[i][j] = SDL_CreateTextureFromSurface(render, tmp);
			SDL_FreeSurface(tmp);
		}
	}
	for (int i = 0; i < 3; i++) {
		SDL_QueryTexture(ln[0][i], NULL, NULL, &tex_w, &tex_h);
		size_ln[i] = float(tex_w) / float(tex_h);
	}
	tmp = IMG_Load((string(curr_dir) + "\\data\\ln_end.png").c_str());
	note[6] = SDL_CreateTextureFromSurface(render, tmp);
	SDL_FreeSurface(tmp);

	tmp = IMG_Load((string(curr_dir) + "\\data\\Perfect.png").c_str());
	letter_judge[0] = SDL_CreateTextureFromSurface(render, tmp);
	SDL_FreeSurface(tmp);
	tmp = IMG_Load((string(curr_dir) + "\\data\\Great.png").c_str());
	letter_judge[1] = SDL_CreateTextureFromSurface(render, tmp);
	SDL_FreeSurface(tmp);
	tmp = IMG_Load((string(curr_dir) + "\\data\\Good.png").c_str());
	letter_judge[2] = SDL_CreateTextureFromSurface(render, tmp);
	SDL_FreeSurface(tmp);
	tmp = IMG_Load((string(curr_dir) + "\\data\\Miss.png").c_str());
	letter_judge[3] = SDL_CreateTextureFromSurface(render, tmp);
	SDL_FreeSurface(tmp);
	rect_judge.x = int(scr_w*0.4f);
	rect_judge.w = int(scr_w*0.2f);
	SDL_QueryTexture(letter_judge[0], NULL, NULL, &tex_w, &tex_h);
	rect_judge.h = rect_judge.w*tex_h / tex_w;
	rect_judge.y = scr_h / 2 - rect_judge.h;
	bg_1.init(render, (string(curr_dir) + "\\data\\bg_1.png").c_str());
	bg_2.init(render, (string(curr_dir) + "\\data\\bg_2.png").c_str());

	tmp = IMG_Load((string(curr_dir) + "\\data\\Just.png").c_str());
	letter_timing[0] = SDL_CreateTextureFromSurface(render, tmp);
	SDL_FreeSurface(tmp);
	tmp = IMG_Load((string(curr_dir) + "\\data\\Fast.png").c_str());
	letter_timing[1] = SDL_CreateTextureFromSurface(render, tmp);
	SDL_FreeSurface(tmp);
	tmp = IMG_Load((string(curr_dir) + "\\data\\Late.png").c_str());
	letter_timing[2] = SDL_CreateTextureFromSurface(render, tmp);
	SDL_FreeSurface(tmp);
	rect_timing.x = int(scr_w*0.4f);
	rect_timing.w = int(scr_w*0.2f);
	SDL_QueryTexture(letter_timing[0], NULL, NULL, &tex_w, &tex_h);
	rect_timing.h = rect_timing.w*tex_h / tex_w;
	rect_timing.y = scr_h / 2;
	for (int i = 0; i < 10; i++) {
		tmp = IMG_Load((curr_dir + "\\data\\rank_" + to_string(i) + ".png").c_str());
		letter_rank[i] = SDL_CreateTextureFromSurface(render, tmp);
		SDL_FreeSurface(tmp);
	}
	//image

	bgm_ready = Mix_LoadWAV((curr_dir + "\\data\\bgm_ready.wav").c_str());

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
	bgm_title = Mix_LoadMUS((curr_dir + "\\data\\bgm_title.wav").c_str());
	bgm_result = Mix_LoadMUS((curr_dir + "\\data\\bgm_result.wav").c_str());
	//mixer

	ifstream ifs(curr_dir + "\\data\\config.txt");
	string line;
	while (getline(ifs, line)) {
		if (line.find("latency")!=0)continue;
		if (line.find("latency1:") == 0)latency1 = stol(line.substr(9));
		if (line.find("latency2:") == 0)latency2 = stol(line.substr(9));
	}
	ifs.close();
	//config.txt

	letter_title.init(render, font, f_color, &rect_title, bmsm.get_title(), 0.01f);
	composer.init(render, font, f_color, &rect_composer, bmsm.get_composer(), 0.01f);
	lv_easy.init(render, font);
	lv_normal.init(render, font);
	lv_hard.init(render, font);
	cap_speed.init(render, font);
	notesdesigner.init(render, font, f_color, &rect_notesdesigner, bmsm.get_notesdesigner(), 0.01f);
	letter_title.set_text(bmsm.get_title());
	composer.set_text(bmsm.get_composer());
	lv_easy.set_center(to_string(bmsm.get_playlevel(0)), p_lv_easy.x, p_lv_easy.y, scr_h*0.08f, f_color);
	lv_normal.set_center(to_string(bmsm.get_playlevel(1)), p_lv_normal.x, p_lv_normal.y, scr_h*0.08f, f_color);
	lv_hard.set_center(to_string(bmsm.get_playlevel(2)), p_lv_hard.x, p_lv_hard.y, scr_h*0.08f, f_color);
	cap_speed.set_center("Speed", scr_w / 2, int(scr_h*0.2f), int(scr_h*0.1f), f_color);
	notesdesigner.set_text(bmsm.get_notesdesigner());

	//qr.userid = "16a9";
	//qr.send(2, 0, 120000);

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
	bg_1.draw();
	bg_2.draw();
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
