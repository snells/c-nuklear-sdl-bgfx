#pragma once

#include <SDL2/SDL.h>
#include <bgfx/c99/bgfx.h>
#include <stdint.h>


#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING

#include <nuklear.h>

enum SE_GUI { SE_GUI_NO_FONTS = 1,
	      SE_GUI_INIT_SHADERS,
	      SE_GUI_INIT_PROG,	      
	      SE_GUI_INIT_ERROR,	      
};

enum nk_theme { NK_THEME_BLACK, NK_THEME_WHITE, NK_THEME_RED, NK_THEME_BLUE, NK_THEME_DARK };

typedef struct nk_buffer nk_buffer;
typedef struct nk_allocator nk_allocator;
typedef struct nk_draw_null_texture nk_draw_null_texture;
typedef struct nk_font nk_font;
typedef struct nk_convert_config nk_convert_config;
typedef struct nk_context nk_context;
typedef struct nk_font_atlas nk_font_atlas;
typedef struct nk_draw_vertex_layout_element nk_draw_vertex_layout_element;
typedef struct nk_draw_command nk_draw_command;
typedef struct nk_color nk_color;
typedef struct nk_font_config nk_font_cfg;

typedef struct {
	float	pos[2];
	float	uv[2];
	uint8_t	col[4];
} nk_sdl_vertex;

typedef struct {
	char path[256];
	int size;
	nk_font_cfg cfg;
} nk_sdl_font;

typedef struct {
	nk_buffer                       cmds;
	nk_buffer                       vbuf;
	nk_buffer                       ibuf;
	nk_convert_config		cfg;
	nk_draw_null_texture            null;
	bgfx_shader_handle_t		vsh;
	bgfx_shader_handle_t		fsh;
	bgfx_program_handle_t		prog;
	bgfx_vertex_decl_t		dcl;
	bgfx_uniform_handle_t		uni;
	bgfx_texture_handle_t		tex;
	nk_draw_vertex_layout_element	layout[4];
} nk_sdl_device;

typedef struct nk_sdl {
	SDL_Window	*win;
	nk_sdl_device	dev;
	nk_context	ctx;
	nk_font_atlas	atlas;
	int		view;
	nk_font		**fonts;
	int		font_len;
} nk_sdl;


int nk_sdl_init(nk_sdl *d, SDL_Window *win, nk_sdl_font *f, int len, char *vs, char *fs);
void nk_sdl_del(nk_sdl *d);
int  nk_sdl_handle_event(nk_sdl *d, SDL_Event *evt);
void nk_sdl_render(nk_sdl *d);
void nk_set_style(nk_context *ctx, int theme);
void nk_sdl_font_default(nk_sdl_font *f, char *p, int s);
int nk_sdl_fonts_load(nk_sdl *d, nk_sdl_font *f, int len);
