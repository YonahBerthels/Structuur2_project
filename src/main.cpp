#include <M5Unified.h>
#include <stdlib.h>
#include <stdint.h>
#include "EEPROM.h"
#include "../lib/config.h"

// temporary boolean mask used to mark gems that are part of a match (gets "cleaned" and reused)
// static so just initted once
static bool matches_buffer[MAX_BOARD_WIDTH * MAX_BOARD_HEIGHT];

// Global game variable + other variables
Game game;
bool options_showing = false;
int selected_gem_idx = 0;
int second_selected_idx = 1;
bool is_horizontal = true;
float acc_x, acc_y, acc_z;
unsigned long last_move_time = 0;
bool game_started = false;
uint8_t selected_variant = 0;
uint8_t selected_option = 0;
bool board_ready = false;
float sx, sy, sz;

void setup()
{
  M5.begin();
  M5.Imu.init();
  Serial.begin(115200);
  Serial.flush();
  M5.Lcd.fillScreen(BLACK);

  // seed random generator
  srand(time(NULL));

  // init gems as NULL just to be sure
  game.gems = NULL;
}

// init game
void init_game(Game *g, uint8_t level)
{
  g->level = level;
  g->moves_left = 25;
  if (level == 1)
    g->score = 0;

  g->board_width = 7;
  g->board_height = 11 - level;
  // no need to check for negative sizes ect since the game ends once the user has completed the level with height of 3

  if (g->gems)
  {
    free(g->gems);
    g->gems = NULL;
  }

  // dynamically allocate an array for the gems based on current level size
  // this array in freed in the code above to prepare for a new level with different dimensions
  // max size = 8 x 11 x sizeof(Gem) ()
  g->gems = (Gem *)calloc(g->board_width * g->board_height, sizeof(Gem));

  // choose random number of colors per level (part of assignment)
  int max_color = 8;
  int min_color = 4;
  int range = max_color - min_color + 1;
  int chosen = min_color + (rand() % range);
  g->num_colors = (u_int8_t)chosen;

  init_gems();

  selected_gem_idx = 0;
  second_selected_idx = 1;

  // start the cursor horizontally
  is_horizontal = true;
  update_selection();
}

u_int8_t get_gem_x(int index)
{
  int col = index % game.board_width;
  return X_OFFSET + (col * GEM_SIZE);
}

u_int8_t get_gem_y(int index)
{
  int row = index / game.board_width;
  return Y_OFFSET + (row * GEM_SIZE);
}

// function to draw the border around 2 gems. Representation of the cursor
void draw_thick_rectangle(u_int8_t x_top, u_int8_t y_top, u_int8_t width, u_int8_t height, u_int8_t thickness, u_int16_t color)
{
  for (int i = 0; i < thickness; i++)
    M5.Lcd.drawRect(x_top + i, y_top + i, width - 2 * i, height - 2 * i, color);
}

// simple function to draw the cells on the board
// the x and y coordinates are based off of GEM_SIZE + GEM_OFFSET + index to preserve size so these values don't need to be stored as members of the struct
void draw_board(void)
{
  int total = game.board_width * game.board_height;
  for (int i = 0; i < total; i++)
  {
    Gem g = game.gems[i];

    u_int16_t color = BLACK;
    if (g.type != GEM_EMPTY && g.type < game.num_colors)
    {
      color = COLORS[g.type];
    }

    u_int8_t x = get_gem_x(i);
    u_int8_t y = get_gem_y(i);

    M5.Lcd.fillRect(x, y, GEM_SIZE, GEM_SIZE, color);

    if (g.is_selected)
      draw_thick_rectangle(x, y, GEM_SIZE, GEM_SIZE, SELECTION_WIDTH, SELECTION_COLOR);
  }
}

