#ifndef CONFIG_H_
#define CONFIG_H_

// In this file some global variables will be declared such as colors, standard gem measurements
#include <M5Unified.h>

#define X_OFFSET 9
#define Y_OFFSET 15

#define GEM_SIZE 17
#define GEM_VALUE 20

#define MIN_TILT 0.15
#define SPEED 3
#define MEM_SIZE 512

#define COLOR_BLACK M5.Lcd.color565(0, 0, 0)

#define COLOR_RED M5.Lcd.color565(255, 0, 0)
#define COLOR_GREEN M5.Lcd.color565(0, 255, 0)
#define COLOR_BLUE M5.Lcd.color565(0, 0, 255)
#define COLOR_YELLOW M5.Lcd.color565(255, 255, 0)
#define COLOR_ORANGE M5.Lcd.color565(255, 165, 0)
const u_int16_t COLORS[] = { COLOR_RED, COLOR_GREEN, COLOR_BLUE, COLOR_YELLOW, COLOR_ORANGE };

#define SELECTION_COLOR M5.Lcd.color565(255, 255, 255)
#define SELECTION_WIDTH 3

#define NUM_OPTIONS 4
const char* OPTIONS[] = { "BACK", "SAVE", "LOAD", "RESET LEVEL" };

/* Definition of Gem struct 
* type:         the color of the gem (RED, GREEN, BLUE, YELLOW, GREEN, ORANGE)
* x_lt:         x-coordinate of the left top
* y_lt:         y-coordinate of the left top
* is_selected:  has the user selected this gem? 
*/
typedef struct gem {
    u_int8_t type;
    u_int8_t x_lt;
    u_int8_t y_lt;
    bool is_selected;
} Gem;

/* Definition of Game struct 
* level: t      he current level
* moves_left:   how many moves the user has left (is reset upon level start)
* score:        points scored
* board_width:  width of the board
* board_height: height of the board
* gems:         the current state of the board
*/
typedef struct game {
    u_int8_t level;
    u_int8_t moves_left;
    u_int16_t score;
    u_int8_t board_width;
    u_int8_t board_height;
    Gem *gems;
} Game;

void draw_board(void);
void init_gems(void);
void init_game(Game*);
void display_gamestate(void);
void draw_thick_rectangle(u_int8_t, u_int8_t, u_int8_t , u_int8_t , u_int8_t , u_int16_t );

void draw_option_screen(void);

#endif /* CONFIG_H_ */