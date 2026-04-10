#ifndef EDITOR_H
#define EDITOR_H

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

void editor_init(Editor *ed);
void editor_new(Editor *ed);
void editor_open(Editor *ed,const char *path);
void editor_insert(Editor *ed,char c);
void editor_insert_str(Editor *ed,char *s);
void editor_delete(Editor *ed);
void editor_move_left(Editor *ed);
void editor_move_right(Editor *ed);
void editor_move_up(Editor *ed);
void editor_move_down(Editor *ed);
void editor_save(Editor *ed);
void editor_free(Editor *ed);
int editor_cursor(Editor *ed);
int editor_length(Editor *ed);
int editor_get(Editor *ed,int pos);
int editor_buf_count(Editor *ed);
int editor_curr_index(Editor *ed);
const char *editor_buf_name(Editor *ed, int index);
void editor_switch(Editor *ed, int index);

#endif