// small text to show the game state to the usre
void display_gamestate(void)
{
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setCursor(X_OFFSET, M5.Lcd.height() - 35);
  M5.Lcd.printf("Level: %d  Moves: %d\n", game.level, game.moves_left);
  M5.Lcd.setCursor(X_OFFSET, M5.Lcd.height() - 25);
  M5.Lcd.printf("Score: %d\n", game.score);
}

void update_selection()
{
  int total = game.board_width * game.board_height;
  for (int i = 0; i < total; i++)
    game.gems[i].is_selected = false;

  if (selected_gem_idx >= 0 && selected_gem_idx < total)
    game.gems[selected_gem_idx].is_selected = true;

  if (second_selected_idx >= 0 && second_selected_idx < total)
    game.gems[second_selected_idx].is_selected = true;
}

void rotate_cursor()
{
  int row = selected_gem_idx / game.board_width;
  int col = selected_gem_idx % game.board_width;

  if (is_horizontal)
  {
    if (row < game.board_height - 1)
    {
      second_selected_idx = selected_gem_idx + game.board_width;
      is_horizontal = false;
    }
  }
  else
  {
    if (col < game.board_width - 1)
    {
      second_selected_idx = selected_gem_idx + 1;
      is_horizontal = true;
    }
  }

  update_selection();
}

// move cursor
// usage of accelerometer based on WPO 9 (exercise 4 - 5)
void move_selection(float ax, float ay)
{
  int row = selected_gem_idx / game.board_width;
  int col = selected_gem_idx % game.board_width;

  if (ax > MIN_TILT)
  {
    col -= 1;
  }
  else if (ax < -MIN_TILT)
  {
    col += 1;
  }

  if (ay > MIN_TILT)
  {
    row += 1;
  }
  else if (ay < -MIN_TILT)
  {
    row -= 1;
  }

  if (col < 0)
    col = game.board_width - 1;
  if (col >= game.board_width)
    col = 0;
  if (row < 0)
    row = game.board_height - 1;
  if (row >= game.board_height)
    row = 0;

  selected_gem_idx = row * game.board_width + col;

  if (is_horizontal)
  {
    second_selected_idx = selected_gem_idx + 1;
    if ((second_selected_idx % game.board_width) == 0)
      second_selected_idx = selected_gem_idx - 1;
  }
  else
  {
    second_selected_idx = selected_gem_idx + game.board_width;
    if (second_selected_idx >= game.board_width * game.board_height)
      second_selected_idx = selected_gem_idx - game.board_width;
  }

  update_selection();
}

void clear_marks(bool *marks)
{
  int total = game.board_width * game.board_height;
  int i;
  for (i = 0; i < total; i++)
    marks[i] = false;
}

int gem_idx(int r, int c)
{
  return r * game.board_width + c;
}

// Returns true if any matches found; marks all matched cells in marks[]
// usage of boolean array to separate concerns: first we scan and mark cells for deletion, after scan is complete they are deleted
bool find_matches(bool *to_remove)
{
  bool match_found = false;

  clear_marks(to_remove);

  // horizontal scan
  for (int row = 0; row < game.board_height; row++)
  {
    int col = 0;

    while (col < game.board_width)
    {
      uint8_t current_type = game.gems[gem_idx(row, col)].type;

      // skip gems that are already marked for removal
      if (current_type == GEM_EMPTY)
      {
        col++;
        continue;
      }

      // start the run
      int run_start_col = col;

      while (col + 1 < game.board_width &&
             game.gems[gem_idx(row, col + 1)].type == current_type)
      {
        col++;
      }

      int run_length = col - run_start_col + 1;

      // run > 3 -> match found and mark for removal
      if (run_length >= 3)
      {
        match_found = true;

        for (int mark_col = run_start_col; mark_col <= col; mark_col++)
        {
          to_remove[gem_idx(row, mark_col)] = true;
        }
      }

      col++;
    }
  }

  // vertical scan
  for (int col = 0; col < game.board_width; col++)
  {
    int row = 0;

    while (row < game.board_height)
    {
      uint8_t current_type = game.gems[gem_idx(row, col)].type;

      if (current_type == GEM_EMPTY)
      {
        row++;
        continue;
      }

      // Start of a potential vertical run
      int run_start_row = row;

      // Extend run downward
      while (row + 1 < game.board_height &&
             game.gems[gem_idx(row + 1, col)].type == current_type)
      {
        row++;
      }

      int run_length = row - run_start_row + 1;

      // run > 3 -> match found and mark for removal
      if (run_length >= 3)
      {
        match_found = true;

        for (int mark_row = run_start_row; mark_row <= row; mark_row++)
        {
          to_remove[gem_idx(mark_row, col)] = true;
        }
      }

      row++;
    }
  }

  return match_found;
}

