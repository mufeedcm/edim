#include "ui.h"
#include "editor.h"
#include <SDL3/SDL_render.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define CLAY_IMPLEMENTATION
#include "clay/clay.h"
#include "clay/clay_renderer_SDL3.c"

static Clay_SDL3RendererData renderData;
static TTF_Font *font;

static const Clay_Color WHITE      = (Clay_Color) {255,255,255,255};
static const Clay_Color PURE_BLACK = (Clay_Color) {0,0,0,255};
static const Clay_Color BLACK      = (Clay_Color) {30,30,30,255};
static const Clay_Color GRAY1      = (Clay_Color) {40,40,40,255};
static const Clay_Color GRAY2      = (Clay_Color) {50,50,50,255};
static const Clay_Color GRAY3      = (Clay_Color) {60,60,60,255};
static const Clay_Color GRAY4      = (Clay_Color) {70,70,70,255};
static const Clay_Color GRAY5      = (Clay_Color) {120,120,120,255};
static const Clay_Color RED        = (Clay_Color) {255,0,0,255};
static const Clay_Color GREEN      = (Clay_Color) {0,255,0,255};

static Clay_Dimensions measure_text(Clay_StringSlice text,Clay_TextElementConfig *config,void *userData){
  TTF_Font *f = userData;
  int w,h;
  TTF_SetFontSize(f,config->fontSize);
  TTF_GetStringSize(f,text.chars,text.length,&w,&h);
  return (Clay_Dimensions){(float)w,(float)h};
}

static bool mouse_clicked = false;
static bool file_menu_open = false;
static int prev_cur_pos = -1;


void ui_init(SDL_Renderer *renderer, int width, int height){
  font = TTF_OpenFont("assets/fonts/JetBrainsMono-Regular.ttf", 14);

  renderData.renderer = renderer;
  renderData.textEngine = TTF_CreateRendererTextEngine(renderer);

  renderData.fonts = malloc(sizeof(TTF_Font*));
  renderData.fonts[0] = font;

  uint64_t size = Clay_MinMemorySize();

  Clay_Arena arena = {
    .memory = malloc(size),
    .capacity = size
  };

  Clay_Initialize(arena, (Clay_Dimensions){width,height}, (Clay_ErrorHandler){0});
  Clay_SetMeasureTextFunction(measure_text,font);
};

void ui_handle_event(Editor *ed,SDL_Event *e){
  switch ( e->type) {
    case SDL_EVENT_MOUSE_MOTION:
      Clay_SetPointerState((Clay_Vector2){e->motion.x,e->motion.y}, e->motion.state & SDL_BUTTON_LMASK);
      break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
      mouse_clicked = true;
      Clay_SetPointerState((Clay_Vector2){e->button.x,e->button.y}, true);
      break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
      Clay_SetPointerState((Clay_Vector2){e->button.x,e->button.y}, false);
      break;
    case SDL_EVENT_MOUSE_WHEEL:
      Clay_UpdateScrollContainers(true,(Clay_Vector2){e->wheel.x,e->wheel.y}, 0.01f);
      editor_scroll_set(ed, editor_scroll(ed) - e->wheel.y *20);
      break;
  }
}

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

void draw_line_number(SDL_Renderer *renderer, int x, int y, int line){

  char num[16];
  snprintf(num, sizeof(num),"%d",line);
  SDL_Surface *line_surface = TTF_RenderText_Blended(font, num,strlen(num), (SDL_Color){GRAY5.r,GRAY5.g,GRAY5.b,GRAY5.a});
  SDL_Texture *line_texture = SDL_CreateTextureFromSurface(renderer, line_surface);
  SDL_FRect line_dst = {x,y,line_surface->w,line_surface->h};
  SDL_DestroySurface(line_surface);
  SDL_RenderTexture(renderer, line_texture, NULL, &line_dst);
  SDL_DestroyTexture(line_texture);

}

