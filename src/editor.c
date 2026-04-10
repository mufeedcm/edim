#include "editor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
static Buffer *editor_current(Editor *ed){
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

int editor_buf_count(Editor *ed){
  return ed->buffer_count;
}

int editor_curr_index(Editor *ed){
  return ed->current_buffer;
}

const char *editor_buf_name(Editor *ed, int index){
  if(index < 0 || index>= ed->buffer_count) return "";
  Buffer *b = &ed->buffers[index];
  return b->filename ? b->filename : "untitled";
}

void editor_switch(Editor *ed, int index){
  if(index>=0 && index < ed->buffer_count){
    ed->current_buffer = index;
  }
}
