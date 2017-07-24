#pragma once
#include <seng/window/gui.h>
#include <SDL2/SDL.h>

typedef struct {
	nk_sdl		gui;
	SDL_Window	*w;
	int		view;
	uint32_t	wid;
} se_window;

enum SE_WINDOW { SE_WINDOW_ERR_GUI = 1,
		 SE_WINDOW_ERR_SDL,
		 SE_WINDOW_ERR
};

int se_window_init(se_window *w, nk_sdl_font *f, int len, char *vs, char *fs);
void se_window_quit(se_window *w);


