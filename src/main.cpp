#include <M5Unified.h>
#include <stdlib.h>
#include <stdint.h>
#include "EEPROM.h"
#include "../lib/config.h"

#define INDEX(row, col) ((row) * game.board_width + (col))

static bool matches_buffer[MAX_BOARD_WIDTH * MAX_BOARD_HEIGHT];

// Global game state
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

void setup()
{
  M5.begin();
  M5.Imu.init();
  Serial.begin(115200);
  Serial.flush();
  M5.Lcd.fillScreen(BLACK);

  // RNG seed
  float sx, sy, sz;
  if (M5.Imu.getAccelData(&sx, &sy, &sz) == 0)
    srand(int(sx * 1000 + sy * 1000 + sz * 1000) & 0x7FFF);
  else
    srand(millis() & 0x7FFF);

  // set gems to NULL as safety measure
  game.gems = NULL;
}

void init_game(Game *g, uint8_t level)
{
  g->level = level;
  g->moves_left = 25;
  if (level == 1)
    g->score = 0;

  g->board_width = 7;
  g->board_height = 11 - level;
  if (g->board_height < 3)
    g->board_height = 3;

  if (g->gems)
  {
    free(g->gems);
    g->gems = NULL;
  }

  g->gems = (Gem *)calloc(g->board_width * g->board_height, sizeof(Gem));
  if (!g->gems)
  {
    Serial.println("calloc failed");
    while (true)
      delay(100);
  }

  // choose random number of colors per level (part of assignment)
  int max_color = 8;
  int min_color = 4;
  int range = max_color - min_color + 1;
  int chosen = min_color + (rand() % range);
  g->num_colors = (u_int8_t)chosen;

  init_gems();

  selected_gem_idx = 0;
  second_selected_idx = 1;
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

void draw_thick_rectangle(u_int8_t x_top, u_int8_t y_top, u_int8_t width, u_int8_t height, u_int8_t thickness, u_int16_t color)
{
  for (int i = 0; i < thickness; i++)
    M5.Lcd.drawRect(x_top + i, y_top + i, width - 2 * i, height - 2 * i, color);
}

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
// usage of accelerometer based on WPO
// millis() and other 'time related' code based on years of experience working with microcontrollers such as Arduino 
void move_selection(float ax, float ay)
{
  unsigned long now = millis();
  if (now - last_move_time < MOVE_DELAY)
    return;

  int row = selected_gem_idx / game.board_width;
  int col = selected_gem_idx % game.board_width;

  int new_row = row;
  int new_col = col;
  bool moved = false;

  if (ax > MIN_TILT)
  {
    if (col > 0)
    {
      new_col--;
      moved = true;
    }
  }
  else if (ax < -MIN_TILT)
  {
    int max_col = is_horizontal ? game.board_width - 2 : game.board_width - 1;
    if (col < max_col)
    {
      new_col++;
      moved = true;
    }
  }

  if (ay > MIN_TILT)
  {
    int max_row = is_horizontal ? game.board_height - 1 : game.board_height - 2;
    if (row < max_row)
    {
      new_row++;
      moved = true;
    }
  }
  else if (ay < -MIN_TILT)
  {
    if (row > 0)
    {
      new_row--;
      moved = true;
    }
  }

  if (!moved)
    return;

  selected_gem_idx = new_row * game.board_width + new_col;

  if (is_horizontal)
  {
    if (new_col == game.board_width - 1)
      second_selected_idx = selected_gem_idx - 1;
    else
      second_selected_idx = selected_gem_idx + 1;
  }
  else
  {
    if (new_row == game.board_height - 1)
      second_selected_idx = selected_gem_idx - game.board_width;
    else
      second_selected_idx = selected_gem_idx + game.board_width;
  }

  update_selection();
  last_move_time = now;
}

static void clear_marks(bool *marks)
{
  int total = game.board_width * game.board_height;
  int i;
  for (i = 0; i < total; i++)
    marks[i] = false;
}

static int idx_rc(int r, int c)
{
  return r * game.board_width + c;
}

// Returns true if any matches found; marks all matched cells in marks[]
bool find_matches(bool *marks)
{
  int W = game.board_width;
  int H = game.board_height;
  bool any = false;

  clear_marks(marks);

  // check for vertical matches
  {
    int r;
    for (r = 0; r < H; r++)
    {
      int c = 0;
      while (c < W)
      {
        uint8_t t = game.gems[idx_rc(r, c)].type;
        if (t == GEM_EMPTY)
        {
          c++;
          continue;
        }

        /* extend run */
        {
          int start = c;
          while ((c + 1) < W && game.gems[idx_rc(r, c + 1)].type == t)
            c++;

          if ((c - start + 1) >= 3)
          {
            int k;
            any = true;
            for (k = start; k <= c; k++)
              marks[idx_rc(r, k)] = true;
          }
        }

        c++;
      }
    }
  }

  // check for horizontal matches
  {
    int c;
    for (c = 0; c < W; c++)
    {
      int r = 0;
      while (r < H)
      {
        uint8_t t = game.gems[idx_rc(r, c)].type;
        if (t == GEM_EMPTY)
        {
          r++;
          continue;
        }

        /* extend run */
        {
          int start = r;
          while ((r + 1) < H && game.gems[idx_rc(r + 1, c)].type == t)
            r++;

          if ((r - start + 1) >= 3)
          {
            int k;
            any = true;
            for (k = start; k <= r; k++)
              marks[idx_rc(k, c)] = true;
          }
        }

        r++;
      }
    }
  }

  return any;
}

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

// move gems down after deletion of underlying gems
void move_gems_down(void)
{
  int width = game.board_width;
  int height = game.board_height;

  for (int col = 0; col < width; col++)
  {
    int write_r = height - 1;

    // move a row
    for (int row = height - 1; row >= 0; row--)
    {
      uint8_t t = game.gems[idx_rc(row, col)].type;
      if (t != GEM_EMPTY)
      {
        if (write_r != row)
          game.gems[idx_rc(write_r, col)].type = t;
        write_r--;
      }
    }

    // fill top cells
    while (write_r >= 0)
    {
      game.gems[idx_rc(write_r, col)].type = (uint8_t)(rand() % game.num_colors);
      write_r--;
    }
  }

  update_selection();
}

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

static void swap_types(int a, int b)
{
  uint8_t t = game.gems[a].type;
  game.gems[a].type = game.gems[b].type;
  game.gems[b].type = t;
}

void swap_gems(void)
{
  bool tmp[MAX_BOARD_WIDTH * MAX_BOARD_HEIGHT];

  swap_types(selected_gem_idx, second_selected_idx);
  game.moves_left--;

  if (game.version == 0)
  {
    handle_matches(true);
  }
  else
  {
    /* only valid swaps */
    if (find_matches(tmp))
    {
      handle_matches(true);
    }
    else
    {
      swap_types(selected_gem_idx, second_selected_idx);
      game.moves_left++;
    }
  }

  update_selection();
}

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

    // Optional: show init process
    M5.Lcd.clear();
    draw_board();
    delay(30);
  } while (has_matches);
}

