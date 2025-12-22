#ifndef CONFIG_H_
#define CONFIG_H_

// In this file some global variables will be declared such as colors, standard gem measurements
#include <M5Unified.h>

#define X_OFFSET 9
#define Y_OFFSET 15

#define GEM_SIZE 17
#define GEM_VALUE 10
#define MATCHES_PER_LEVEL 10

#define MAX_BOARD_WIDTH 8
#define MAX_BOARD_HEIGHT 11

// these values are taken from/based on the WPO where we worked with the M5SStick
#define MIN_TILT 0.15
#define MOVE_DELAY 200
#define SPEED 3
#define MEM_SIZE 512

#define COLOR_COUNT 8
const u_int16_t COLORS[] = {RED, GREEN, BLUE, YELLOW, ORANGE, PINK, PURPLE, CYAN};

// extra "type" for a gem that needs to be filled in after deletion
#define GEM_EMPTY 255
#define NUM_GEM_TYPES 5

#define SELECTION_COLOR M5.Lcd.color565(255, 255, 255)
#define SELECTION_WIDTH 3

#define NUM_OPTIONS 4
const char *OPTIONS[] = {"BACK", "SAVE", "LOAD", "RESET LEVEL"};

/* Definition of Gem struct
 * type:         the color of the gem
 * is_selected:  is this gem part of the cursor?
 */
typedef struct gem
{
    u_int8_t type;
    bool is_selected;
} Gem;

/* Definition of Game struct
 * num_colors:   number of colors on the board (must be random/variable)
 * version:      0 or 1: in one of the versions only valid swaps can be made, in the other they can all be done
 * level: t      he current level
 * moves_left:   how many moves the user has left (is reset upon level start)
 * score:        points scored
 * board_width:  width of the board
 * board_height: height of the board
 * gems:         the current state of the board
 */
typedef struct game
{
    u_int8_t num_colors;
    u_int8_t version;
    u_int8_t level;
    u_int8_t moves_left;
    u_int16_t score;
    u_int8_t board_width;
    u_int8_t board_height;
    Gem *gems;
} Game;

void draw_board(void);
void init_gems(void);
void init_game(Game *g, u_int8_t level);
void display_gamestate(void);
void draw_thick_rectangle(u_int8_t, u_int8_t, u_int8_t, u_int8_t, u_int8_t, u_int16_t);

void draw_option_screen(void);
u_int8_t get_gem_x(int index);
u_int8_t get_gem_y(int index);

void update_selection(void);
void rotate_cursor(void);
void swap_gems(void);

bool find_matches(bool *matches);

void remove_matches(bool *matches, bool is_user_move);
void move_gems_down(void);
void handle_matches(bool is_user_move);

void check_level_up(void);
void show_victory_screen(void);

void execute_menu_option(void);

void save_to_eeprom(void);
void load_from_eeprom(void);

u_int8_t encode_gem(Gem g);
Gem decode_gem(u_int8_t encoded);

void move_selection(float acc_x, float acc_y);
void draw_start_screen(void);

static void clear_marks(bool *marks);
static int idx_rc(int r, int c);

#endif /* CONFIG_H_ */