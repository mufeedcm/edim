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
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

static const SDL_Color WHITE      = {255,255,255,255};
static const SDL_Color BLACK      = {30,30,30,255};

typedef struct {
  TTF_Font *small;
  TTF_Font *normal;
  TTF_Font *large;

}Fonts;

Fonts fonts;

SDL_Window *window;
SDL_Renderer *renderer;

void drawText(SDL_Renderer *renderer,TTF_Font *font, const char *text,int x, int y, SDL_Color color){
  SDL_Surface *surface = TTF_RenderText_Blended(font, text,strlen(text), color);
  SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FRect dst = {x,y,surface->w,surface->h};
  SDL_DestroySurface(surface);
  SDL_RenderTexture(renderer, texture, NULL, &dst);
  SDL_DestroyTexture(texture);
}

int main(int argc,char **argv){
  if(SDL_Init(SDL_INIT_VIDEO) !=0){ 
    fprintf(stderr,"SDL Init Warning: %s\n",SDL_GetError());
  }
  if(TTF_Init()!=0){ 
    fprintf(stderr,"TTF Init Warning: %s\n",SDL_GetError());
  }
  
  const char *fontPath = "assets/fonts/JetBrainsMono-Regular.ttf";
  fonts.small = TTF_OpenFont(fontPath, 12);
  fonts.normal= TTF_OpenFont(fontPath, 14);
  fonts.large = TTF_OpenFont(fontPath, 18);

  if(!fonts.small|| !fonts.normal || !fonts.large){
    fprintf(stderr,"Font Failed To Load : %s\n",SDL_GetError());
    return 1;
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

  SDL_SetWindowResizable(window, true);
  SDL_StartTextInput(window);

  int width, height;
  int char_w = 10,char_h = 18;
  int margin=10;

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

    int len= editor_length(&ed);
    int col=0;
    int max_cols = (width/char_w)-2;
    int x = margin,y=margin;
    int cx=margin,cy=margin;
    int cursor = editor_cursor(&ed);

    for(int i=0;i<len;i++){
      char c = editor_get(&ed, i);

      if(i==cursor){
        cx=x;
        cy=y;
      }
      if(c =='\n'||col>=max_cols){
        y+=char_h;
        x = margin;
        col =0;
        if(c == '\n') continue;
      }
      char str[2] = {c,'\0'};
      drawText(renderer, fonts.normal,str,x,y, WHITE);
      x+=char_w;
      col++;
    }
    if(cursor==len){
      cx=x;
      cy=y;
    }
    bool showCursor = (SDL_GetTicks() / 300) % 2;
    if(showCursor){
      SDL_FRect cur = {cx,cy,2,char_h};
      SDL_SetRenderDrawColor(renderer, WHITE.r, WHITE.g, WHITE.b, WHITE.a);
      SDL_RenderFillRect(renderer, &cur);
    }
    SDL_RenderPresent(renderer);
  }

  editor_save(&ed);
  editor_free(&ed);

  TTF_CloseFont(fonts.small);
  TTF_CloseFont(fonts.normal);
  TTF_CloseFont(fonts.large);
  TTF_Quit();
  SDL_StopTextInput(window);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
