#include <bgfx/c99/bgfx.h>
#include <bgfx/c99/platform.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <seng/window.h>

void bgfx_sdl_set_window(SDL_Window* _window)
{
	SDL_SysWMinfo wmi;
	SDL_VERSION(&wmi.version);
	if (!SDL_GetWindowWMInfo(_window, &wmi) )
	{
		return;
	}

	bgfx_platform_data_t pd;
	pd.ndt          = wmi.info.x11.display;
	pd.nwh          = (void*)(uintptr_t)wmi.info.x11.window;
	pd.context      = NULL;
	pd.backBuffer   = NULL;
	pd.backBufferDS = NULL;
	bgfx_set_platform_data(&pd);

	return;
}
int se_window_init(se_window *w, nk_sdl_font *f, int len, char *vs, char *fs)
{
	if(SDL_Init(SDL_INIT_VIDEO)) {
		return SE_WINDOW_ERR_SDL;
	}
	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);	
	w->w = SDL_CreateWindow("...",
				SDL_WINDOWPOS_UNDEFINED,
				SDL_WINDOWPOS_UNDEFINED,				      
				640, 480,
				SDL_WINDOW_OPENGL);
	if(!w->w)
		return SE_WINDOW_ERR_SDL;
	int width  = 640;
	int height = 480;
	bgfx_sdl_set_window(w->w);
	bgfx_init(BGFX_RENDERER_TYPE_COUNT,
		  BGFX_PCI_ID_NONE , 0 , NULL , NULL);	
	bgfx_reset(width, height, BGFX_RESET_VSYNC);
	bgfx_set_debug(BGFX_DEBUG_TEXT);
	bgfx_set_view_clear(0, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
	if(nk_sdl_init(&w->gui, w->w, f, len, vs, fs)) {
		SDL_DestroyWindow(w->w);
		bgfx_shutdown();
		SDL_Quit();
		return SE_WINDOW_ERR_GUI;
	}
	return 0;
}


void se_window_quit(se_window *w)
{
	nk_sdl_del(&w->gui);
	bgfx_shutdown();
	SDL_DestroyWindow(w->w);
	SDL_Quit();
}
