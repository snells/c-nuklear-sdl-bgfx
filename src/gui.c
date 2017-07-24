#define NK_IMPLEMENTATION
#include <seng/window/gui.h>
#include <stdint.h>


const bgfx_memory_t *se_window_file_load(char *path)
{
	if(!path)
		return 0;
	SDL_RWops *rw = SDL_RWFromFile(path, "r");
	if (!rw) {
		return 0;
	}
	size_t l = SDL_RWsize(rw);
	if(l <= 0) {
		return 0;
	}
	char *buf = malloc(l + 1);
	char *tmp = buf;
	size_t tot = 0, reads = 1;
	while(tot < reads && reads) {
		reads = SDL_RWread(rw, tmp, 1, l - tot);
		tot += reads;
		tmp += reads;
	}
	SDL_RWclose(rw);
	if(reads != l) {
		free(buf);
		return 0;
	}
	buf[tot] = 0;
	const bgfx_memory_t *m = bgfx_alloc(l + 1);
	if(!m) {
		free(buf);
		return m;
	}
	memcpy(m->data, buf, l + 1);
	free(buf);
	return m;	
}



void nk_sdl_font_default(nk_sdl_font *f, char *p, int s)
{
	f->path[0] = 0;
	strncat(f->path, p, 256);
	f->size = s;
	f->cfg = nk_font_config(s);
}


int get_transient_buf(uint32_t vc, bgfx_vertex_decl_t *dcl, uint32_t ic)
{
	return bgfx_get_avail_transient_vertex_buffer(vc, dcl) >= vc &&
		bgfx_get_avail_transient_index_buffer(ic) >= ic;
}

int nk_sdl_bgfx_init(nk_sdl_device *dev, char *vs, char *fs)
{
	if(!dev || !vs || !fs) {
		return SE_GUI_INIT_ERROR;
	}
	bgfx_vertex_decl_begin(&dev->dcl, BGFX_RENDERER_TYPE_NOOP);
	bgfx_vertex_decl_add(&dev->dcl, BGFX_ATTRIB_POSITION, 2, BGFX_ATTRIB_TYPE_FLOAT, 0, 0);
	bgfx_vertex_decl_add(&dev->dcl, BGFX_ATTRIB_TEXCOORD0, 2, BGFX_ATTRIB_TYPE_FLOAT, 0, 0);
	bgfx_vertex_decl_add(&dev->dcl, BGFX_ATTRIB_COLOR0, 4, BGFX_ATTRIB_TYPE_UINT8, 1, 0);
	bgfx_vertex_decl_end(&dev->dcl);
	const bgfx_memory_t *vsh = se_window_file_load(vs);
	if(!vsh) {
		return SE_GUI_INIT_SHADERS;
	}
	const bgfx_memory_t *fsh = se_window_file_load(fs);
	if(!fsh)
		return SE_GUI_INIT_SHADERS;
	dev->vsh = bgfx_create_shader(vsh);
	dev->fsh = bgfx_create_shader(fsh);
	dev->prog = (bgfx_create_program(dev->vsh, dev->fsh, 1));
	if(dev->prog.idx == UINT16_MAX)
		return SE_GUI_INIT_PROG;
	dev->uni = bgfx_create_uniform("s_texColor", BGFX_UNIFORM_TYPE_INT1, 1);
	nk_buffer_init_default(&dev->cmds);
	bgfx_vertex_decl_t *dcl = &dev->dcl;
	dev->layout[0].offset = dcl->offset[BGFX_ATTRIB_POSITION];
	dev->layout[0].format = NK_FORMAT_FLOAT;
	dev->layout[0].attribute = NK_VERTEX_POSITION;

	dev->layout[1].offset = dcl->offset[BGFX_ATTRIB_TEXCOORD0];
	dev->layout[1].format = NK_FORMAT_FLOAT;
	dev->layout[1].attribute = NK_VERTEX_TEXCOORD;

	dev->layout[2].offset = dcl->offset[BGFX_ATTRIB_COLOR0];
	dev->layout[2].format = NK_FORMAT_R8G8B8A8;
	dev->layout[2].attribute = NK_VERTEX_COLOR;

	dev->layout[3].offset = 0;
	dev->layout[3].format = NK_FORMAT_COUNT;
	dev->layout[3].attribute = NK_VERTEX_ATTRIBUTE_COUNT;
	return 0;
}



