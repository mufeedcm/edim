#include "app.h"
#include "editor.h"
#include "ui.h"
#include <SDL3/SDL_keycode.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif 

static const SDL_Color BLACK = {30,30,30,255};

static void SDLCALL open_file_callback(void *userdata,const char * const *filelist,int filter){
  (void)filter;
  Editor *ed = userdata;
  if(!filelist || !filelist[0]){
    return; 
  }
  editor_open(ed, filelist[0]);
}

static void SDLCALL save_file_callback(void *userdata,const char * const *filelist,int filter){
  (void)filter;
  Editor *ed = userdata;
  if(!filelist || !filelist[0]){
    return; 
  }
  editor_save_as(ed, filelist[0]);
}

static void app_execute_action(App *app, Action action){
  switch (action){
    case ACTION_NEW: editor_new(&app->ed); break;
    case ACTION_OPEN:
#ifndef __EMSCRIPTEN__
    SDL_ShowOpenFileDialog(open_file_callback, &app->ed, NULL, NULL, 0, NULL, false);
#endif
    break;
    case ACTION_SAVE: editor_save(&app->ed); break;
    case ACTION_SAVE_AS:
#ifndef __EMSCRIPTEN__
    SDL_ShowSaveFileDialog(save_file_callback, &app->ed, NULL, NULL, 0, NULL);
#endif
    break;
    case ACTION_CLOSE: editor_close(&app->ed); break;
    case ACTION_NONE: break;
  }

}
static void app_handle_event(App *app, SDL_Event *e){
  if(e->type == SDL_EVENT_QUIT){
    app->running =false;
    return;
  }
  ui_handle_event(&app->ed,e);

  if(e->type == SDL_EVENT_KEY_DOWN){
    bool ctrl = (e->key.mod & SDL_KMOD_CTRL) || (e->key.mod & SDL_KMOD_GUI);
    bool shift = (e->key.mod & SDL_KMOD_SHIFT);
    if(ctrl){
      switch (e->key.key) {
        case SDLK_N:
        case SDLK_T:
          app_execute_action(app, ACTION_NEW); break;
        case SDLK_O: app_execute_action(app, ACTION_OPEN); break;
        case SDLK_S: app_execute_action(app, shift? ACTION_SAVE_AS : ACTION_SAVE); break;
        case SDLK_W: app_execute_action(app, ACTION_CLOSE); break;
      }
      return;
    }

    switch (e->key.key){
      case SDLK_RETURN: editor_insert(&app->ed, '\n'); break;
      case SDLK_BACKSPACE: editor_delete(&app->ed); break;
      case SDLK_TAB: editor_insert_str(&app->ed, "    "); break;
      case SDLK_RIGHT: editor_move_right(&app->ed); break;
      case SDLK_LEFT: editor_move_left(&app->ed); break;
      case SDLK_UP: editor_move_up(&app->ed); break;
      case SDLK_DOWN: editor_move_down(&app->ed); break;
    }
    return;
  }
  if(e->type == SDL_EVENT_TEXT_INPUT){
    editor_insert_str(&app->ed, (char*)e->text.text);
    app->last_input_time = SDL_GetTicks();
    return;
  }
}


static void app_render(App *app){
    SDL_SetRenderDrawColor(app->renderer, BLACK.r, BLACK.g, BLACK.b, BLACK.a);
    SDL_RenderClear(app->renderer);
    Action action = ui_draw(&app->ed, app->renderer, app->width, app->height, app->last_input_time);
    app_execute_action(app, action);
    SDL_RenderPresent(app->renderer);
}

void app_loop(App *app){
    SDL_GetWindowSize(app->window, &app->width, &app->height);
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      app_handle_event(app,&e);
    }
    app_render(app);
}

#ifdef __EMSCRIPTEN__
static App *emscripten_app = NULL;
void emscripten_main_loop(void){
  if(!emscripten_app){
    return;
  }
  if(!emscripten_app->running){
    emscripten_cancel_main_loop();
    return;
  }
  app_loop(emscripten_app);
}

void app_run_web(App *app){
  emscripten_app = app;
  emscripten_set_main_loop(emscripten_main_loop,0,1);
}
#endif

bool app_init(App *app){
  app->running = true;

  if(!SDL_Init(SDL_INIT_VIDEO)){ 
    fprintf(stderr,"SDL Init Warning: %s\n",SDL_GetError());
    return false;
  }
  if(!TTF_Init()){ 
    fprintf(stderr,"TTF Init Warning: %s\n",SDL_GetError());
    return false;
  }
  
  app->window = SDL_CreateWindow("EDIM", 800, 600, 0);
  if(!app->window){
    fprintf(stderr, "Window Creation Failed : %s\n",SDL_GetError());
    return false;
  }
  app->renderer = SDL_CreateRenderer(app->window, NULL);
  if(!app->renderer){
    fprintf(stderr, "Renderer Creation Failed : %s\n",SDL_GetError());
    return false;
  }
  ui_init(app->renderer, 800, 600);

  SDL_SetWindowResizable(app->window, true);
  SDL_StartTextInput(app->window);

  editor_init(&app->ed);

  return true;
}

void app_destroy(App *app){
  editor_save(&app->ed);
  editor_free(&app->ed);
  SDL_StopTextInput(app->window);
  if (app->renderer) {
  SDL_DestroyRenderer(app->renderer);
  }
  if (app->window) {
    SDL_DestroyWindow(app->window);
  }
  SDL_Quit();
}

