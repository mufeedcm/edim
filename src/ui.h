#ifndef UI_H
#define UI_H

#include "editor.h"
#include <SDL3/SDL.h>

void ui_init(SDL_Renderer *renderer,int width, int height);
void ui_handle_event(SDL_Event *e);
void ui_draw(Editor *ed, SDL_Renderer *renderer, int width, int height);

#endif