void draw_editor(SDL_Renderer *renderer,Editor *ed,int x_offset, int y_offset,int width, int height){
  if(ed->buffer_count!=0){
  int char_w = 10;
  int char_h = 18;

  int margin =10;
  int gutter_w = 30;

  int text_len= editor_length(ed);
  int cur_pos = editor_cursor(ed);

  int row_count = 1;
  int line_no =1;
  int col = 0;
  int max_cols = (width/char_w)-5;

  int current_scroll = editor_scroll(ed);
  int text_start_x = x_offset+gutter_w+margin;
  int text_start_y = (y_offset+margin)- current_scroll;

  int x = text_start_x; // x position
  int y = text_start_y; // y position

  int cur_x = text_start_x; //cursor x
  int cur_y = text_start_y; //cursor y

  SDL_SetRenderClipRect(renderer,&(SDL_Rect){x_offset,y_offset,width,height});
  
  draw_line_number(renderer, x_offset+5, y, line_no);

  for(int i=0;i<text_len;i++){
    char c = editor_get(ed, i);

    if(i==cur_pos){
      cur_x = x;
      cur_y = y;
    }
    if(c =='\n'||col>=max_cols){
      row_count++;
      y+=char_h;
      x = text_start_x;
      col =0;
      if(c=='\n'){
        line_no++;
        draw_line_number(renderer, x_offset+5, y, line_no);
      }
      if(c == '\n') continue;
    }
    char str[2] = {c,'\0'};
    SDL_Surface *surface = TTF_RenderText_Blended(font, str,strlen(str), (SDL_Color){WHITE.r,WHITE.g,WHITE.b,WHITE.a});
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FRect dst = {x,y,surface->w,surface->h};
    SDL_DestroySurface(surface);
    SDL_RenderTexture(renderer, texture, NULL, &dst);
    SDL_DestroyTexture(texture);
    x+=char_w;
    col++;
  }

  if(cur_pos==text_len){
    cur_x=x;
    cur_y=y;
  }

  int top_bound = y_offset + margin;
  int bottom_bound = y_offset +height - margin;
  if(cur_pos != prev_cur_pos){
    if(cur_y<top_bound){
      current_scroll -=(top_bound - cur_y);
    }
    if(cur_y +char_h >bottom_bound){
      current_scroll += (cur_y+char_h)-bottom_bound;
    }
    editor_scroll_set(ed, current_scroll);
    prev_cur_pos = cur_pos;
  }


  // scroll limit
  int content_h = row_count * char_h + margin * 2;
  int max_scroll = content_h - height;

  if(max_scroll<0) max_scroll = 0; 
  if(current_scroll > max_scroll){
    current_scroll = max_scroll;
    editor_scroll_set(ed, current_scroll);
  }
  

  bool showCursor = (SDL_GetTicks() / 300) % 2;
  if(showCursor){
    SDL_FRect cur = {cur_x,cur_y,2,char_h};
    SDL_SetRenderDrawColor(renderer, WHITE.r,WHITE.g,WHITE.b,WHITE.a);
    SDL_RenderFillRect(renderer, &cur);
  }
  SDL_SetRenderClipRect(renderer, NULL);
  }
}

