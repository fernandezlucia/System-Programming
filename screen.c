/* ** por compatibilidad se omiten tildes **
================================================================================
 TALLER System Programming - ORGANIZACION DE COMPUTADOR II - FCEN
================================================================================

  Definicion de funciones de impresion por pantalla.
*/

#include "screen.h"

void print(const char* text, uint32_t x, uint32_t y, uint16_t attr) {
  ca(*p)[VIDEO_COLS] = (ca(*)[VIDEO_COLS])VIDEO; 
  int32_t i;
  for (i = 0; text[i] != 0; i++) {
    p[y][x].c = (uint8_t)text[i];
    p[y][x].a = (uint8_t)attr;
    x++;
    if (x == VIDEO_COLS) {
      x = 0;
      y++;
    }
  }
}

void print_dec(uint32_t numero, uint32_t size, uint32_t x, uint32_t y,
               uint16_t attr) {
  ca(*p)[VIDEO_COLS] = (ca(*)[VIDEO_COLS])VIDEO; 
  uint32_t i;
  uint8_t letras[16] = "0123456789";

  for (i = 0; i < size; i++) {
    uint32_t resto = numero % 10;
    numero = numero / 10;
    p[y][x + size - i - 1].c = letras[resto];
    p[y][x + size - i - 1].a = attr;
  }
}

void print_hex(uint32_t numero, int32_t size, uint32_t x, uint32_t y,
               uint16_t attr) {
  ca(*p)[VIDEO_COLS] = (ca(*)[VIDEO_COLS])VIDEO; 
  int32_t i;
  uint8_t hexa[8];
  uint8_t letras[16] = "0123456789ABCDEF";
  hexa[0] = letras[(numero & 0x0000000F) >> 0];
  hexa[1] = letras[(numero & 0x000000F0) >> 4];
  hexa[2] = letras[(numero & 0x00000F00) >> 8];
  hexa[3] = letras[(numero & 0x0000F000) >> 12];
  hexa[4] = letras[(numero & 0x000F0000) >> 16];
  hexa[5] = letras[(numero & 0x00F00000) >> 20];
  hexa[6] = letras[(numero & 0x0F000000) >> 24];
  hexa[7] = letras[(numero & 0xF0000000) >> 28];
  for (i = 0; i < size; i++) {
    p[y][x + size - i - 1].c = hexa[i];
    p[y][x + size - i - 1].a = attr;
  }
}

void screen_draw_box(uint32_t fInit, uint32_t cInit, uint32_t fSize,
                     uint32_t cSize, uint8_t character, uint8_t attr) {
  ca(*p)[VIDEO_COLS] = (ca(*)[VIDEO_COLS])VIDEO;
  uint32_t f;
  uint32_t c;
  for (f = fInit; f < fInit + fSize; f++) {
    for (c = cInit; c < cInit + cSize; c++) {
      p[f][c].c = character;
      p[f][c].a = attr;
    }
  }
}

void screen_draw_layout(void) {


  uint8_t c1 = 0x01;//  verde fondo;
  //uint8_t c2 = 0x02;//  verde bordes;
  //uint8_t c3 = 0x33;//  0b00110011;
  uint8_t c4 = 0x03;//  verde nombres;

  //fondo
  screen_draw_box(0,0,VIDEO_FILS, VIDEO_COLS, '.', c1);
  
  //bordes
  /*
  screen_draw_box(0,0,VIDEO_FILS,1, '#', c2);   //izq
  screen_draw_box(VIDEO_FILS-1,0,1,VIDEO_COLS, '$', c2); //bot
  screen_draw_box(0,0,1,VIDEO_COLS, '%', c2); //top
  screen_draw_box(0,VIDEO_COLS-1,VIDEO_FILS,1, '9',c2);//der
  */

  int mid_x = VIDEO_COLS / 2 - 10;
  print("Fernando Parra",mid_x,1, c4);
  print("Julieta Giri",mid_x,2, c4);
  print("Lucia Ariadna Fernandez",mid_x,3,c4);

  //cell_init_all();
}

void cell_init_all(){

  for(int i = 0; i < VIDEO_FILS * VIDEO_COLS; i++){
    cell[i] = 0;
    cell_buffer[i] = 0;
  }

  for(int x = 4; x < VIDEO_COLS - 4; x++){
    for(int y = 10; y < VIDEO_FILS - 10; y++){

      int i = x + y * VIDEO_COLS;

      if(i % 2 == 0){
        cell[i] = 1;
        print("o", x, y, 0xF0);
      } else {
        cell[i] = 0;
      }   

    }

  }


}

void cell_clear(){
  uint8_t c1 = 0x06;//  verde fondo;
  screen_draw_box(10,4, VIDEO_FILS - 20, VIDEO_COLS - 8, '.', c1);
}

void cell_proc(){

  cell_clear();

  for(int y = 0; y < VIDEO_FILS; y++){
    for(int x = 0; x < VIDEO_COLS; x++){
      
      int i = x + y * VIDEO_COLS;
      int neighs = cell_get_neighboors(x, y);
  
      if(cell[i] == 1){
        if(neighs < 2){
          cell_buffer[i] = 0;
        } else if(neighs > 3){
          cell_buffer[i] = 0;
        } else {
          cell_buffer[i] = 1;
          print("o", x, y, 0xF0);
        }
      } else {
        if(neighs == 3){
          cell_buffer[i] = 1;
          print("o", x, y, 0xF0);
        }
      }
    }
  }

  for(int i = 0; i < VIDEO_FILS * VIDEO_COLS; i++){
    cell[i] = cell_buffer[i];
  }

}

int  cell_get_neighboors(int cx, int cy){
  int res = 0;

  for (int y = -1; y < 2; y++){
    for (int x = -1; x < 2; x++){
      
      int next_x = cx+x;
      int next_y = cy+y;

      //no deberia pasar:
      if(next_x < 0 || next_x >= VIDEO_COLS || next_y < 0 || next_y >= VIDEO_FILS){
        res++;
        continue;
      }

      if(x == 0 && y == 0){
        continue;
      } else {
        int i = next_x + next_y * VIDEO_COLS;
        if(cell[i] == 1){
          res++;
        }
      }
    }
  }
  
  return res;
}
