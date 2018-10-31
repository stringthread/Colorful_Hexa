#include "stdafx.h"
#include "ColorfulHexa.h"
#include <mmsystem.h>
#include <fstream>

//Timer
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
	zbar = _popen((curr_dir+"\\zbarcam").c_str(), "r");
	inited = true;
	return 0;
}
void QR_Reader::read_start(){
	timer.start();
	timer.set_frametime(2000);
	reading = true;
	auto th = thread([this] {
		while (1) {
			timer.sleep();
			if (!inited || read)continue;
			fgets(buff, sizeof(buff), zbar);
			tmp_str = string(buff);
			if (tmp_str==""||tmp_str.find("https://") == string::npos || tmp_str.compare(tmp_str.size() - 4, 4, userid) == 0)continue;
			userid = tmp_str.substr(tmp_str.size() - 55, 4);
			read = true;
			break;
		}
	});
	th.detach();
}
void QR_Reader::reset(){
	userid = "";
	return;
}
int QR_Reader::send(int score){
	curl = curl_easy_init();
	string chunk,url,post_data;
	//url = "https://tk67ennichi.official.jp/card/database.php";
	//post_data = "mode=set&attrac=musicgame&score=";
	url = "http://localhost/test/test.php";
	post_data = "test=qwerty";
	if (curl){
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, post_data.size());
		curl_easy_setopt(curl, CURLOPT_PROXY, "");
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
	}
	if (res != CURLE_OK) {
		return -1;
	}
	return 0;
}
void QR_Reader::check_hidden(bool hidden[2]) {
	curl = curl_easy_init();
	string chunk, url,post_data, ret;
	//url = "https://tk67ennichi.official.jp/enpass/database.php";
	//post_data = "mode=---&password=torisasamiyukke&id="+userid;
	url = "http://localhost/test/test.php";
	post_data = "test=qwerty";
	if (curl)
	{
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(curl, CURLOPT_POST, 1);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, post_data.size());
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ret);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback);
		curl_easy_setopt(curl, CURLOPT_PROXY, "");
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
	}
	if (res != CURLE_OK) {
		if (err_count < 5)check_hidden(hidden);
		else err_count = 0;
		return;
	}
	//hidden[0] = (ret == "2" || ret == "3");
	//hidden[1] = (ret == "1" || ret == "3");
	err_count = 0;
	return;
}
size_t QR_Reader::callback(char* ptr, size_t size, size_t nmemb, string* stream){
	*stream = string(ptr);
	return size*nmemb;
}
void QR_Reader::close(){
	_pclose(zbar);
}

//BMS_Manager
BMS_Manager::BMS_Manager(SDL_Renderer* render) {
	auto th = thread([this,render] {
		HANDLE h_dir, h_file;
		WIN32_FIND_DATA dir_data, file_data;
		char curr_dir[256] = {};
		string dir_path,root_dir;
		string wildcards[] = { "*.bms","*.bme","*.bml" };
		bool is_first;
		GetModuleFileName(NULL, &curr_dir[0], 256);
		root_dir = regex_replace(curr_dir, regex("\\\\[^\\\\]+$"), "");
		//char_traits<char>::copy(curr_dir, regex_replace(curr_dir, regex("\\\\[^\\\\]+$"), "").c_str(), 256);
		h_dir = FindFirstFile((root_dir + "\\song\\*").c_str(), &dir_data);
		if (h_dir == INVALID_HANDLE_VALUE) {
			if (GetLastError() != ERROR_NO_MORE_FILES)error = -1;
		}
		while (error == 0) {
			if (string(dir_data.cFileName).compare(".") != 0 && string(dir_data.cFileName).compare("..") != 0) {
				is_first = true;
				dir_path = root_dir + "\\song\\" + dir_data.cFileName + "\\";
				for (int i = 0; i < 3; i++) {
					error = 0;
					h_file = FindFirstFile((dir_path + wildcards[i]).c_str(), &file_data);
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
					}
				}
			}
			if (!FindNextFile(h_dir, &dir_data)) {
				if (GetLastError() != ERROR_NO_MORE_FILES)error = -1;
				FindClose(h_dir);
				header_loaded = true;
				break;
			}
		}
		if (header.size() == 0) {
			error = -1;
		}
	});
	th.detach();
	//load header
}
int BMS_Manager::Header::load(string dir,string file,bool is_first,SDL_Renderer* render) {
	this->dir = dir;
	file = dir + file;
	bmsfile.push_back(file);
	ifstream ifs(bmsfile.back());
	string line,data;
	bool has_total=false;

	bar_num.push_back(1);
	//set default

	if (ifs.fail())return -1;
	num++;
	while(getline(ifs,line)){
		if (line[0] != '#')continue;
		line=regex_replace(line, regex(" \t"), "");
		if (regex_search(line, regex("^#[0-9]{5}"))) {
			if (stoi(line.substr(1, 3)) > bar_num.back())bar_num.back() = stoi(line.substr(1, 3))+1;
		}
		has_total = false;
		for (int i= 0; i < 10; i++) {
			if (!is_first && (i == 0 || i == 1 || i == 2 || i == 4 || i == 7 || i == 9))continue;
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
				SDL_Surface* tmp = IMG_Load((dir + data).c_str());
				stagefile = SDL_CreateTextureFromSurface(render,tmp);
				SDL_FreeSurface(tmp);
				break;
			}
		}
	}
	ifs.close();
	if (bpm.size() < num)bpm.push_back("130");
	if (!has_total)total.push_back(300l);
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
			bmp[line.substr(4,2)]=SDL_CreateTextureFromSurface(render,IMG_Load(regex_replace(line, regex("#BMP[0-9]{2}[ \t]+"), "").c_str()));
		}
		if (regex_search(line, regex("^#WAV[0-9]{2}"))) {
			wav[line.substr(4, 2)] = Mix_LoadWAV((header[sel].dir+regex_replace(line, regex("#WAV[0-9]{2}[ \t]+"), "")).c_str());
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
		if (channel == 2)data.push_back(tmp_data);
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
				if (channel!=0&&channel!=14)num_note++;
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
	music_loaded = true;//tell thread ending
	return 0;
}
void BMS_Manager::reset() {
	for (int i= 0; i < NUM_CHANNEL;i++) {
		music[i].clear();
	}
	for (auto itr = wav.begin(), end = wav.end(); itr != end; ++itr) {
		Mix_FreeChunk(itr->second);
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
		time += (long)((music[2][i].get_count() - music[2][i - 1].get_count())/BAR_STANDARD*4/float(stol(music[2][i].get_data(),NULL,16))*60000);//ms
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
	return music[2][i].get_count() + long(double(stol(music[2][i].get_data(),NULL,16))/60.0*double(time - time_bpm_change[i])/1000.0*double(BAR_STANDARD)/4.0);
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
