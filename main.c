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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// terminal ui
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

typedef struct{
  char *data;
  int gap_start;
  int gap_end;
  int capacity;
}GapBuffer;

typedef struct{
  GapBuffer gb;
  char *filename;
  int modified;
  int cursor_pos;
}Buffer;

typedef struct{
  Buffer *buffers;
  int buffer_count;
  int current_buffer;
}Editor;

// GapBuffer
void gb_init(GapBuffer *gb,int capacity){
  gb->data= malloc(capacity);
  if(!gb->data){
    perror("malloc");
    exit(1);
  }
  gb->capacity = capacity;
  gb->gap_start = 0;
  gb->gap_end = gb->capacity;
}

void gb_resize(GapBuffer *gb){
  int newcapacity = gb->capacity*2;
  char *new_data = malloc(newcapacity);
  if(!new_data){ 
    perror("malloc");
    exit(1);
  }
  int left_side = gb->gap_start;
  int right_side= gb->capacity-gb->gap_end;
  memcpy(new_data, gb->data, left_side);
  memcpy(new_data + ( newcapacity - right_side), gb->data+gb->gap_end, right_side);
  free(gb->data);
  gb->data = new_data;
  gb->gap_end = newcapacity - right_side;
  gb->capacity = newcapacity;
}

void gb_insert(GapBuffer *gb,char c){
  if(gb->gap_start == gb->gap_end){
    gb_resize(gb);
  }
  gb->data[gb->gap_start] = c;
  gb->gap_start++;
}

void gb_delete(GapBuffer *gb){
  if(gb->gap_start == 0) return ;
  gb->gap_start--;
}

void gb_move_left(GapBuffer *gb){
  if (gb->gap_start==0)  return ;
  gb->gap_start--;
  gb->gap_end--;
  gb->data[gb->gap_end]=gb->data[gb->gap_start];
}

void gb_move_right(GapBuffer *gb){
  if (gb->gap_end==gb->capacity)  return ;
  gb->data[gb->gap_start]=gb->data[gb->gap_end];
  gb->gap_start++;
  gb->gap_end++;
}


int gb_length(GapBuffer *gb){
  return  gb->capacity-(gb->gap_end-gb->gap_start);
}

char gb_get(GapBuffer *gb,int pos){
  if(pos<gb->gap_start){
    return gb->data[pos];
  }else{
    return gb->data[pos+(gb->gap_end - gb->gap_start)];
  }
}

int line_start(GapBuffer *gb,int pos){
  while(pos>0&&gb_get(gb,pos-1)!='\n') pos--;
  return pos;
}

int line_end(GapBuffer *gb,int pos){
  int len = gb_length(gb);
  while(pos< len&&gb_get(gb,pos)!='\n') pos++;
  return pos;
}


//Buffer
void b_init(Buffer *b,int capacity){
  gb_init(&b->gb, capacity);
  b->filename = NULL;
  b->modified = 0;
  b->cursor_pos =0;
}

void b_free(Buffer *b){
  free(b->gb.data);
  free(b->filename);
}

void b_insert(Buffer *b,char c){
  gb_insert(&b->gb, c);
  b->cursor_pos++;
  b->modified = 1;
}

void b_delete(Buffer *b){
  if (b->cursor_pos==0) return;

  gb_delete(&b->gb);
  b->cursor_pos--;
  b->modified = 1;
}

void b_move_left(Buffer *b){
  if (b->cursor_pos==0) return;

  gb_move_left(&b->gb);
  b->cursor_pos--;
}

void b_move_right(Buffer *b){
  if (b->cursor_pos>=gb_length(&b->gb)) return;
  gb_move_right(&b->gb);
  b->cursor_pos++;
}

void b_move_up(Buffer *b){
  if(b->cursor_pos == 0) return;
  int start = line_start(&b->gb,b->cursor_pos);
  int col = b->cursor_pos- start;

  if(start==0) return;
  int prev_end = start-1;
  int prev_start= line_start(&b->gb, prev_end);
  int prev_len = prev_end- prev_start;

  int target = prev_start + (col < prev_len ? col : prev_len);

  while(b->cursor_pos > target) b_move_left(b);
  while(b->cursor_pos < target) b_move_right(b);
}

