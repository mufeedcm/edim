/*
 * edim - a minimal editor written in C.
 * Copyright (C) 2025 mufeedcm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>. 
 */

/* includes */
#include "editor.h"
#include "ui.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

static const SDL_Color WHITE      = {255,255,255,255};
static const SDL_Color BLACK      = {30,30,30,255};

SDL_Window *window;
SDL_Renderer *renderer;

int main(int argc,char **argv){
  if(SDL_Init(SDL_INIT_VIDEO) !=0){ 
    fprintf(stderr,"SDL Init Warning: %s\n",SDL_GetError());
  }
  if(TTF_Init()!=0){ 
    fprintf(stderr,"TTF Init Warning: %s\n",SDL_GetError());
  }
  
  window = SDL_CreateWindow("EDIM", 800, 600, 0);
  if(!window){
    fprintf(stderr, "Window Creation Failed : %s\n",SDL_GetError());
    return 1;
  }
  renderer = SDL_CreateRenderer(window, NULL);
  if(!renderer){
    fprintf(stderr, "Renderer Creation Failed : %s\n",SDL_GetError());
    return 1;
  }
  ui_init(renderer, 800, 600);

  // SDL_SetWindowResizable(window, true);
  SDL_StartTextInput(window);

  int width, height;
  Editor ed;
  editor_init(&ed);
  editor_new(&ed);
  int running =1;
  while (running) {
    SDL_GetWindowSize(window, &width, &height);
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if(e.type == SDL_EVENT_QUIT){
        running =0;
      }
      ui_handle_event(&e);
      if(e.type == SDL_EVENT_KEY_DOWN){
        switch (e.key.key){
          case SDLK_RETURN:
            editor_insert(&ed, '\n');
            break;
          case SDLK_BACKSPACE:
            editor_delete(&ed);
            break;
          case SDLK_TAB:
            editor_insert_str(&ed, "    ");
            break;
          case SDLK_RIGHT:
            editor_move_right(&ed);
            break;
          case SDLK_LEFT:
            editor_move_left(&ed);
            break;
          case SDLK_UP:
            editor_move_up(&ed);
            break;
          case SDLK_DOWN:
            editor_move_down(&ed);
            break;
        }
      }
      if(e.type == SDL_EVENT_TEXT_INPUT){
        editor_insert_str(&ed, (char*)e.text.text);
      }
    }
    SDL_SetRenderDrawColor(renderer, BLACK.r, BLACK.g, BLACK.b, BLACK.a);
    SDL_RenderClear(renderer);
    ui_draw(&ed, renderer, width, height);
  //
    SDL_RenderPresent(renderer);
  }

  editor_save(&ed);
  editor_free(&ed);
  SDL_StopTextInput(window);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