// mark "real" board gems for deletoin (empty value) + update score
void remove_matches(bool *marks, bool player_move)
{
  int total = game.board_width * game.board_height;
  int i;
  for (i = 0; i < total; i++)
  {
    if (marks[i])
    {
      game.gems[i].type = GEM_EMPTY;
      if (player_move)
        game.score += GEM_VALUE;
    }
  }
}

// move gems down after deletion of underlying gems: "gravity" effect
void move_gems_down(void)
{
  for (int col = 0; col < game.board_width; col++)
  {
    // new gems will appear in the top row
    int write_row = game.board_height - 1;

    // move a row
    for (int row = game.board_height - 1; row >= 0; row--)
    {
      uint8_t t = game.gems[gem_idx(row, col)].type;
      if (t != GEM_EMPTY)
      {
        if (write_row != row)
          game.gems[gem_idx(write_row, col)].type = t;
        write_row--;
      }
    }

    // fill top cells
    while (write_row >= 0)
    {
      game.gems[gem_idx(write_row, col)].type = (uint8_t)(rand() % game.num_colors);
      write_row--;
    }
  }

  update_selection();
}

// handle a match
// the boolean parameter is a remnant of a previous version, but is kept for safety -> if loading from EEPROM somehow produces a match, it is not counted towards the user's score
void handle_matches(bool is_user_move)
{
  bool found;

  do
  {
    found = find_matches(matches_buffer);
    if (found)
    {
      remove_matches(matches_buffer, is_user_move);
      move_gems_down();
    }
  } while (found);

  if (is_user_move)
    check_level_up();
}

// only swap the types of a gem to prevent moving the cursor any only swaps types. The cursor should stay the same since we are using 2 "state" variables which would need to get updated too etc.
void swap_types(int a, int b)
{
  uint8_t t = game.gems[a].type;
  game.gems[a].type = game.gems[b].type;
  game.gems[b].type = t;
}

// swap 2 gems
void swap_gems(void)
{
  // this buffer is only used in "valid only" mode
  bool tmp[MAX_BOARD_WIDTH * MAX_BOARD_HEIGHT];

  swap_types(selected_gem_idx, second_selected_idx);
  game.moves_left--;

  // version 0: always swap
  if (game.version == 0)
  {
    handle_matches(true);
  }
  else
  // version 1: only let swap go through if it creates a match
  {
    if (find_matches(tmp))
    {
      handle_matches(true);
    }
    else
    {
      // rollback if no match was found
      swap_types(selected_gem_idx, second_selected_idx);
      game.moves_left++;
    }
  }

  update_selection();
}

// init the gems
// this is done randomly until there are a board without matches is found. This is not the most effective way but looks like a cool "init animation" so i kept it like this
void init_gems(void)
{
  bool has_matches;
  int total = game.board_width * game.board_height;

  do
  {
    for (int i = 0; i < total; i++)
    {
      game.gems[i].type = (uint8_t)(rand() % game.num_colors);
      game.gems[i].is_selected = false;
    }

    has_matches = find_matches(matches_buffer);

    M5.Lcd.clear();
    draw_board();
    delay(30);
  } while (has_matches);
}