int nk_sdl_init_font(nk_sdl *d, nk_sdl_font *fonts, int len)
{
	if(len <= 0 || !fonts) {
		return SE_GUI_NO_FONTS;
	}
	nk_font_atlas *atlas = &d->atlas;
	nk_font_atlas_init_default(atlas);
	nk_font_atlas_begin(atlas);
	d->fonts = malloc(sizeof(nk_font**) * len);
	memset(d->fonts, 0, sizeof(nk_font**) * len);
	for(int n = 0; n < len; n++) {
		nk_sdl_font *sf = fonts + n;
		nk_font *f = nk_font_atlas_add_from_file(atlas, sf->path, sf->size, &sf->cfg);
		if(!f) {
			if(!n) {
				return SE_GUI_NO_FONTS;
			}
		}
		*(d->fonts + d->font_len) = f;
		d->font_len++;
	}
	int wi, h;
	const void *p = nk_font_atlas_bake(atlas, &wi, &h, NK_FONT_ATLAS_RGBA32);	
	long s = wi * h * 4;
	const bgfx_memory_t *m = bgfx_alloc(s);
	memcpy(m->data, p, s);
	nk_sdl_device *dev = &d->dev;
	dev->tex = bgfx_create_texture_2d(wi, h, 0, 1,
					  BGFX_TEXTURE_FORMAT_RGBA8,
					  0, m);
	if(dev->tex.idx == UINT16_MAX) {
		printf("texture failed\n");
	}
	nk_font_atlas_end(atlas, nk_handle_id(dev->tex.idx), &dev->null);
	nk_init_default(&d->ctx, &d->fonts[0]->handle);
	d->ctx.style.font = &d->fonts[0]->handle;
	d->atlas.default_font = d->fonts[0];
	nk_set_style(&d->ctx, NK_THEME_DARK);
	nk_style_set_font(&d->ctx, &d->fonts[0]->handle);
	return 0;
}

void nk_sdl_set_font(nk_sdl *d, int n)
{
	if(n <= 0 || n > d->font_len) {
		return;
	}
	nk_style_set_font(&d->ctx, &((d->fonts[n-1]->handle)));
}


int nk_sdl_init(nk_sdl *d, SDL_Window *w, nk_sdl_font *fonts, int len, char *vs, char *fs)
{
	memset(d, 0, sizeof(*d));
	d->win = w;
	d->view = 0;
	nk_sdl_device *dev = &d->dev;
	int tmp = nk_sdl_bgfx_init(dev, vs , fs);
	if(tmp) {
		return tmp;
	}
	nk_convert_config *cfg = &dev->cfg;
	cfg->null = dev->null;
	cfg->vertex_size = sizeof(nk_sdl_vertex);
	cfg->vertex_layout = dev->layout;
	cfg->vertex_alignment = _Alignof(nk_sdl_vertex);
	cfg->circle_segment_count = 22;
	cfg->curve_segment_count = 22;
	cfg->arc_segment_count = 22;
	cfg->global_alpha = 1.0f;
	cfg->shape_AA = NK_ANTI_ALIASING_ON;
	cfg->line_AA = NK_ANTI_ALIASING_ON;
	tmp = nk_sdl_init_font(d, fonts, len);
	if(tmp) {
		return tmp;
	}
	return 0;
}




