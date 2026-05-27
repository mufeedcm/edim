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
#include "app.h"

int main(int argc,char **argv){
  (void)argc;
  (void)argv;

  App app = {0};
  if(!app_init(&app)){
    app_destroy(&app);
    return 1;
  }

#ifdef __EMSCRIPTEN__
  app_run_web(&app);
#else
  while(app.running){
    app_loop(&app);
  }
#endif
  app_destroy(&app);

  return 0;
}