// when player completes level with height of 3 the game is won. This because with a height of 2 only horizontal matches are possible which is a lot harder
void show_victory_screen()
{
  M5.Lcd.fillScreen(BLUE);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(3);
  M5.Lcd.drawCenterString("YOU WIN!", M5.Lcd.width() / 2, 80);

  M5.Lcd.setTextSize(1);
  M5.Lcd.drawCenterString(String("Total Score: ") + String(game.score), M5.Lcd.width() / 2, 150);
}

// check if the user needs to progress to the netx level
void check_level_up()
{
  // arbitrary target score per level
  int target_score = game.level * MATCHES_PER_LEVEL * GEM_VALUE;
  if (game.score < target_score)
    return;

  // reduce height by 1 each level since the dimensions needed to change
  int next_height = 11 - (game.level + 1);
  if (next_height < 3)
  {
    show_victory_screen();
    return;
  }

  game.level++;
  game.moves_left = 25;

  M5.Lcd.fillScreen(GREEN);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.drawCenterString("LEVEL UP!", M5.Lcd.width() / 2, 100);
  M5.Lcd.setTextSize(1);
  M5.Lcd.drawCenterString(String("Level ") + String(game.level), M5.Lcd.width() / 2, 140);
  delay(1200);

  int temp_score = game.score;
  init_game(&game, game.level);
  game.score = temp_score;

  M5.Lcd.fillScreen(BLACK);
}

// encoding and decoding based on the techniques seen in WPO 7 + revision in the last WPO
uint8_t encode_gem(Gem g)
{
  uint8_t shifted_selected = g.is_selected << 7;
  uint8_t encoded = shifted_selected | g.type;
  return encoded;
}

Gem decode_gem(uint8_t encoded)
{
  uint8_t bitmask_selected = 0b10000000;
  uint8_t shifted_selected = encoded & bitmask_selected;
  bool is_selected = shifted_selected >> 7;

  uint8_t bitmask_type = 0b01111111;
  uint8_t type = encoded & bitmask_type;

  Gem g = {type, is_selected};
  return g;
}

void save_to_eeprom(void)
{
  EEPROM.begin(MEM_SIZE);
  EEPROM.write(0, game.version);
  EEPROM.write(1, game.level);
  EEPROM.write(2, game.moves_left);
  // since score is a u_int16 it is being split into 2 bytes (u_int8 each)
  // using byte shifting the first and second byte are read separatly
  // seen in several WPOs but "splitting" the u_int16_t was something in which i only succeeded after a lot of trail and error
  // we are storing the low byte first so this is little endian
  EEPROM.write(3, (u_int8_t)(game.score & 0b11111111));
  EEPROM.write(4, (u_int8_t)((game.score >> 8) & 0b11111111));
  EEPROM.write(5, game.board_width);
  EEPROM.write(6, game.board_height);
  EEPROM.write(7, (u_int8_t)selected_gem_idx);
  EEPROM.write(8, (u_int8_t)second_selected_idx);
  EEPROM.write(9, (u_int8_t)is_horizontal);

  int total = game.board_width * game.board_height;
  for (int i = 0; i < total; i++)
    EEPROM.write(10 + i, encode_gem(game.gems[i]));

  EEPROM.commit();
  EEPROM.end();
}

void load_from_eeprom()
{
  EEPROM.begin(MEM_SIZE);

  game.version = EEPROM.read(0);
  game.level = EEPROM.read(1);
  game.moves_left = EEPROM.read(2);
  // read in 2 bytes and "combine" them into 1 u_int16 for the score
  // again, byte shifting is used to get the correct result
  u_int8_t low = EEPROM.read(3);
  u_int8_t high = EEPROM.read(4);
  game.score = (u_int16_t)low | (high << 8);

  game.board_width = EEPROM.read(5);
  game.board_height = EEPROM.read(6);
  selected_gem_idx = EEPROM.read(7);
  second_selected_idx = EEPROM.read(8);
  is_horizontal = (bool)EEPROM.read(9);

  if (game.gems)
    free(game.gems);
  game.gems = (Gem *)calloc(game.board_width * game.board_height, sizeof(Gem));

  int total = game.board_width * game.board_height;
  for (int i = 0; i < total; i++)
    game.gems[i] = decode_gem(EEPROM.read(10 + i));

  EEPROM.end();

  update_selection();
  handle_matches(false);
}

