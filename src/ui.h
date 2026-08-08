#ifndef UI_H
#define UI_H

#include "editor.h"
#include <SDL3/SDL.h>

typedef enum{
  ACTION_NONE,
  ACTION_NEW,
  ACTION_OPEN,
  ACTION_SAVE,
  ACTION_SAVE_AS,
  ACTION_CLOSE,
  ACTION_PASTE,
}Action;

void ui_init(SDL_Renderer *renderer,int width, int height);
void ui_handle_event(Editor *ed,SDL_Event *e);
Action ui_draw(Editor *ed, SDL_Renderer *renderer, int width, int height, Uint64 last_input_time);
void ui_destroy(void);

#endif