UiAction ui_draw(Editor *ed,SDL_Renderer *renderer, int width , int height){
  UiAction action = UI_ACTION_NONE;
  Clay_SetLayoutDimensions((Clay_Dimensions){width,height});
  Clay_PointerData pointer = Clay_GetPointerState();

  Clay_BeginLayout();

  Clay_Sizing layoutExpand = {
          .width =  CLAY_SIZING_GROW(0),
          .height = CLAY_SIZING_GROW(0)
  };


  CLAY(CLAY_ID("container"),{
      .layout = {
        .layoutDirection = CLAY_TOP_TO_BOTTOM,
        .sizing = layoutExpand
        },
      })
  {
       CLAY(CLAY_ID("menubar"),{
           .layout = {
           .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = {
             .width = CLAY_SIZING_GROW(0),
             .height = CLAY_SIZING_FIT(),
             }
           },
           .backgroundColor = GRAY1,
       })
       {

         CLAY(CLAY_ID("LOGO"),{
             .layout = {
              .sizing = {
               .width = CLAY_SIZING_FIT(),
               .height = CLAY_SIZING_FIT(),
               },
               .padding = {10,10,5,5}
             },
             .backgroundColor = GRAY1,
         })
         {
           CLAY_TEXT(CLAY_STRING("EDIM"),{
               .fontId = 0,
               .fontSize = 14,
               .textColor = WHITE 
            });
         }

         CLAY(CLAY_ID("file_btn"),{
             .layout = {
              .sizing = {
               .width = CLAY_SIZING_FIT(),
               .height = CLAY_SIZING_FIT(),
               },
               .padding = {10,10,5,5}
             },
             .backgroundColor = GRAY4 
         })
         {
           CLAY_TEXT(CLAY_STRING("File"),{
               .fontId = 0,
               .fontSize = 14,
               .textColor = WHITE 
            });
           
           if(Clay_PointerOver(CLAY_ID("file_btn")) && mouse_clicked){
             file_menu_open = !file_menu_open;
           }


           if(file_menu_open){

             CLAY(CLAY_ID("file_list"),{
                 .floating = {
                   .attachTo = CLAY_ATTACH_TO_PARENT,
                   .attachPoints = {
                   .parent = CLAY_ATTACH_POINT_LEFT_BOTTOM,
                   },
                   .offset = {0,5},
                   
                 },
                 .layout = {
                   .layoutDirection = CLAY_TOP_TO_BOTTOM,
                   .sizing = {
                     .width = CLAY_SIZING_FIXED(120),
                     .height = CLAY_SIZING_FIT()
                   },
                   .padding = {10,10,10,10},
                   .childGap = 16,
                 },
                 .border = {
                   .color = GRAY5,
                   .width = {1,1,1,1},
                 },
                 .cornerRadius = {5,5,5,5},
                 .backgroundColor = GRAY3
               }){
              CLAY(CLAY_ID("new_btn"),{
                  .layout = {
                   .sizing = layoutExpand,
                   },
                  }){
               CLAY_TEXT(CLAY_STRING("New"),{
                   .fontId = 0,
                   .fontSize = 14,
                   .textColor = WHITE 
                   });
              }
              if(Clay_PointerOver(CLAY_ID("new_btn")) && mouse_clicked){
                action = UI_ACTION_NEW;
               file_menu_open = !file_menu_open;
              }
#ifndef __EMSCRIPTEN__
              CLAY(CLAY_ID("open_btn"),{
                  .layout = {
                   .sizing = layoutExpand,
                   },
                  }){
               CLAY_TEXT(CLAY_STRING("Open"),{
                   .fontId = 0,
                   .fontSize = 14,
                   .textColor = WHITE 
                   });
              }
              if(Clay_PointerOver(CLAY_ID("open_btn")) && mouse_clicked){
                SDL_ShowOpenFileDialog(open_file_callback, ed, NULL, NULL, 0, NULL, false);
               file_menu_open = !file_menu_open;
              }
              CLAY(CLAY_ID("save_btn"),{
                  .layout = {
                   .sizing = layoutExpand,
                   },
                  }){
               CLAY_TEXT(CLAY_STRING("Save"),{
                   .fontId = 0,
                   .fontSize = 14,
                   .textColor = WHITE 
                   });
              }
              if(Clay_PointerOver(CLAY_ID("save_btn")) && mouse_clicked){
                action = UI_ACTION_SAVE;
               file_menu_open = !file_menu_open;
              }
              CLAY(CLAY_ID("saveas_btn"),{
                  .layout = {
                   .sizing = layoutExpand,
                   },
                  }){
               CLAY_TEXT(CLAY_STRING("Save As.."),{
                   .fontId = 0,
                   .fontSize = 14,
                   .textColor = WHITE 
                   });
              }
              if(Clay_PointerOver(CLAY_ID("saveas_btn")) && mouse_clicked){
                SDL_ShowSaveFileDialog(save_file_callback, ed, NULL, NULL, 0, NULL);
               file_menu_open = !file_menu_open;
              }
#endif
              CLAY(CLAY_ID("close_btn"),{
                  .layout = {
                   .sizing = layoutExpand,
                   },
                  }){
               CLAY_TEXT(CLAY_STRING("Close"),{
                   .fontId = 0,
                   .fontSize = 14,
                   .textColor = WHITE 
                   });
              }
              if(Clay_PointerOver(CLAY_ID("close_btn")) && mouse_clicked){
                action = UI_ACTION_CLOSE;
               file_menu_open = !file_menu_open;
              }
             }
             if(!(Clay_PointerOver(CLAY_ID("file_btn")))&&mouse_clicked){
               file_menu_open = false;
             }
           }
         }
       }
       CLAY(CLAY_ID("tabs"),{
           .layout = {
           .layoutDirection = CLAY_LEFT_TO_RIGHT,
            .sizing = {
             .width = CLAY_SIZING_GROW(0),
             .height = CLAY_SIZING_FIT(),
             },
             .childGap = 10,
             .padding = {5,5,5,5},
           },
           .border = {
             .color = GRAY3,
             .width = {.top =1,.bottom=1},
           },
           .backgroundColor = GRAY1 
       })
       {
         for ( int i =0;i<editor_buf_count(ed);i++){
           Clay_Color bg = ( i == editor_curr_index(ed)) ? GRAY4 : GRAY2;
          const char *name = editor_buf_name(ed,i);
           CLAY(CLAY_IDI("tab",i),{
               .layout = {
               .layoutDirection = CLAY_LEFT_TO_RIGHT,
                 .sizing = {
                   .width = CLAY_SIZING_FIT(),
                   .height = CLAY_SIZING_FIT(),
                 },
               .padding = {10,10,5,5},
               .childGap = 10,
               },
               .cornerRadius = {5,5,5,5},
               .backgroundColor = bg,
               }){

           CLAY(CLAY_IDI("tab_name_container",i),{
               .layout = {
                 .sizing = {
                   .width = CLAY_SIZING_FIT(),
                   .height = CLAY_SIZING_FIT(),
                 },
               },
               }){
               Clay_String tab_name= {
                   .length = (int)strlen(name),
                   .chars = name,
               };
               CLAY_TEXT(tab_name,{
                   .fontId = 0,
                   .fontSize = 14,
                   .textColor = WHITE 
                   });
             }
             bool tab_name_clicked =
               Clay_PointerOver((CLAY_IDI("tab_name_container", i))) 
               && 
               mouse_clicked;

             if(tab_name_clicked){
               editor_switch(ed, i);
             }

           CLAY(CLAY_IDI("tab_close_container",i),{
               .layout = {
                 .sizing = {
                   .width = CLAY_SIZING_FIT(),
                   .height = CLAY_SIZING_FIT(),
                 },
               },
               }){
               CLAY_TEXT(CLAY_STRING("x"),{
                   .fontId = 0,
                   .fontSize = 14,
                   .textColor = RED 
                   });
             }
             bool tab_close_clicked =
               Clay_PointerOver((CLAY_IDI("tab_close_container", i))) 
               && 
               mouse_clicked;

             if(tab_close_clicked){
               editor_switch(ed, i);
               action = UI_ACTION_CLOSE;
               mouse_clicked = false;
               return action;
             }
           }
         }
       }
       CLAY(CLAY_ID("editor"),{
           .layout = {
            .sizing = layoutExpand,
           }
       })
       {
         if(editor_buf_count(ed) == 0){
       CLAY(CLAY_ID("greeter"),{
           .layout = {
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
            .sizing = layoutExpand,
            .childAlignment = {CLAY_ALIGN_X_CENTER,CLAY_ALIGN_Y_CENTER},
           }
       }){
         const char *logo = 
                   " ▄▄▄▄▄▄     █    ▀          \n"
                   " █       ▄▄▄█  ▄▄▄    ▄▄▄▄▄ \n"
                   " █▄▄▄▄▄ █▀ ▀█    █    █ █ █ \n"
                   " █      █   █    █    █ █ █ \n"
                   " █▄▄▄▄▄ ▀█▄██  ▄▄█▄▄  █ █ █ \n"
                   "                            \n";
              Clay_String logo_string = {
                 .chars = logo,
                 .length = (int)strlen(logo),
               };
               CLAY_TEXT(logo_string,{
                   .fontId = 0,
                   .fontSize = 14,
                   .textColor = WHITE 
                   });

               CLAY_TEXT(CLAY_STRING("A minimal editor written in C"),{
                   .fontId = 0,
                   .fontSize = 14,
                   .textColor = WHITE 
                   });
           }
         }
       }
  }
                            
  Clay_RenderCommandArray cmds = Clay_EndLayout(0.016f);
  Clay_ElementData editorData = Clay_GetElementData(CLAY_ID("editor"));
  if(editorData.found){
    Clay_BoundingBox editor_box = editorData.boundingBox;
    draw_editor(renderer, ed,
        (int)editor_box.x,
        (int)editor_box.y,
        (int)editor_box.width,
        (int)editor_box.height
    );
  }
  SDL_Clay_RenderClayCommands(&renderData, &cmds);
  mouse_clicked = false;
  return action;
}
