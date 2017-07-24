#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <seng/window.h>
#include <bgfx/c99/bgfx.h>
#include <bgfx/c99/platform.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>


int main()
{
	se_window w;
	nk_sdl_font f;
	nk_sdl_font_default(&f, "droid.ttf", 14);
	if(se_window_init(&w, &f, 1, "vs", "fs")) {
		return 1;		
	}	
	SDL_Event e;
	int end = 0;
	while(!end) {
		nk_input_begin(&w.gui.ctx);
		while(SDL_PollEvent(&e)) {
			if(e.type == SDL_QUIT)
				end = 1;
			nk_sdl_handle_event(&w.gui, &e);
		}		
		nk_input_end(&w.gui.ctx);
		if(end)
			break;
		
		bgfx_touch(0);
		bgfx_dbg_text_clear(0, false);
		    struct nk_rect r;
		    nk_sdl *gui = &w.gui;
		    nk_context *ctx = &gui->ctx;
		    r.x = 50; r.y = 50; r.w = 500 ;r.h = 300;
		    if(nk_begin(ctx, "TEST", r,
				NK_WINDOW_BORDER |
				NK_WINDOW_MOVABLE |
				NK_WINDOW_TITLE
			       )) {
	
			    enum {EASY, HARD};
			    static int op = EASY;
			    static int property = 20;
			    nk_layout_row_static(ctx, 30, 80, 1);
			    if (nk_button_label(ctx, "button"))
				    fprintf(stdout, "button pressed\n");
			    nk_layout_row_dynamic(ctx, 30, 2);
			    if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
			    if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;
			    nk_layout_row_dynamic(ctx, 25, 1);
		    }
		    nk_end(ctx);
		    nk_sdl_render(gui);
		    bgfx_frame(0);
	}
	se_window_quit(&w);
	return 0;
}