void b_move_down(Buffer *b){
  int len = gb_length(&b->gb);

  int start = line_start(&b->gb,b->cursor_pos);
  int col = b->cursor_pos- start;

  int end = line_end(&b->gb, b->cursor_pos);
  if(end==len) return;

  int next_start = end+1;
  int next_end = line_end(&b->gb, next_start);

  int next_len = next_end - next_start;

  int target = next_start + (col < next_len ? col : next_len);

  while(b->cursor_pos > target) b_move_left(b);
  while(b->cursor_pos < target) b_move_right(b);
}

void b_load(Buffer *b, const char *path){
  FILE *f = fopen(path,"rb");
  if(!f){
    perror("fopen");
    return;
  }
  fseek(f,0,SEEK_END);
  long size = ftell(f);
  rewind(f);
  if(size>=b->gb.capacity){
    free(b->gb.data);
    b->gb.capacity = size*2;
    b->gb.data= malloc(b->gb.capacity);
    if(!b->gb.data){
      perror("malloc");
      fclose(f);
      exit(1);
    }
  }
  fread(b->gb.data, 1, size, f);
  b->gb.gap_start = size;
  b->gb.gap_end = b->gb.capacity;
  fclose(f);
  if (b->filename) free(b->filename);
  b->filename = malloc(strlen(path)+1);
  if(!b->filename){
    perror("malloc");
    exit(1);
  }
  strcpy(b->filename, path);
  b->modified = 0;
  b->cursor_pos = size;
}

void b_save(Buffer *b,const char *path){
  FILE *f = fopen(path,"wb");
  if(!f){
    perror("fopen");
    return;
  }
  size_t left = b->gb.gap_start;
  size_t right = b->gb.capacity-b->gb.gap_end;

  fwrite(b->gb.data, 1,left ,f);
  fwrite(b->gb.data+b->gb.gap_end,1,right,f);

  fclose(f);
}

/* Editor */
Buffer *editor_current(Editor *ed){
  if(ed->current_buffer<0) return NULL;
  return &ed->buffers[ed->current_buffer];
}

void editor_init(Editor *ed){
  ed->buffers = NULL;
  ed->buffer_count = 0;
  ed->current_buffer = -1;

}
void editor_new(Editor *ed){
  Buffer *new_buffers = realloc(ed->buffers, sizeof(Buffer)*(ed->buffer_count+1));
  if(!new_buffers){
    perror("realloc");
    exit(1);
  }
  ed->buffers = new_buffers;
  Buffer *b = &ed->buffers[ed->buffer_count];
  b_init(b, 16);

  ed->current_buffer = ed->buffer_count;
  ed->buffer_count++;
}

void editor_open(Editor *ed,const char *path){
  Buffer *new_buffers = realloc(ed->buffers, sizeof(Buffer)*(ed->buffer_count+1));
  if(!new_buffers){
    perror("realloc");
    exit(1);
  }
  ed->buffers = new_buffers;
  Buffer *b = &ed->buffers[ed->buffer_count];

  b_init(b, 16);
  b_load(b,path);

  ed->current_buffer = ed->buffer_count;
  ed->buffer_count++;
}

void editor_insert(Editor *ed,char c){
  Buffer *b = editor_current(ed);
  if(!b) return;
  b_insert(b, c);
}

void editor_insert_str(Editor *ed,char *s){
  while(*s){
    editor_insert(ed, *s++);
  }
}

void editor_delete(Editor *ed){
  Buffer *b = editor_current(ed);
  if(!b) return;
  b_delete(b);
}
void editor_move_left(Editor *ed){
  Buffer *b = editor_current(ed);
  if(!b) return;
  b_move_left(b);
}
void editor_move_right(Editor *ed){
  Buffer *b = editor_current(ed);
  if(!b) return;
  b_move_right(b);
}

void editor_move_up(Editor *ed){
  Buffer *b = editor_current(ed);
  if(!b) return;
  b_move_up(b);
}