NK_API void nk_sdl_render(nk_sdl *d)
{
	nk_sdl_device *dev = &d->dev;
	nk_buffer_init_default(&dev->vbuf);
	nk_buffer_init_default(&dev->ibuf);
	nk_convert(&d->ctx, &dev->cmds, &dev->vbuf, &dev->ibuf, &dev->cfg);
	void *vd = dev->vbuf.memory.ptr;
	void *id = dev->ibuf.memory.ptr;
	size_t vds = dev->vbuf.allocated;
	size_t ids = dev->ibuf.allocated;
	
	uint32_t os = 0;
	uint32_t vc = vds / dev->dcl.stride;
	int wid, hei;
	SDL_GetWindowSize(d->win, &wid, &hei);
	
	float ortho[4][4] = {
		{2.0f, 0.0f, 0.0f, 0.0f},
		{0.0f,-2.0f, 0.0f, 0.0f},
		{0.0f, 0.0f,-1.0f, 0.0f},
		{-1.0f,1.0f, 0.0f, 1.0f},
	};
	ortho[0][0] /= wid;
	ortho[1][1] /= hei;

	
	if(vc > 0) {
		uint32_t toti = ids / sizeof(uint16_t);
		bgfx_transient_vertex_buffer_t tvb;
		bgfx_transient_index_buffer_t tib;
		if(get_transient_buf(vc, &dev->dcl, toti)) {
			bgfx_alloc_transient_vertex_buffer(&tvb, vc, &dev->dcl);
			memcpy(tvb.data, vd, vds);
			bgfx_alloc_transient_index_buffer(&tib, toti);
			memcpy(tib.data, id, ids);
			const nk_draw_command *cmd = nk__draw_begin(&d->ctx, &dev->cmds);
			while(cmd) {
				bgfx_set_view_rect(0, 0, 0, wid, hei);
				bgfx_set_view_transform(0, 0, ortho);
				bgfx_set_state(BGFX_STATE_RGB_WRITE
					       | BGFX_STATE_ALPHA_WRITE
					       | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA,
									BGFX_STATE_BLEND_INV_SRC_ALPHA),
					       0);
				bgfx_texture_handle_t hdl; hdl.idx = (uint16_t)(cmd->texture.id);
				if(cmd->texture.id)					
					bgfx_set_texture(0, dev->uni, hdl, UINT32_MAX);
				else
					bgfx_set_texture(0, dev->uni, dev->tex, UINT32_MAX);

				bgfx_set_transient_vertex_buffer(0, &tvb, 0, vc);
				bgfx_set_transient_index_buffer(&tib, os, cmd->elem_count);
				bgfx_submit(d->view, dev->prog, 0, 0);
				os += cmd->elem_count;
				cmd = nk__draw_next(cmd, &dev->cmds, &d->ctx);
			}
		}
	}
	nk_clear(&d->ctx);
}

void nk_sdl_del(nk_sdl *d)
{
	bgfx_destroy_texture(d->dev.tex);
	bgfx_destroy_uniform(d->dev.uni);
	bgfx_destroy_program(d->dev.prog);
	free(d->fonts);
}