void draw_option_screen(void)
{
  const u_int8_t y_offsets[] = {Y_OFFSET, Y_OFFSET + 20, Y_OFFSET + 40, Y_OFFSET + 60};

  for (int i = 0; i < NUM_OPTIONS; i++)
  {
    M5.Lcd.setCursor(X_OFFSET, y_offsets[i]);
    u_int16_t color = (i == selected_option) ? RED : WHITE;
    M5.Lcd.setTextColor(color);
    M5.Lcd.printf("%s\n", OPTIONS[i]);
  }
}

// function to choose which option is exectuted
void execute_menu_option(void)
{
  switch (selected_option)
  {
  case 0: // back
    options_showing = false;
    break;
  case 1: // save
    save_to_eeprom();
    break;
  case 2: // load
    load_from_eeprom();
    break;
  case 3: // reset level
    game.score = 0;
    init_game(&game, game.level);
    selected_gem_idx = 0;
    second_selected_idx = 1;
    is_horizontal = true;
    update_selection();
    options_showing = false;
    break;
  }
}

// tinkered a little bit with some colors and layouts
// https://docs.m5stack.com/en/arduino/m5gfx/m5gfx_text for the "drawCenterString" function
void draw_start_screen()
{
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.drawCenterString("BEJEWELED", M5.Lcd.width() / 2, 40);

  M5.Lcd.setTextSize(1);
  M5.Lcd.setCursor(M5.Lcd.width() / 2 - 60, 100);

  if (selected_variant == 0)
    M5.Lcd.setTextColor(YELLOW);
  else
    M5.Lcd.setTextColor(WHITE);
  M5.Lcd.println("FREE SWAPS");

  M5.Lcd.setCursor(M5.Lcd.width() / 2 - 60, 130);
  if (selected_variant == 1)
    M5.Lcd.setTextColor(YELLOW);
  else
    M5.Lcd.setTextColor(WHITE);
  M5.Lcd.println("ONLY VALID SWAPS");

  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.drawCenterString("A to toggle", M5.Lcd.width() / 2, 200);
  M5.Lcd.drawCenterString("B to start", M5.Lcd.width() / 2, 220);
}

// main event lop
void loop()
{
  M5.update();
  delay(150);

  if (!game_started)
  {
    if (M5.BtnA.wasPressed())
      selected_variant = (selected_variant == 0) ? 1 : 0;

    if (M5.BtnB.wasPressed())
    {
      game.version = selected_variant;
      init_game(&game, 1);
      board_ready = true;
      game_started = true;
      M5.Lcd.fillScreen(BLACK);
    }

    draw_start_screen();
    return;
  }

  if (!board_ready)
    return;

  // Toggle options screen with A+B
  if (M5.BtnB.wasPressed() && M5.BtnA.wasPressed())
  {
    options_showing = !options_showing;
    if (!options_showing)
      selected_option = 0;
  }

  if (options_showing)
  {
    if (M5.BtnA.wasPressed())
      selected_option = (selected_option + 1) % NUM_OPTIONS;

    if (M5.BtnB.wasClicked())
      execute_menu_option();

    M5.Lcd.clear();
    draw_option_screen();
    return;
  }

  if (M5.BtnA.wasPressed())
    rotate_cursor();

  if (M5.BtnB.wasPressed())
    swap_gems();

  M5.Imu.getAccelData(&acc_x, &acc_y, &acc_z);
  move_selection(acc_x, acc_y);

  // Draw
  M5.Lcd.clear();
  draw_board();
  display_gamestate();
}
