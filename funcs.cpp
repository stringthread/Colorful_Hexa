#include "stdafx.h"
#include "ColorfulHexa.h"
#include <mmsystem.h>

//Timer
Timer::Timer() {
	start();
}
void Timer::start(){
	high = QueryPerformanceFrequency(&freq);
	if (high) {
		freq.QuadPart /= 1000l;
		QueryPerformanceCounter(&start_time);
		start_time.QuadPart /= freq.QuadPart;
	}
	else {
		freq.QuadPart = 1;
		start_time.QuadPart = timeGetTime();
		timeBeginPeriod(1);
	}
	play_time.QuadPart = 0;
	frames = 0;
}
void Timer::sleep(){
	frames++;
	if (high) {
		QueryPerformanceCounter(&current_time);
		current_time.QuadPart /= freq.QuadPart;
	}
	else {
		current_time.QuadPart = timeGetTime();
	}
	play_time.QuadPart = current_time.QuadPart - start_time.QuadPart;
	if (play_time.QuadPart < frames*frametime) {
		Sleep((long)(frames*frametime)-(long)(play_time.QuadPart));
	}
	return;
}
void Timer::close(){
	timeEndPeriod(1);
}

//QR_Reader
int QR_Reader::init(string curr_dir) {
	lock_guard<mutex> lg(m);
	zbar = _popen((curr_dir+"\\zbarcam").c_str(), "r");
	_inited = true;
	return 0;
}
void QR_Reader::read_start(){
	m.lock();
	timer.start();
	timer.set_frametime(2000);
	_reading = true;
	m.unlock();
	auto th = thread([this] {
		while (1) {
			timer.sleep();
			if (flg_break()) {
				flg_break(false);
				break;
			}
			if (!inited() || read()) continue;
			m.lock();
			fgets(buff, sizeof(buff), zbar);
			tmp_str = string(buff);
			if (tmp_str==""||tmp_str.find("https://") == string::npos || tmp_str.compare(tmp_str.size() - 4, 4, userid) == 0)continue;
			userid = tmp_str.substr(tmp_str.size() - 5, 4);
			_read = true;
			m.unlock();
			break;
		}
	});
	th.detach();
}
void QR_Reader::reset(){
	lock_guard<mutex> lg(m);
	userid = "";
	_read = false;
	_flg_break = true;
	return;
}
int QR_Reader::send(int rank,int song,int score){
	lock_guard<mutex> lg(m);
	curl = curl_easy_init();
	string chunk,url,post_data;
	url = "http://tk67ennichi.official.jp/enpass/database.php";
	post_data = "mode='set'&id='"+userid+"'&password='torisasamiyukke'&attrac='colorfulhexa'&score_mode="+to_string(rank)+"&song="+to_string(song)+"&score="+to_string(score);
	//url = "http://localhost/test/test.php";
	//post_data = "test=qwerty";
	if (curl){
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, post_data.size());
		//curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
		//curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback_send);
		curl_easy_setopt(curl, CURLOPT_PROXY, "");
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
	}
	if (res != CURLE_OK) {
		return -1;
	}
	return 0;
}
size_t QR_Reader::callback_send(char* ptr, size_t size, size_t nmemb, bool* stream) {
	return size * nmemb;
}
void QR_Reader::check_hidden(bool *hidden) {
	lock_guard<mutex> lg(m);
	curl = curl_easy_init();
	string chunk, url,post_data;
	url = "http://tk67ennichi.official.jp/enpass/database.php";
	post_data = "mode=special_song&id="+userid;
	//url = "http://localhost/test/test.php";
	//post_data = "test=qwerty";
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, post_data.size());
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, hidden);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
		curl_easy_setopt(curl, CURLOPT_PROXY, "");
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
	}
	if (res != CURLE_OK) {
		err_count++;
		if (err_count < 5)check_hidden(hidden);
		else err_count = 0;
		return;
	}
	err_count = 0;
	return;
}
size_t QR_Reader::callback(char* ptr, size_t size, size_t nmemb, bool* stream){
	string ret = string(ptr);
	stream[0] = (ret == "2" || ret == "3");
	stream[1] = (ret == "1" || ret == "3");
	return size*nmemb;
}
void QR_Reader::close(){
	_pclose(zbar);
}

