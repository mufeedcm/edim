#ifndef UI_H
#define UI_H

#include "editor.h"
#include <SDL3/SDL.h>

typedef enum{
  UI_ACTION_NONE,
  UI_ACTION_NEW,
  UI_ACTION_SAVE,
  UI_ACTION_CLOSE,
}UiAction;

void ui_init(SDL_Renderer *renderer,int width, int height);
void ui_handle_event(Editor *ed,SDL_Event *e);
UiAction ui_draw(Editor *ed, SDL_Renderer *renderer, int width, int height);

#endif
