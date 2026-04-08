#include "ui.h"
#include "editor.h"
#include <SDL3_ttf/SDL_ttf.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

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

static int editor_scroll_y=0;


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

void ui_handle_event(SDL_Event *e){
  switch ( e->type) {
    case SDL_EVENT_MOUSE_MOTION:
      Clay_SetPointerState((Clay_Vector2){e->motion.x,e->motion.y}, e->motion.state & SDL_BUTTON_LMASK);
      break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
      Clay_SetPointerState((Clay_Vector2){e->button.x,e->button.y}, true);
      break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
      Clay_SetPointerState((Clay_Vector2){e->button.x,e->button.y}, false);
      break;
    case SDL_EVENT_MOUSE_WHEEL:
      Clay_UpdateScrollContainers(true,(Clay_Vector2){e->wheel.x,e->wheel.y}, 0.01f);
      editor_scroll_y -= e->wheel.y *20;
      if(editor_scroll_y <0) editor_scroll_y =0;
      break;
  }
}

void draw_editor(SDL_Renderer *renderer,Editor *ed,int x_offset, int y_offset,int width, int height){
  int char_w = 10,char_h = 18;
  int margin =10;

  int len= editor_length(ed);
  int col=0;
  int max_cols = (width/char_w)-2;
  int x = x_offset+margin,y=(y_offset+margin)-editor_scroll_y;
  int cx=x_offset+margin,cy=(y_offset+margin)-editor_scroll_y;
  int cursor = editor_cursor(ed);
  SDL_SetRenderClipRect(renderer,&(SDL_Rect){x_offset,y_offset,width,height});

    for(int i=0;i<len;i++){
      char c = editor_get(ed, i);

      if(i==cursor){
        cx=x;
        cy=y;
      }
      if(c =='\n'||col>=max_cols){
        y+=char_h;
        x = x_offset+margin;
        col =0;
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
    if(cursor==len){
      cx=x;
      cy=y;
    }
    bool showCursor = (SDL_GetTicks() / 300) % 2;
    if(showCursor){
      SDL_FRect cur = {cx,cy,2,char_h};
      SDL_SetRenderDrawColor(renderer, WHITE.r,WHITE.g,WHITE.b,WHITE.a);
      SDL_RenderFillRect(renderer, &cur);
    }
    SDL_SetRenderClipRect(renderer, NULL);
}

void ui_draw(Editor *ed,SDL_Renderer *renderer, int width , int height){
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
           .backgroundColor = GRAY1 
       })
       {
         CLAY(CLAY_ID("file_btn"),{
             .layout = {
              .sizing = {
               .width = CLAY_SIZING_FIT(),
               .height = CLAY_SIZING_FIT(),
               },
               .padding = {10,10,5,5}
             },
             .backgroundColor = GRAY3 
         })
         {
           CLAY_TEXT(CLAY_STRING("File"),{
               .fontId = 0,
               .fontSize = 14,
               .textColor = WHITE 
            });
           
           bool file_list_visible =
             Clay_PointerOver(Clay_GetElementId(CLAY_STRING("file_btn"))) 
             ||
             Clay_PointerOver(Clay_GetElementId(CLAY_STRING("file_list")));

           if(file_list_visible){

           CLAY(CLAY_ID("file_list"),{
               .floating = {
                 .attachTo = CLAY_ATTACH_TO_PARENT,
                 .attachPoints = {
                 .parent = CLAY_ATTACH_POINT_LEFT_BOTTOM,
                 },
                 // .offset = {5,5}
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
               .cornerRadius = {5,5,5,5},
               .backgroundColor = GRAY3
             }){
// //          Clicked example
// ❯           bool btn_clicked =
//             Clay_PointerOver(Clay_GetElementId(CLAY_STRING("_btn")))
//             &&
//             pointer.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME;
             CLAY_TEXT(CLAY_STRING("New"),{
                 .fontId = 0,
                 .fontSize = 14,
                 .textColor = WHITE 
                 });
             CLAY_TEXT(CLAY_STRING("Open"),{
                 .fontId = 0,
                 .fontSize = 14,
                 .textColor = WHITE
                 });
             CLAY_TEXT(CLAY_STRING("Save"),{
                 .fontId = 0,
                 .fontSize = 14,
                 .textColor = WHITE 
                 });
           }
           }
         }

       }
       CLAY(CLAY_ID("editor"),{
           .layout = {
            .sizing = layoutExpand
           }
       })
       {}
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

}