//BMS_Manager
//BMS_Manager::BMS_Manager(SDL_Renderer* render) {
void BMS_Manager::init(SDL_Renderer* render, string curr_dir){
	//auto th = thread([this,render] {
		HANDLE h_dir, h_file;
		WIN32_FIND_DATA dir_data, file_data;
		string dir_path,root_dir;
		string wildcards[] = { "*.bms","*.bme","*.bml" };
		ifstream ifs_info(curr_dir+ "\\song\\info.txt");
		string line;
		if (!ifs_info.fail()) {
			while (getline(ifs_info, line)) {
				if (line.find("num:") == 0) {
					try {
						header.reserve(stoi(line.substr(4)));
					}
					catch(const invalid_argument& e){}
					catch (const out_of_range& e) {}
				}
			}
		}
		ifs_info.close();
		if (header.capacity() == 0) header.reserve(10);
		bool is_first;
		//char_traits<char>::copy(curr_dir, regex_replace(curr_dir, regex("\\\\[^\\\\]+$"), "").c_str(), 256);
		h_dir = FindFirstFile((curr_dir + "\\song\\*").c_str(), &dir_data);
		if (h_dir == INVALID_HANDLE_VALUE) {
			if (GetLastError() != ERROR_NO_MORE_FILES)error = -1;
		}
		while (error == 0) {
			if ((dir_data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) && string(dir_data.cFileName).compare(".") != 0 && string(dir_data.cFileName).compare("..") != 0) {
				is_first = true;
				dir_path = curr_dir + "\\song\\" + dir_data.cFileName + "\\";
				for (int i = 0; i < 3; i++) {
					error = 0;
					h_file = FindFirstFile((dir_path + wildcards[i]).c_str(), &file_data);
					//Sleep(0);
					if (h_file == INVALID_HANDLE_VALUE) {
						continue;
					}
					while (error == 0) {
						if (string(file_data.cFileName).compare(".") != 0 && string(file_data.cFileName).compare("..") != 0) {
							if (is_first) { header.emplace_back(); }
							error = header.back().load(dir_path, file_data.cFileName, is_first, render);
							is_first = false;
							if (!FindNextFile(h_file, &file_data) || error != 0) {
								if (GetLastError() != ERROR_NO_MORE_FILES)error = -1;
								header.back().sort();
								FindClose(h_file);
								break;
							}
						}
						//Sleep(0);
					}
				}
			}
			if (!FindNextFile(h_dir, &dir_data)) {
				if (GetLastError() != ERROR_NO_MORE_FILES)error = -1;
				FindClose(h_dir);
				break;
			}
			//Sleep(0);
		}
		if (header.size() == 0) {
			error = -1;
		}
		set_header_loaded(true);
	/*});
	th.detach();*/
	//load header
}
int BMS_Manager::Header::load(string dir,string file,bool is_first,SDL_Renderer* render) {
	this->dir = dir;
	file = dir + file;
	bmsfile.push_back(file);
	ifstream ifs(bmsfile.back());
	string line,data;
	SDL_Surface* tmp;
	bool has_total=false;

	bar_num.push_back(1);
	//set default

	if (ifs.fail())return -1;
	num++;
	while(getline(ifs,line)){
		if (line[0] != '#')continue;
		if (regex_search(line, regex("^#[0-9]{5}"))) {
			if (stoi(line.substr(1, 3)) > bar_num.back())bar_num.back() = stoi(line.substr(1, 3))+1;
		}
		has_total = false;
		for (int i= 0; i < 12; i++) {
			if (!is_first && (i == 0 || i == 1 || i == 2 || i == 4 || i == 7 || i == 9||i==11))continue;
			if (!regex_search(line, regex("^#" + cmd[i] + "[ \t]+")))continue;
			data = regex_replace(line,regex("^#[^ \t]+[ \t]+"),"");
			switch (i) {
			case 0:
				genre = data;
				break;
			case 1:
				title = data;
				break;
			case 2:
				artist = data;
				break;
			case 3:
				bpm.push_back(data);
				break;
			case 4:
				midifile = data;
				break;
			case 5:
				playlevel.push_back(stoi(data));
				break;
			case 6:
				rank.push_back(stoi(data));
				break;
			case 7:
				volwav = stoi(data);
				break;
			case 8:
				total.push_back(stoi(data));
				has_total = true;
				break;
			case 9:
				if (!stagefile.is_free()) break;
				stagefile.init(dir + data);
				break;
			case 10:
				notesdesigner = data;
				break;
			case 11:
				//bgm.init(dir + data);
				break;
			}
		}
		Sleep(0);
	}
	ifs.close();
	if (bpm.size() < num)bpm.push_back("130");
	if (!has_total)total.push_back(300l);
	//Sleep(0);
	return 0;
}
void BMS_Manager::Header::sort() {
	class Data {
	public:
		string bpm;
		int playlevel;
		int rank;
		long total;
		Data(string s_bpm, int i_playlevel, int i_rank, long l_total) {
			bpm = s_bpm;
			playlevel = i_playlevel;
			rank = i_rank;
			total = l_total;
		}
		Data(const Data& d){
			bpm = d.bpm;
			playlevel = d.playlevel;
			rank = d.rank;
			total = d.total;
		}
		~Data(){}
		bool operator <(const Data& d) {
			return playlevel < d.playlevel;
		}
		bool operator >(const Data& d) {
			return playlevel > d.playlevel;
		}
	};
	vector<Data> tmp;
	for (int i= 0; i < num; i++) {
		tmp.emplace_back(bpm[i], playlevel[i], rank[i], total[i]);
	}
	std::sort(tmp.begin(), tmp.end());
	for (int i= 0; i < num; i++) {
		bpm[i] = tmp[i].bpm;
		playlevel[i] = tmp[i].playlevel;
		rank[i] = tmp[i].rank;
		total[i] = tmp[i].total;
	}
	tmp.clear();
	tmp.shrink_to_fit();
}
int BMS_Manager::load_music(SDL_Renderer *render){
	string file = header[sel].bmsfile[lv];
	ifstream ifs(file);
	string line;
	int bar_no,channel;
	int curr_bar_count;
	vector<string> data;
	array<long,6> ln_start, start_bar_no;
	if (ifs.fail()) {
		error = -1;
		goto ending;
	}

	bar_count = vector<long>(header[sel].bar_num[lv]+3, -1);
	curr_bar_count = stoi(header[sel].bpm[lv]) * BAR_STANDARD / 130;
	music[2].emplace_back(header[sel].bpm[lv],0, 0l);
	bar_count[0]=0l;
	//set initial data

	while (getline(ifs, line)) {
		line = regex_replace(line, regex("[ \t]+"), "");
		if (!regex_search(line, regex("^#[0-9]{3}02:[0-9].?[0-9]+$")))continue;
		bar_count[stoi(line.substr(1, 3))] = long(stof(line.substr(7))*float(BAR_STANDARD));
	}
	for (int i = 1; i < bar_count.size(); i++) {
		if (bar_count[i] > 0)curr_bar_count = bar_count[i];
		bar_count[i] =bar_count[i-1]+curr_bar_count;
	}
	//get bar frame

	ifs.clear();
	ifs.seekg(0, std::ios::beg);

	while(getline(ifs,line)){
		line = regex_replace(line,regex("[ \t]+$"),"");
		if (regex_search(line, regex("^#BMP[0-9]{2}"))) {
			bmp[line.substr(4,2)].init(regex_replace(line, regex("#BMP[0-9]{2}[ \t]+"), ""));
		}
		if (regex_search(line, regex("^#WAV[0-9]{2}"))) {
			wav[line.substr(4, 2)].init(header[sel].dir+regex_replace(line, regex("#WAV[0-9]{2}[ \t]+"), ""));
			if (wav[line.substr(4, 2)] == nullptr) {
				SDL_Log(Mix_GetError());
			}
		}
		if (regex_search(line, regex("^#BPM[0-9]{2}"))) {
			bpm[line.substr(4, 2)] = stoi(regex_replace(line, regex("#BPM[0-9]{2}[ \t]+"), ""));
		}

		line = regex_replace(line, regex("[ \t]+"), "");
		if (!regex_search(line,regex("^#[0-9]{5}:[0-9A-F]+$")))continue;

		bar_no = stoi(line.substr(1, 3));
		channel = stoi(line.substr(4, 2));

		if ((51 > channel && channel > 18)||channel==10||channel==9||channel==2)continue;
		if (channel <= 8)channel--;
		else if(channel<18)channel -= 3;
		else channel -= 36;
		//channel -> array key

		string tmp_data = line.substr(7);
		data.clear();
		if (channel == 2)data.push_back(to_string(stoi(tmp_data,NULL,16)));
		else {
			while (tmp_data.length() > 2) {
				data.push_back(tmp_data.substr(0, 2));
				tmp_data = tmp_data.substr(2);
			}
			data.push_back(tmp_data);
			tmp_data = "";
			//split data into unit (2 digits)
		}

		for (unsigned i = 0; i < data.size(); i++) {
			if (data[i] == "00")continue;
			if (channel < 15) {
				music[channel].emplace_back(data[i], bar_no, (long)(bar_count[(bar_no == 0) ? 0 : (bar_no - 1)] + i * (bar_count[bar_no] - bar_count[(bar_no == 0) ? 0 : (bar_no - 1)]) / data.size()));
				if (channel!=0&&channel!=2&&channel!=14)num_note++;
			}
			//for normal notes

			else {
				if (ln_start[channel - 15] > 0) {//for end note
					music[channel].emplace_back(data[i], start_bar_no[channel - 15], ln_start[channel - 15] , (long)(bar_count[bar_no-1] + i * (bar_count[bar_no] - bar_count[bar_no-1]) / data.size()));
					ln_start[channel - 15] = -1;
				} else {//for start note
					start_bar_no[channel - 15] = bar_no;
					ln_start[channel - 15] = (long)(bar_count[bar_no - 1] + i * (bar_count[bar_no] - bar_count[bar_no - 1]) / data.size());
					num_note++;
				}
			}
			//for long notes
		}
		//put data into array
	}
	//get notes data

	ifs.close();

	for (int i= 0; i < NUM_CHANNEL; i++) {
		sort(music[i].begin(), music[i].end());
	}
	//sort notes
	set_time_bpm_change();

	ending:
	set_music_loaded(true);//tell thread ending
	return 0;
}
void BMS_Manager::reset() {
	for (int i= 0; i < NUM_CHANNEL;i++) {
		music[i].clear();
	}
	for (auto itr = wav.begin(), end = wav.end(); itr != end; ++itr) {
		Mix_FreeMusic(itr->second);
	}
	wav.clear();
	for (auto itr = bmp.begin(), end = bmp.end(); itr != end; ++itr) {
		SDL_DestroyTexture(itr->second);
	}
	bmp.clear();
	bpm.clear();
	bar_count.clear();
	fill_n(next_note, NUM_CHANNEL, 0);
	num_note = 0;
	sel = 0;
	lv = 0;
	speed = 1.0f;
	time_draw_start = 1000l;
	error = 0;
}
void BMS_Manager::get_notes(vector<BMS_Manager::ENote> en[],long time) {
	for (int i= 0; i < 14; i++) {
		if(i<8)en[i].clear();
		if (music[(i==0)?0:i+7].size() <= next_note[(i==0)?0:i+7])continue;
		for (int j = next_note[(i==0)?0:i+7];music[(i==0)?0:i+7][j].get_count()<time_to_count(time + time_draw_start/*-((i==0)?120l:0)*/); j++) {
			en[(i<8)?i:i-7].emplace_back(music[(i==0)?0:i + 7][j], j);
			if (j >= music[(i==0)?0:i+7].size()-1)break;
		}
	}
	for (int i= 0; i < 8; i++) {
		if (en[i].size() == 0)continue;
		sort(en[i].begin(), en[i].end());
	}
}
void BMS_Manager::set_time_bpm_change() {
	time_bpm_change.clear();
	time_bpm_change.reserve(music[2].size());
	time_bpm_change.push_back(0l);
	long time = 0l;
	for (int i=1;i<music[2].size();i++){
		time = time_bpm_change[i - 1];
		time += (long)((music[2][i].get_count() - music[2][i - 1].get_count())/BAR_STANDARD*4/float(stol(music[2][i].get_data()))*60000);//ms
		time_bpm_change.push_back(time);
	}
	return;
}
long BMS_Manager::time_to_count(long time) {
	int i;
	//time += 50;//for projector
	for (i= 0;i<time_bpm_change.size()-1; i++) {
		if (time_bpm_change[i+1] > time)break;
	}
	return music[2][i].get_count() + long(double(stol(music[2][i].get_data()))/60.0*double(time - time_bpm_change[i])/1000.0*double(BAR_STANDARD)/4.0);
}

