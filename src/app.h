#ifndef APP_H
#define APP_H

#include "editor.h"
#include <SDL3/SDL.h>
#include <stdbool.h>

typedef struct {
  bool running;
  SDL_Window *window;
  SDL_Renderer *renderer;
  Editor ed;
  int width;
  int height;
}App;

void app_loop(App *app);
void app_run_web(App *app);
bool app_init(App *app);
void app_destroy(App *app);

#endif