NK_API int
nk_sdl_handle_event(nk_sdl *d, SDL_Event *evt)
{
	struct nk_context *ctx = &d->ctx;
	if (evt->type == SDL_KEYUP || evt->type == SDL_KEYDOWN) {
		/* key events */
		int down = evt->type == SDL_KEYDOWN;
		const Uint8* state = SDL_GetKeyboardState(0);
		SDL_Keycode sym = evt->key.keysym.sym;
		if (sym == SDLK_RSHIFT || sym == SDLK_LSHIFT)
			nk_input_key(ctx, NK_KEY_SHIFT, down);
		else if (sym == SDLK_DELETE)
			nk_input_key(ctx, NK_KEY_DEL, down);
		else if (sym == SDLK_RETURN)
			nk_input_key(ctx, NK_KEY_ENTER, down);
		else if (sym == SDLK_TAB)
			nk_input_key(ctx, NK_KEY_TAB, down);
		else if (sym == SDLK_BACKSPACE)
			nk_input_key(ctx, NK_KEY_BACKSPACE, down);
		else if (sym == SDLK_HOME) {
			nk_input_key(ctx, NK_KEY_TEXT_START, down);
			nk_input_key(ctx, NK_KEY_SCROLL_START, down);
		} else if (sym == SDLK_END) {
			nk_input_key(ctx, NK_KEY_TEXT_END, down);
			nk_input_key(ctx, NK_KEY_SCROLL_END, down);
		} else if (sym == SDLK_PAGEDOWN) {
			nk_input_key(ctx, NK_KEY_SCROLL_DOWN, down);
		} else if (sym == SDLK_PAGEUP) {
			nk_input_key(ctx, NK_KEY_SCROLL_UP, down);
		} else if (sym == SDLK_z)
			nk_input_key(ctx, NK_KEY_TEXT_UNDO, down && state[SDL_SCANCODE_LCTRL]);
		else if (sym == SDLK_r)
			nk_input_key(ctx, NK_KEY_TEXT_REDO, down && state[SDL_SCANCODE_LCTRL]);
		else if (sym == SDLK_c)
			nk_input_key(ctx, NK_KEY_COPY, down && state[SDL_SCANCODE_LCTRL]);
		else if (sym == SDLK_v)
			nk_input_key(ctx, NK_KEY_PASTE, down && state[SDL_SCANCODE_LCTRL]);
		else if (sym == SDLK_x)
			nk_input_key(ctx, NK_KEY_CUT, down && state[SDL_SCANCODE_LCTRL]);
		else if (sym == SDLK_b)
			nk_input_key(ctx, NK_KEY_TEXT_LINE_START, down && state[SDL_SCANCODE_LCTRL]);
		else if (sym == SDLK_e)
			nk_input_key(ctx, NK_KEY_TEXT_LINE_END, down && state[SDL_SCANCODE_LCTRL]);
		else if (sym == SDLK_UP)
			nk_input_key(ctx, NK_KEY_UP, down);
		else if (sym == SDLK_DOWN)
			nk_input_key(ctx, NK_KEY_DOWN, down);
		else if (sym == SDLK_LEFT) {
			if (state[SDL_SCANCODE_LCTRL])
				nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, down);
			else nk_input_key(ctx, NK_KEY_LEFT, down);
		} else if (sym == SDLK_RIGHT) {
			if (state[SDL_SCANCODE_LCTRL])
				nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, down);
			else nk_input_key(ctx, NK_KEY_RIGHT, down);
		} else return 0;
		return 1;
	} else if (evt->type == SDL_MOUSEBUTTONDOWN || evt->type == SDL_MOUSEBUTTONUP) {
		/* mouse button */
		int down = evt->type == SDL_MOUSEBUTTONDOWN;
		const int x = evt->button.x, y = evt->button.y;
		if (evt->button.button == SDL_BUTTON_LEFT) {
			if (evt->button.clicks > 1)
				nk_input_button(ctx, NK_BUTTON_DOUBLE, x, y, down);
			else
				nk_input_button(ctx, NK_BUTTON_LEFT, x, y, down);
		} else if (evt->button.button == SDL_BUTTON_MIDDLE)
			nk_input_button(ctx, NK_BUTTON_MIDDLE, x, y, down);
		else if (evt->button.button == SDL_BUTTON_RIGHT)
			nk_input_button(ctx, NK_BUTTON_RIGHT, x, y, down);
		return 1;
	} else if (evt->type == SDL_MOUSEMOTION) {
		/* mouse motion */
		if (ctx->input.mouse.grabbed) {
			int x = (int)ctx->input.mouse.prev.x, y = (int)ctx->input.mouse.prev.y;
			nk_input_motion(ctx, x + evt->motion.xrel, y + evt->motion.yrel);
		} else nk_input_motion(ctx, evt->motion.x, evt->motion.y);
		return 1;
	} else if (evt->type == SDL_TEXTINPUT) {
		/* text input */
		nk_glyph glyph;
		memcpy(glyph, evt->text.text, NK_UTF_SIZE);
		nk_input_glyph(ctx, glyph);
		return 1;
	} else if (evt->type == SDL_MOUSEWHEEL) {
		/* mouse wheel */
		nk_input_scroll(ctx,nk_vec2((float)evt->wheel.x,(float)evt->wheel.y));
		return 1;
	}
	return 0;
}