void Letter_Scroll::init(SDL_Renderer *a_render, TTF_Font *a_font, SDL_Color a_color, const SDL_Rect *a_dst, string a_str, float f_vel) {
	render = a_render;
	font = a_font;
	color = a_color;
	org_dst=dst = *a_dst;
	str = a_str;
	SDL_Surface *sur;
	sur = TTF_RenderUTF8_Blended(font, str.c_str(), color);
	tex = SDL_CreateTextureFromSurface(render, sur);
	SDL_FreeSurface(sur);
	SDL_QueryTexture(tex, NULL, NULL, &tex_w, &tex_h);
	src.h = tex_h;
	src.w = min(tex_w, tex_h*dst.w / dst.h);
	scroll = tex_w > tex_h*dst.w / dst.h;
	if (!scroll) {
		dst.w = tex_h * dst.w / dst.h;
		dst.x = org_dst.x + (org_dst.w - dst.w) / 2;
	}
	rate_v = f_vel;
	v = (int)(src.h*f_vel);
}
void Letter_Scroll::set_text(string a_str,SDL_Color *a_color) {
	SDL_Surface *sur;
	str = a_str;
	if(a_color!=NULL)color = *a_color;
	sur = TTF_RenderUTF8_Blended(font, str.c_str(), color);
	tex = SDL_CreateTextureFromSurface(render, sur);
	SDL_FreeSurface(sur);
	SDL_QueryTexture(tex, NULL, NULL, &tex_w, &tex_h);
	dst = org_dst;
	dst_w = dst.w;
	src.h = tex_h;
	src.w = min(tex_w, tex_h*dst.w / dst.h);
	scroll = tex_w > tex_h*dst.w / dst.h;
	if (!scroll) {
		dst.w = dst.h*tex_w / tex_h;
		dst.x = org_dst.x + (org_dst.w - dst.w) / 2;
	}
	v = (int)(src.h*rate_v);
	src.x = 0;
}
void Letter_Scroll::draw(){
	if (scroll) {
		if (src.x + src.w > tex_w) {
			dst_w -= float(v)*org_dst.w/src.w;
			dst.w = (int)dst_w;
			src.x += v;
			if(dst.w<0){
				src.x = 0;
				dst = org_dst;
				dst_w = dst.w;
			}
		}
		else src.x += v;
	}
	SDL_RenderCopy(render, tex, &src, &dst);
}
void Letter_Scroll::close(){
	SDL_DestroyTexture(tex);
}

