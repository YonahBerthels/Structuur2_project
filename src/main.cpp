#include <M5Unified.h>

#include <stdlib.h>
#include <stdint.h>
#include "EEPROM.h"
#include "../lib/config.h"

// Global game state variables
Game game;
bool options_showing = false;

// Variables related to options-screen
u_int8_t selected_option = 0; // Standard the "BACK" option is selected

void setup() {
  M5.begin();
  M5.Imu.init();
  Serial.begin(115200);
  Serial.flush();
  M5.Lcd.fillScreen(COLOR_BLACK);

  init_game(&game);
  init_gems();
  game.gems[2].is_selected = true;
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
      draw_thick_rectangle(g.x_lt, g.y_lt, GEM_SIZE, GEM_SIZE, SELECTION_WIDTH, SELECTION_COLOR);
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
      game.gems[j * game.board_width + i] = g;
    }
  }
}

void draw_thick_rectangle(u_int8_t x_top, u_int8_t y_top, u_int8_t width, u_int8_t height, u_int8_t thickness, u_int16_t color) {
  for (int i = 0; i < thickness; i++) {
    M5.Lcd.drawRect(x_top - i, y_top - i, width + 2 * i, height + 2 * i, color);
  }
}

void display_gamestate(void) {
  M5.Lcd.setCursor(X_OFFSET, M5.Lcd.height() - 35);
  M5.Lcd.printf("Moves left: %d\n", game.moves_left);
    M5.Lcd.setCursor(X_OFFSET, M5.Lcd.height() - 25);
  M5.Lcd.printf("Score: %d\n", game.score);
}

void draw_option_screen(void) {
  const u_int8_t y_offsets[] = {Y_OFFSET, Y_OFFSET + 20, Y_OFFSET + 40, Y_OFFSET + 60};

  for (int i = 0; i < NUM_OPTIONS; i++) {
    M5.Lcd.setCursor(X_OFFSET, y_offsets[i]);

    u_int16_t color = (i == selected_option) ? RED : WHITE;
    M5.Lcd.setTextColor(color);

    M5.Lcd.printf("%s\n", OPTIONS[i]);
  }
}

void loop(void) {
  M5.update();
  delay(150);

  if (M5.BtnB.wasPressed()) {
    options_showing = !options_showing;
    if (!options_showing) 
      selected_option = 0; // reset the selected option after closing option screen
  }

  if (options_showing && M5.BtnA.wasPressed()) {
    selected_option = (selected_option + 1) % NUM_OPTIONS; // use MOD operator to go back to beginning if end of list is reached
  }

  M5.Lcd.clear();

  if (options_showing) {
    draw_option_screen();
    return;
  }

  draw_board();
  display_gamestate();
}