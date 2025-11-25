#include <M5Unified.h>

#include <stdlib.h>
#include <stdint.h>
#include "EEPROM.h"
#include "../lib/config.h"

Game game;

void setup() {
  M5.begin();
  M5.Imu.init();
  Serial.begin(115200);
  Serial.flush();
  M5.Lcd.fillScreen(COLOR_BLACK);

  init_game(&game);
  init_gems();
  game.gems[10].is_selected = true;
}

void init_game(Game *g) {
  g->level = 1;
  g->moves_left = 25;
  g->score = 0;
  g->board_height = 10;
  g->board_width = 7;
  // TODO: free this array/reseed the board
  g->gems = (Gem*) calloc((g->board_height * g->board_width), sizeof(Gem));
}

void draw_board(void) {
  for (int i = 0; i < game.board_height * game.board_width; i++) {
    Gem g = game.gems[i];
    u_int16_t color = COLORS[g.type];
    M5.Lcd.fillRect(g.x_lt, g.y_lt, GEM_SIZE, GEM_SIZE, color);
    // TODO: scale this to a selection of more than 1 square
    if (g.is_selected)
      M5.Lcd.drawRect(
        (g.x_lt - SELECTION_WIDTH), 
        (g.y_lt - SELECTION_WIDTH), 
        GEM_SIZE + 2 * SELECTION_WIDTH, 
        GEM_SIZE + 2 * SELECTION_WIDTH,
        SELECTION_COLOR
      );
  }
}

void init_gems(void) {
  srand(time(NULL));
  for (int i = 0; i < game.board_width; i++) {
    for (int j = 0; j < game.board_height; j++) {
      Gem g = {
        .type = rand() % 5,
        .x_lt = X_OFFSET + (i * GEM_SIZE),
        .y_lt = Y_OFFSET + (j * GEM_SIZE),
        .is_selected = false, 
      };
      game.gems[i * game.board_height + j] = g;
    }
  }
}

void display_gamestate(void) {
  M5.Lcd.setCursor(X_OFFSET, M5.Lcd.height() - 35);
  M5.Lcd.printf("Moves left: %d\n", game.moves_left);
    M5.Lcd.setCursor(X_OFFSET, M5.Lcd.height() - 25);
  M5.Lcd.printf("Score: %d\n", game.score);
}

void loop(void) {
  M5.update();
  delay(20);
  M5.Lcd.clear();
  draw_board();
  display_gamestate();
}