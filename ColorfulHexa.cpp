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

	if (SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO|SDL_INIT_NOPARACHUTE) < 0)
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
	Mix_Init(0);

	Timer timer;
	Gamemain gamemain(&timer,render,window);
	gamemain.init_sync();
	//thread th_init(&Gamemain::init,&gamemain);
	thread th_init([&gamemain] {
		gamemain.init();
		//Sleep(10 * 1000);
	});
	//th_init.join();
	//mainloop
	//if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL))SDL_Log("main priority fail");
	while (!quit) {  
		while (SDL_PollEvent(&e) != 0) {
			if (e.type == SDL_QUIT)quit = true;
		}
		Lazy_Load::load_queue(render);
		if(gamemain.loop()!=0)quit=true;
		timer.sleep();
	}

	if (th_init.joinable()) th_init.join();
	gamemain.close();
	timer.close();
	SDL_DestroyRenderer(render);
	SDL_DestroyWindow(window);
	Mix_CloseAudio();
	Mix_Quit();
	SDL_Quit();
	return 0;
}