void Text_Draw::set_left(string a_text, int x, int y, unsigned int h,const SDL_Color col) {
	SDL_Surface *sur;
	text = a_text;
	sur = TTF_RenderUTF8_Blended(font, text.c_str(), col);
	tex = SDL_CreateTextureFromSurface(render,sur);
	SDL_FreeSurface(sur);
	int tex_w, tex_h;
	SDL_QueryTexture(tex, NULL, NULL, &tex_w, &tex_h);
	rect.x = x;
	rect.y = y;
	rect.h = h;
	rect.w = h * tex_w / tex_h;
}
void Text_Draw::set_center(string a_text, int x_cen, int y_cen, unsigned int h, const SDL_Color col) {
	SDL_Surface *sur;
	text = a_text;
	sur = TTF_RenderUTF8_Blended(font, text.c_str(), col);
	tex = SDL_CreateTextureFromSurface(render, sur);
	SDL_FreeSurface(sur);
	int tex_w, tex_h;
	SDL_QueryTexture(tex, NULL, NULL, &tex_w, &tex_h);
	rect.h = h;
	rect.w = h * tex_w / tex_h;
	rect.x = x_cen - rect.w / 2;
	rect.y = y_cen - rect.h / 2;
}
void Text_Draw::draw() {
	SDL_RenderCopy(render, tex, NULL, &rect);
}
void Text_Draw::close() {
	SDL_DestroyTexture(tex);
}

