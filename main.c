/*
 * muedit - a minimal editor written in C.
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

typedef struct{
  char *data;
  int gap_start;
  int gap_end;
  int capacity;

  char *filename;
  int modified;
}TextBuffer;

typedef struct{
  TextBuffer *buffers;
  int buffer_count;
  int current_buffer;
}Editor;


/* Buffer */
void tb_init(TextBuffer *tb,int capacity){
  tb->data= malloc(capacity);
  if(!tb->data){
    perror("malloc");
    exit(1);
  }
  tb->capacity = capacity;
  tb->gap_start = 0;
  tb->gap_end = tb->capacity;
  tb->filename = NULL;
  tb->modified = 0;
}

void tb_resize(TextBuffer *tb){
  int newcapacity = tb->capacity*2;
  char *new_data = malloc(newcapacity);
  if(!new_data){ 
    perror("malloc");
    exit(1);
  }
  int left_side = tb->gap_start;
  int right_side= tb->capacity-tb->gap_end;
  memcpy(new_data, tb->data, left_side);
  memcpy(new_data + ( newcapacity - right_side), tb->data+tb->gap_end, right_side);
  free(tb->data);
  tb->data = new_data;
  tb->gap_end = newcapacity - right_side;
  tb->capacity = newcapacity;
}

void tb_insert(TextBuffer *tb,char c){
  if(tb->gap_start == tb->gap_end){
    tb_resize(tb);
  }
  tb->data[tb->gap_start] = c;
  tb->gap_start++;

  tb->modified = 1;
}

void tb_move_left(TextBuffer *tb){
  if (tb->gap_start==0)  return ;
  tb->gap_start--;
  tb->gap_end--;
  tb->data[tb->gap_end]=tb->data[tb->gap_start];
}

void tb_move_right(TextBuffer *tb){
  if (tb->gap_end==tb->capacity)  return ;
  tb->data[tb->gap_start]=tb->data[tb->gap_end];
  tb->gap_start++;
  tb->gap_end++;
}

void tb_delete(TextBuffer *tb){
  if(tb->gap_start == 0) return ;
  tb->gap_start--;

  tb->modified = 1;
}

int tb_length(TextBuffer *tb){
  return  tb->capacity-(tb->gap_end-tb->gap_start);
}


void tb_print(TextBuffer *tb){
  for(int i =0; i<tb->gap_start; i++) {
    putchar(tb->data[i]);
  }
  for(int i = tb->gap_end;i< tb->capacity;i++){
    putchar(tb->data[i]);
  }
  putchar('\n');
}

void tb_load(TextBuffer *tb, const char *path){
  FILE *f = fopen(path,"rb");
  if(!f){
    perror("fopen");
    return;
  }
  fseek(f,0,SEEK_END);
  long size = ftell(f);
  rewind(f);
  if(size>=tb->capacity){
    free(tb->data);
    tb->capacity = size*2;
    tb->data= malloc(tb->capacity);
    if(!tb->data){
      perror("malloc");
      fclose(f);
      exit(1);
    }
  }
  fread(tb->data, 1, size, f);
  tb->gap_start = size;
  tb->gap_end = tb->capacity;
  tb->modified = 0;
  fclose(f);
}

void tb_save(TextBuffer *tb,const char *path){
  FILE *f = fopen(path,"wb");
  if(!f){
    perror("fopen");
    return;
  }
  size_t left = tb->gap_start;
  size_t right = tb->capacity-tb->gap_end;

  fwrite(tb->data, 1,left ,f);
  fwrite(tb->data+tb->gap_end,1,right,f);

  fclose(f);
}

void tb_free(TextBuffer *tb){
  free(tb->data);
  free(tb->filename);
}

/* Editor */
TextBuffer *editor_current(Editor *ed){
  if(ed->current_buffer<0) return NULL;
  return &ed->buffers[ed->current_buffer];
}

void editor_init(Editor *ed){
  ed->buffers = NULL;
  ed->buffer_count = 0;
  ed->current_buffer = -1;

}

void editor_new(Editor *ed){
  TextBuffer *new_buffers = realloc(ed->buffers, sizeof(TextBuffer)*(ed->buffer_count+1));
  if(!new_buffers){
    perror("realloc");
    exit(1);
  }
  ed->buffers = new_buffers;
  TextBuffer *tb = &ed->buffers[ed->buffer_count];
  tb_init(tb, 16);

  ed->current_buffer = ed->buffer_count;
  ed->buffer_count++;
}

void editor_open(Editor *ed,const char *path){
  TextBuffer *new_buffers = realloc(ed->buffers, sizeof(TextBuffer)*(ed->buffer_count+1));
  if(!new_buffers){
    perror("realloc");
    exit(1);
  }
  ed->buffers = new_buffers;
  TextBuffer *tb = &ed->buffers[ed->buffer_count];

  tb_init(tb, 16);
  tb_load(tb,path);

  ed->current_buffer = ed->buffer_count;
  ed->buffer_count++;

  tb->filename = malloc(strlen(path)+1);
  if(!tb->filename){
    perror("malloc");
    exit(1);
  }
  strcpy(tb->filename, path);


  tb->modified = 0;
}

void editor_insert(Editor *ed,char c){
  TextBuffer *tb = editor_current(ed);
  if(!tb) return;
  tb_insert(tb, c);
}
void editor_delete(Editor *ed){
  TextBuffer *tb = editor_current(ed);
  if(!tb) return;
  tb_delete(tb);
}
void editor_move_left(Editor *ed){
  TextBuffer *tb = editor_current(ed);
  if(!tb) return;
  tb_move_left(tb);
}
void editor_move_right(Editor *ed){
  TextBuffer *tb = editor_current(ed);
  if(!tb) return;
  tb_move_right(tb);
}

void editor_save(Editor *ed){
  TextBuffer *tb=editor_current(ed);
  if(!tb) return;
  if(!tb->filename){
    tb->filename = malloc(strlen("out.txt")+1);
    strcpy(tb->filename, "out.txt");
  }
  tb_save(tb, tb->filename);
  tb->modified = 0;
}
void editor_print(Editor *ed){
  TextBuffer *tb = editor_current(ed);
  if(!tb) return;
  tb_print(tb);
}
void editor_free(Editor *ed){
  for(int i = 0;i<ed->buffer_count;i++){
    tb_free(&ed->buffers[i]);
  }
  free(ed->buffers);
}

int main(int argc,char **argv){
  Editor ed;

  editor_init(&ed);
  if(argc>1){
    editor_open(&ed, argv[1]);
  }else{
    editor_new(&ed);
  }

  while (1) {
    printf("\x1b[2J\x1b[H"); //clear the screen and move the cursor to top left
    editor_print(&ed);
    printf("\nCtrl+D to quit\n");
    int c = getchar();
    if(c == EOF) break;
    if(c == 127|| c ==8){
      editor_delete(&ed);
    }else{
      editor_insert(&ed, c);
    }
  }
  editor_save(&ed);
  editor_free(&ed);
  return 0;
}
