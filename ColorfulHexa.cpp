#include "stdafx.h"
#include "ColorfulHexa.h"

int scr_w, scr_h;

int main(int argc, char* args[]){
	bool quit = false;
	SDL_Window* window = NULL;
	SDL_Renderer* render = NULL;
	SDL_Event e;
	HDC hdc = GetDC(GetDesktopWindow());
	scr_h = int(min(int(1.732f*GetDeviceCaps(hdc, HORZRES)), GetDeviceCaps(hdc, VERTRES))*0.9f);
	scr_w = int(1.732f*scr_h / 2);

	if (SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO) < 0)
	{
		OutputDebugString(_T("Init Error"));
		return -1;
	}
	else
	{
		window = SDL_CreateWindow("Colorful Hexa", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, scr_w, scr_h, SDL_WINDOW_SHOWN);
		if (window==NULL)
		{
			OutputDebugString(_T("Window Error"));
			SDL_Quit();
			return -1;
		}
		//SDL_SetWindowBordered(window, SDL_bool::SDL_FALSE);
		render = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
		if (render==NULL) {
			OutputDebugString(_T("Screen Surface Error"));
			SDL_DestroyWindow(window);
			SDL_Quit();
			return -1;
		}
	}
	//window initialization
	if (Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, AUDIO_S16, 2, 4096) == -1) {
		OutputDebugString(_T("Mixer initialization error"));
		Mix_Quit();
		SDL_Quit();
		return -1;
	}
	Mix_Volume(-1, 64);

	Timer timer;
	Gamemain gamemain = Gamemain(&timer,render,window);
	gamemain.init();
	//mainloop
	while (!quit) {  
		while (SDL_PollEvent(&e) != 0) {
			if (e.type == SDL_QUIT)quit = true;
		}
		if(gamemain.loop()!=0)quit=true;
		timer.sleep();
	}

	gamemain.close();
	timer.close();
	SDL_DestroyRenderer(render);
	SDL_DestroyWindow(window);
	Mix_CloseAudio();
	Mix_Quit();
	SDL_Quit();
	return 0;
}