Safe_Queue<pair<Lazy_Load*, function<void()>>> Lazy_Load::que(make_pair(nullptr, nullptr));
pair<Lazy_Load*, function<void()>> Lazy_Load::tmp;
void Lazy_Load::load_queue(SDL_Renderer *render) {
	while (!que.empty()) {
		tmp = que.pop();
		if (tmp.first->is_free()) continue;
		tmp.first->load(render);
		if (tmp.second) {
			tmp.second();
		}
	}
}
void Lazy_Load::push_queue(Lazy_Load* lt, function<void()> callback) {
	que.push(make_pair(lt, callback));
}

void Lazy_Texture::init(string path, function<void()> callback) {
	free();
	surface = IMG_Load(path.c_str());
	push_queue(this, callback);
	return;
}
void Lazy_Texture::init(string path, bool flg_queue) {
	free();
	surface = IMG_Load(path.c_str());
	if (flg_queue) push_queue(this);
	return;
}
void Lazy_Texture::init(const string path, SDL_Renderer *render) {
	free();
	surface = IMG_Load(path.c_str());
	load(render);
	return;
}
void Lazy_Texture::load(SDL_Renderer *render) {
	if (tex != NULL) SDL_DestroyTexture(tex);
	tex = SDL_CreateTextureFromSurface(render, surface);
	if (tex != NULL) SDL_FreeSurface(surface);
	return;
}
void Lazy_Texture::set_surface(SDL_Surface *arg_surface) {
	if (surface != NULL) SDL_FreeSurface(surface);
	surface = arg_surface;
	return;
}
void Lazy_Texture::set_texture(SDL_Texture *arg_tex) {
	if (tex != NULL) SDL_DestroyTexture(tex);
	tex = arg_tex;
	return;
}
void Lazy_Texture::free() {
	SDL_FreeSurface(surface);
	surface = nullptr;
	SDL_DestroyTexture(tex);
	tex = nullptr;
}