// -----------------------------------------------------------------------------
// Level / Victory (kept from your structure)
// -----------------------------------------------------------------------------

void show_victory_screen()
{
  M5.Lcd.fillScreen(BLUE);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(3);
  M5.Lcd.drawCenterString("YOU WIN!", M5.Lcd.width() / 2, 80);

  M5.Lcd.setTextSize(1);
  M5.Lcd.drawCenterString(String("Final Score: ") + String(game.score), M5.Lcd.width() / 2, 150);
  M5.Lcd.drawCenterString("Press A+B to Restart", M5.Lcd.width() / 2, 180);

  while (true)
  {
    M5.update();
    if (M5.BtnA.wasPressed() && M5.BtnB.wasPressed())
    {
      game.score = 0;
      init_game(&game, 1);
      return;
    }
    delay(10);
  }
}

void check_level_up()
{
  int target_score = game.level * MATCHES_PER_LEVEL * GEM_VALUE;
  if (game.score < target_score)
    return;

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

// -----------------------------------------------------------------------------
// EEPROM (kept mostly as-is; match logic doesn’t depend on this)
// -----------------------------------------------------------------------------

u_int8_t encode_gem(Gem g)
{
  u_int8_t encoded = g.type;
  if (g.is_selected)
    encoded |= (1 << 7);
  return encoded;
}

Gem decode_gem(u_int8_t data)
{
  Gem g;
  g.is_selected = (data >> 7) & 1;
  g.type = data & 0x7F;
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