void nk_set_style(nk_context *ctx, int theme) {
	nk_color table[ NK_COLOR_COUNT ];
    if ( theme == NK_THEME_WHITE ) {
        table[ NK_COLOR_TEXT ]                    = nk_rgba( 70, 70, 70, 255 );
        table[ NK_COLOR_WINDOW ]                  = nk_rgba( 175, 175, 175, 255 );
        table[ NK_COLOR_HEADER ]                  = nk_rgba( 175, 175, 175, 255 );
        table[ NK_COLOR_BORDER ]                  = nk_rgba( 0, 0, 0, 255 );
        table[ NK_COLOR_BUTTON ]                  = nk_rgba( 185, 185, 185, 255 );
        table[ NK_COLOR_BUTTON_HOVER ]            = nk_rgba( 170, 170, 170, 255 );
        table[ NK_COLOR_BUTTON_ACTIVE ]           = nk_rgba( 160, 160, 160, 255 );
        table[ NK_COLOR_TOGGLE ]                  = nk_rgba( 150, 150, 150, 255 );
        table[ NK_COLOR_TOGGLE_HOVER ]            = nk_rgba( 120, 120, 120, 255 );
        table[ NK_COLOR_TOGGLE_CURSOR ]           = nk_rgba( 175, 175, 175, 255 );
        table[ NK_COLOR_SELECT ]                  = nk_rgba( 190, 190, 190, 255 );
        table[ NK_COLOR_SELECT_ACTIVE ]           = nk_rgba( 175, 175, 175, 255 );
        table[ NK_COLOR_SLIDER ]                  = nk_rgba( 190, 190, 190, 255 );
        table[ NK_COLOR_SLIDER_CURSOR ]           = nk_rgba( 80, 80, 80, 255 );
        table[ NK_COLOR_SLIDER_CURSOR_HOVER ]     = nk_rgba( 70, 70, 70, 255 );
        table[ NK_COLOR_SLIDER_CURSOR_ACTIVE ]    = nk_rgba( 60, 60, 60, 255 );
        table[ NK_COLOR_PROPERTY ]                = nk_rgba( 175, 175, 175, 255 );
        table[ NK_COLOR_EDIT ]                    = nk_rgba( 150, 150, 150, 255 );
        table[ NK_COLOR_EDIT_CURSOR ]             = nk_rgba( 0, 0, 0, 255 );
        table[ NK_COLOR_COMBO ]                   = nk_rgba( 175, 175, 175, 255 );
        table[ NK_COLOR_CHART ]                   = nk_rgba( 160, 160, 160, 255 );
        table[ NK_COLOR_CHART_COLOR ]             = nk_rgba( 45, 45, 45, 255 );
        table[ NK_COLOR_CHART_COLOR_HIGHLIGHT ]   = nk_rgba( 255, 0, 0, 255 );
        table[ NK_COLOR_SCROLLBAR ]               = nk_rgba( 180, 180, 180, 255 );
        table[ NK_COLOR_SCROLLBAR_CURSOR ]        = nk_rgba( 140, 140, 140, 255 );
        table[ NK_COLOR_SCROLLBAR_CURSOR_HOVER ]  = nk_rgba( 150, 150, 150, 255 );
        table[ NK_COLOR_SCROLLBAR_CURSOR_ACTIVE ] = nk_rgba( 160, 160, 160, 255 );
        table[ NK_COLOR_TAB_HEADER ]              = nk_rgba( 180, 180, 180, 255 );
        nk_style_from_table( ctx, table );
    } else if ( theme == NK_THEME_RED ) {
        table[ NK_COLOR_TEXT ]                    = nk_rgba( 190, 190, 190, 255 );
        table[ NK_COLOR_WINDOW ]                  = nk_rgba( 30, 33, 40, 215 );
        table[ NK_COLOR_HEADER ]                  = nk_rgba( 181, 45, 69, 220 );
        table[ NK_COLOR_BORDER ]                  = nk_rgba( 51, 55, 67, 255 );
        table[ NK_COLOR_BUTTON ]                  = nk_rgba( 181, 45, 69, 255 );
        table[ NK_COLOR_BUTTON_HOVER ]            = nk_rgba( 190, 50, 70, 255 );
        table[ NK_COLOR_BUTTON_ACTIVE ]           = nk_rgba( 195, 55, 75, 255 );
        table[ NK_COLOR_TOGGLE ]                  = nk_rgba( 51, 55, 67, 255 );
        table[ NK_COLOR_TOGGLE_HOVER ]            = nk_rgba( 45, 60, 60, 255 );
        table[ NK_COLOR_TOGGLE_CURSOR ]           = nk_rgba( 181, 45, 69, 255 );
        table[ NK_COLOR_SELECT ]                  = nk_rgba( 51, 55, 67, 255 );
        table[ NK_COLOR_SELECT_ACTIVE ]           = nk_rgba( 181, 45, 69, 255 );
        table[ NK_COLOR_SLIDER ]                  = nk_rgba( 51, 55, 67, 255 );
        table[ NK_COLOR_SLIDER_CURSOR ]           = nk_rgba( 181, 45, 69, 255 );
        table[ NK_COLOR_SLIDER_CURSOR_HOVER ]     = nk_rgba( 186, 50, 74, 255 );
        table[ NK_COLOR_SLIDER_CURSOR_ACTIVE ]    = nk_rgba( 191, 55, 79, 255 );
        table[ NK_COLOR_PROPERTY ]                = nk_rgba( 51, 55, 67, 255 );
        table[ NK_COLOR_EDIT ]                    = nk_rgba( 51, 55, 67, 225 );
        table[ NK_COLOR_EDIT_CURSOR ]             = nk_rgba( 190, 190, 190, 255 );
        table[ NK_COLOR_COMBO ]                   = nk_rgba( 51, 55, 67, 255 );
        table[ NK_COLOR_CHART ]                   = nk_rgba( 51, 55, 67, 255 );
        table[ NK_COLOR_CHART_COLOR ]             = nk_rgba( 170, 40, 60, 255 );
        table[ NK_COLOR_CHART_COLOR_HIGHLIGHT ]   = nk_rgba( 255, 0, 0, 255 );
        table[ NK_COLOR_SCROLLBAR ]               = nk_rgba( 30, 33, 40, 255 );
        table[ NK_COLOR_SCROLLBAR_CURSOR ]        = nk_rgba( 64, 84, 95, 255 );
        table[ NK_COLOR_SCROLLBAR_CURSOR_HOVER ]  = nk_rgba( 70, 90, 100, 255 );
        table[ NK_COLOR_SCROLLBAR_CURSOR_ACTIVE ] = nk_rgba( 75, 95, 105, 255 );
        table[ NK_COLOR_TAB_HEADER ]              = nk_rgba( 181, 45, 69, 220 );
        nk_style_from_table( ctx, table );
    } else if ( theme == NK_THEME_BLUE ) {
        table[ NK_COLOR_TEXT ]                    = nk_rgba( 20, 20, 20, 255 );
        table[ NK_COLOR_WINDOW ]                  = nk_rgba( 202, 212, 214, 215 );
        table[ NK_COLOR_HEADER ]                  = nk_rgba( 137, 182, 224, 220 );
        table[ NK_COLOR_BORDER ]                  = nk_rgba( 140, 159, 173, 255 );
        table[ NK_COLOR_BUTTON ]                  = nk_rgba( 137, 182, 224, 255 );
        table[ NK_COLOR_BUTTON_HOVER ]            = nk_rgba( 142, 187, 229, 255 );
        table[ NK_COLOR_BUTTON_ACTIVE ]           = nk_rgba( 147, 192, 234, 255 );
        table[ NK_COLOR_TOGGLE ]                  = nk_rgba( 177, 210, 210, 255 );
        table[ NK_COLOR_TOGGLE_HOVER ]            = nk_rgba( 182, 215, 215, 255 );
        table[ NK_COLOR_TOGGLE_CURSOR ]           = nk_rgba( 137, 182, 224, 255 );
        table[ NK_COLOR_SELECT ]                  = nk_rgba( 177, 210, 210, 255 );
        table[ NK_COLOR_SELECT_ACTIVE ]           = nk_rgba( 137, 182, 224, 255 );
        table[ NK_COLOR_SLIDER ]                  = nk_rgba( 177, 210, 210, 255 );
        table[ NK_COLOR_SLIDER_CURSOR ]           = nk_rgba( 137, 182, 224, 245 );
        table[ NK_COLOR_SLIDER_CURSOR_HOVER ]     = nk_rgba( 142, 188, 229, 255 );
        table[ NK_COLOR_SLIDER_CURSOR_ACTIVE ]    = nk_rgba( 147, 193, 234, 255 );
        table[ NK_COLOR_PROPERTY ]                = nk_rgba( 210, 210, 210, 255 );
        table[ NK_COLOR_EDIT ]                    = nk_rgba( 210, 210, 210, 225 );
        table[ NK_COLOR_EDIT_CURSOR ]             = nk_rgba( 20, 20, 20, 255 );
        table[ NK_COLOR_COMBO ]                   = nk_rgba( 210, 210, 210, 255 );
        table[ NK_COLOR_CHART ]                   = nk_rgba( 210, 210, 210, 255 );
        table[ NK_COLOR_CHART_COLOR ]             = nk_rgba( 137, 182, 224, 255 );
        table[ NK_COLOR_CHART_COLOR_HIGHLIGHT ]   = nk_rgba( 255, 0, 0, 255 );
        table[ NK_COLOR_SCROLLBAR ]               = nk_rgba( 190, 200, 200, 255 );
        table[ NK_COLOR_SCROLLBAR_CURSOR ]        = nk_rgba( 64, 84, 95, 255 );
        table[ NK_COLOR_SCROLLBAR_CURSOR_HOVER ]  = nk_rgba( 70, 90, 100, 255 );
        table[ NK_COLOR_SCROLLBAR_CURSOR_ACTIVE ] = nk_rgba( 75, 95, 105, 255 );
        table[ NK_COLOR_TAB_HEADER ]              = nk_rgba( 156, 193, 220, 255 );
        nk_style_from_table( ctx, table );
    } else if ( theme == NK_THEME_DARK ) {
        table[ NK_COLOR_TEXT ]                    = nk_rgba( 210, 210, 210, 255 );
        table[ NK_COLOR_WINDOW ]                  = nk_rgba( 57, 67, 71, 215 );
        table[ NK_COLOR_HEADER ]                  = nk_rgba( 51, 51, 56, 220 );
        table[ NK_COLOR_BORDER ]                  = nk_rgba( 46, 46, 46, 255 );
        table[ NK_COLOR_BUTTON ]                  = nk_rgba( 48, 83, 111, 255 );
        table[ NK_COLOR_BUTTON_HOVER ]            = nk_rgba( 58, 93, 121, 255 );
        table[ NK_COLOR_BUTTON_ACTIVE ]           = nk_rgba( 63, 98, 126, 255 );
        table[ NK_COLOR_TOGGLE ]                  = nk_rgba( 50, 58, 61, 255 );
        table[ NK_COLOR_TOGGLE_HOVER ]            = nk_rgba( 45, 53, 56, 255 );
        table[ NK_COLOR_TOGGLE_CURSOR ]           = nk_rgba( 48, 83, 111, 255 );
        table[ NK_COLOR_SELECT ]                  = nk_rgba( 57, 67, 61, 255 );
        table[ NK_COLOR_SELECT_ACTIVE ]           = nk_rgba( 48, 83, 111, 255 );
        table[ NK_COLOR_SLIDER ]                  = nk_rgba( 50, 58, 61, 255 );
        table[ NK_COLOR_SLIDER_CURSOR ]           = nk_rgba( 48, 83, 111, 245 );
        table[ NK_COLOR_SLIDER_CURSOR_HOVER ]     = nk_rgba( 53, 88, 116, 255 );
        table[ NK_COLOR_SLIDER_CURSOR_ACTIVE ]    = nk_rgba( 58, 93, 121, 255 );
        table[ NK_COLOR_PROPERTY ]                = nk_rgba( 50, 58, 61, 255 );
        table[ NK_COLOR_EDIT ]                    = nk_rgba( 50, 58, 61, 225 );
        table[ NK_COLOR_EDIT_CURSOR ]             = nk_rgba( 210, 210, 210, 255 );
        table[ NK_COLOR_COMBO ]                   = nk_rgba( 50, 58, 61, 255 );
        table[ NK_COLOR_CHART ]                   = nk_rgba( 50, 58, 61, 255 );
        table[ NK_COLOR_CHART_COLOR ]             = nk_rgba( 48, 83, 111, 255 );
        table[ NK_COLOR_CHART_COLOR_HIGHLIGHT ]   = nk_rgba( 255, 0, 0, 255 );
        table[ NK_COLOR_SCROLLBAR ]               = nk_rgba( 50, 58, 61, 255 );
        table[ NK_COLOR_SCROLLBAR_CURSOR ]        = nk_rgba( 48, 83, 111, 255 );
        table[ NK_COLOR_SCROLLBAR_CURSOR_HOVER ]  = nk_rgba( 53, 88, 116, 255 );
        table[ NK_COLOR_SCROLLBAR_CURSOR_ACTIVE ] = nk_rgba( 58, 93, 121, 255 );
        table[ NK_COLOR_TAB_HEADER ]              = nk_rgba( 48, 83, 111, 255 );
        nk_style_from_table( ctx, table );
    } else {
        nk_style_default( ctx );
    }
}