void Lazy_Music::init(string path, function<void()> callback) {
	free();
	this->path = path;
	push_queue(this, callback);
	return;
}
void Lazy_Music::init(string path, bool flg_queue) {
	free();
	this->path = path;
	if (flg_queue) push_queue(this);
	return;
}
void Lazy_Music::load(SDL_Renderer *render) {
	if (music != NULL)Mix_FreeMusic(music);
	music = Mix_LoadMUS(path.c_str());
	return;
}
void Lazy_Music::free() {
	Mix_FreeMusic(music);
	music = nullptr;
	path = "";
	return;
}

void Lazy_Chunk::init(string path, function<void()> callback) {
	free();
	this->path = path;
	push_queue(this, callback);
	return;
}
void Lazy_Chunk::init(string path, bool flg_queue) {
	free();
	this->path = path;
	if (flg_queue) push_queue(this);
	return;
}
void Lazy_Chunk::load(SDL_Renderer *render) {
	if (chunk != NULL)Mix_FreeChunk(chunk);
	chunk = Mix_LoadWAV(path.c_str());
	return;
}
void Lazy_Chunk::free() {
	Mix_FreeChunk(chunk);
	chunk = nullptr;
	path = "";
	return;
}

//algorithms
int min_l(initializer_list<int> args) {
	int min = INT_MAX;
	for (int x : args) {
		min = min(min, x);
	}
	return min;
}
int max_l(initializer_list<int> args) {
	int max = INT_MIN;
	for (int x : args) {
		max = max(max, x);
	}
	return max;
}
float min_l(initializer_list<float> args) {
	float min = FLT_MAX;
	for (float x : args) {
		min = min(min, x);
	}
	return min;
}
float max_l(initializer_list<float> args) {
	float max = FLT_MIN;
	for (float x : args) {
		max = max(max, x);
	}
	return max;
}
int SDL_RenderCopyAlpha(SDL_Renderer *render, SDL_Texture *texture, const SDL_Rect *srcrect, const SDL_Rect *dstrect, unsigned int alpha) {
	if (alpha > 255)return -1;
	if(SDL_SetTextureAlphaMod(texture, alpha)!=0)return -1;
	SDL_RenderCopy(render, texture, srcrect, dstrect);
	SDL_SetTextureAlphaMod(texture, 255);
	return 0;
}