void editor_move_down(Editor *ed){
  Buffer *b = editor_current(ed);
  if(!b) return;
  b_move_down(b);
}

void editor_save(Editor *ed){
  Buffer *b=editor_current(ed);
  if(!b) return;
  if(!b->filename){
    b->filename = malloc(strlen("out.txt")+1);
    strcpy(b->filename, "out.txt");
  }
  b_save(b, b->filename);
  b->modified = 0;
}

void editor_free(Editor *ed){
  for(int i = 0;i<ed->buffer_count;i++){
    b_free(&ed->buffers[i]);
  }
  free(ed->buffers);
}

int editor_cursor(Editor *ed){
  Buffer *b=editor_current(ed);
  return b ? b->cursor_pos :0;
}

int editor_length(Editor *ed){
  Buffer *b=editor_current(ed);
  return b ? gb_length(&b->gb) :0;
}

int editor_get(Editor *ed,int pos){
  Buffer *b=editor_current(ed);
  return b ? gb_get(&b->gb,pos) :0;
}


// terminal
struct termios orig_terminos;
void disable_raw_mode(void){
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_terminos);
}

void enable_raw_mode(void){
  tcgetattr(STDIN_FILENO, &orig_terminos);
  atexit(disable_raw_mode);
  struct termios raw = orig_terminos;
  raw.c_lflag &= ~(ECHO | ICANON | ISIG);
  raw.c_iflag &= ~(IXON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
void get_window_size(int *rows,int *cols){
  struct winsize ws;

  if(ioctl(STDIN_FILENO, TIOCGWINSZ,&ws) == -1 || ws.ws_col == 0){
    *rows=24;
    *cols= 80;
  }else{
    *rows = ws.ws_row;
    *cols = ws.ws_col;
  }
}

void render(Editor *ed){
  Buffer *b = editor_current(ed);
  if(!b) return;

  printf("\x1b[2J\x1b[H"); //clear the screen and move the cursor to top left
  int len=editor_length(ed);

  int rows,cols;
  get_window_size(&rows, &cols);

  int gutter_width  = 6;

  int line =1;
  int row =0, col=0;

  printf("%d   | ",line++);
  for(int i =0; i<len; i++) {
    char c =  editor_get(ed,i);
    if( c == '\n') {
      row++;
      col = 0;
      printf("\n%d   | ",line++);
      continue;
    }
    if(col>=(cols-gutter_width)){
      row++;
      col=0;
      printf("\n    |  ");
    }
    putchar(c);
    col++;
  }
  int crow=0,ccol=0;
  int cursor = editor_cursor(ed);

 for(int i=0;i<cursor; i++){
    char c = editor_get(ed,i);
    if( c == '\n'){
      crow++;
      ccol=0;
      continue;
    }
    if(ccol>=cols-gutter_width){
      crow++;
      ccol=0;
    }
    ccol++;
  }
  printf("\x1b[%d;%dH",crow+1,ccol+(gutter_width));
}

int main(int argc,char **argv){
  enable_raw_mode();
  Editor ed;

  editor_init(&ed);
  if(argc>1){
    editor_open(&ed, argv[1]);
  }else{
    editor_new(&ed);
  }

  while (1) {
    render(&ed);
    int c = getchar();
    // printf("%c = %d\n",c,c);
    if(c == EOF) break;
    if(c == 17) break; 
    else if(c == 27) {
      int c1 = getchar();
      if(c1!='[') continue; ;
      int c2 = getchar();
      switch (c2) {
        case 'A' : editor_move_up(&ed); 
                   break;
        case 'B' : editor_move_down(&ed); 
                   break;
        case 'C' : editor_move_right(&ed); 
                   break;
        case 'D' : editor_move_left(&ed); 
                   break;
      }
    }
    else if(c == '\r') editor_insert(&ed, '\n');
    else if(c == 127|| c ==8){
      editor_delete(&ed);
    }else{
      editor_insert(&ed, c);
    }
  }
  editor_save(&ed);
  editor_free(&ed);
  return 0;
}
