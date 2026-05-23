/*******************************************************************
  Snake Game for MicroZed based MZ_APO board
  Hardware Bridge: Laptop Keyboard (WASD) via SSH
 *******************************************************************/

/*
#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h> 
#include <termios.h>

#include "mzapo_parlcd.h"
#include "mzapo_phys.h"
#include "mzapo_regs.h"
#include "serialize_lock.h"
#include "font_types.h" 

extern font_descriptor_t font_rom8x16;

#define BLACK  0x0000
#define WHITE  0xFFFF
#define RED    0xF800
#define GREEN  0x07E0
#define BLUE   0x001F
#define YELLOW 0xFFE0

#define TOP_BAR_HEIGHT 40
#define BOTTOM_BAR_HEIGHT 20
#define MAX_APPLES 20

unsigned char *parlcd_mem_base;
unsigned char *spiled_mem_base;

// --- GRAPHICS FUNCTIONS ---
void draw_rect(int x, int y, int w, int h, uint16_t color) {
    *(volatile uint16_t*)(parlcd_mem_base + PARLCD_REG_CMD_o) = 0x2a;
    *(volatile uint16_t*)(parlcd_mem_base + PARLCD_REG_DATA_o) = x >> 8;
    *(volatile uint16_t*)(parlcd_mem_base + PARLCD_REG_DATA_o) = x & 0xFF;
    *(volatile uint16_t*)(parlcd_mem_base + PARLCD_REG_DATA_o) = (x+w-1) >> 8;
    *(volatile uint16_t*)(parlcd_mem_base + PARLCD_REG_DATA_o) = (x+w-1) & 0xFF;

    *(volatile uint16_t*)(parlcd_mem_base + PARLCD_REG_CMD_o) = 0x2b;
    *(volatile uint16_t*)(parlcd_mem_base + PARLCD_REG_DATA_o) = y >> 8;
    *(volatile uint16_t*)(parlcd_mem_base + PARLCD_REG_DATA_o) = y & 0xFF;
    *(volatile uint16_t*)(parlcd_mem_base + PARLCD_REG_DATA_o) = (y+h-1) >> 8;
    *(volatile uint16_t*)(parlcd_mem_base + PARLCD_REG_DATA_o) = (y+h-1) & 0xFF;

    *(volatile uint16_t*)(parlcd_mem_base + PARLCD_REG_CMD_o) = 0x2c;
    for (int i = 0; i < w * h; i++) {
        *(volatile uint16_t*)(parlcd_mem_base + PARLCD_REG_DATA_o) = color;
    }
}

// Draws large text (scaled 2x)
void draw_char(int x, int y, char ch, uint16_t color) {
    if (ch < font_rom8x16.firstchar || ch >= font_rom8x16.firstchar + font_rom8x16.size) return;
    const uint16_t *bits = font_rom8x16.bits + (ch - font_rom8x16.firstchar) * font_rom8x16.height;
    for (int i = 0; i < font_rom8x16.height; i++) {
        uint16_t row = bits[i];
        for (int j = 0; j < font_rom8x16.maxwidth; j++) {
            if ((row >> (15 - j)) & 1) draw_rect(x + j*2, y + i*2, 2, 2, color); 
        }
    }
}

void draw_string(int x, int y, const char *str, uint16_t color) {
    while(*str) { draw_char(x, y, *str, color); x += (font_rom8x16.maxwidth * 2); str++; }
}

// Draws small text (scaled 1x) for the bottom UI and sub-menus
void draw_char_small(int x, int y, char ch, uint16_t color) {
    if (ch < font_rom8x16.firstchar || ch >= font_rom8x16.firstchar + font_rom8x16.size) return;
    const uint16_t *bits = font_rom8x16.bits + (ch - font_rom8x16.firstchar) * font_rom8x16.height;
    for (int i = 0; i < font_rom8x16.height; i++) {
        uint16_t row = bits[i];
        for (int j = 0; j < font_rom8x16.maxwidth; j++) {
            if ((row >> (15 - j)) & 1) draw_rect(x + j, y + i, 1, 1, color); 
        }
    }
}

void draw_string_small(int x, int y, const char *str, uint16_t color) {
    while(*str) { draw_char_small(x, y, *str, color); x += font_rom8x16.maxwidth; str++; }
}

// --- CINEMATIC SCREENS ---
void start_screen() {
    draw_rect(0, 0, 480, 320, BLACK);
    // Center 10 chars * 16px = 160px width. (480 - 160) / 2 = 160
    draw_string(160, 120, "SNAKE GAME", GREEN);
    // Center 25 chars * 8px = 200px width. (480 - 200) / 2 = 140
    draw_string_small(140, 180, "PRESS ANY BUTTON TO START", WHITE);

    while(1) {
        uint32_t knobs = *(volatile uint32_t*)(spiled_mem_base + SPILED_REG_KNOBS_8BIT_o);
        if ((knobs >> 24) & 7) break;
        usleep(50000);
    }
    usleep(300000); // Debounce
    draw_rect(0, 0, 480, 320, BLACK); 
}

void end_screen(int win) {
    uint32_t led_color = win ? 0x0000FF00 : 0x00FF0000; 
    draw_rect(0, 0, 480, 320, BLACK);
    
    if (win) {
        // "YOU WIN!" -> 8 chars * 16px = 128px. Center = 176
        draw_string(176, 140, "YOU WIN!", GREEN);
    } else {
        // "GAME OVER" -> 9 chars * 16px = 144px. Center = 168
        draw_string(168, 140, "GAME OVER", RED);
    }
    
    // "PRESS ANY BUTTON TO RESTART" -> 27 chars * 8px = 216px. Center = 132
    draw_string_small(132, 200, "PRESS ANY BUTTON TO RESTART", WHITE);
    
    // Flash RGB LEDs
    for(int i = 0; i < 5; i++) {
        *(volatile uint32_t*)(spiled_mem_base + SPILED_REG_LED_RGB1_o) = led_color;
        *(volatile uint32_t*)(spiled_mem_base + SPILED_REG_LED_RGB2_o) = led_color;
        *(volatile uint32_t*)(spiled_mem_base + SPILED_REG_LED_LINE_o) = 0xFFFFFFFF;
        usleep(150000);
        *(volatile uint32_t*)(spiled_mem_base + SPILED_REG_LED_RGB1_o) = 0;
        *(volatile uint32_t*)(spiled_mem_base + SPILED_REG_LED_RGB2_o) = 0;
        *(volatile uint32_t*)(spiled_mem_base + SPILED_REG_LED_LINE_o) = 0;
        usleep(150000);
    }

    while(1) {
        uint32_t knobs = *(volatile uint32_t*)(spiled_mem_base + SPILED_REG_KNOBS_8BIT_o);
        if ((knobs >> 24) & 7) break;
        usleep(50000);
    }
    usleep(300000);
}


int main(int argc, char *argv[]) {
    // Memory Mapping
    parlcd_mem_base = map_phys_address(PARLCD_REG_BASE_PHYS, PARLCD_REG_SIZE, 0);
    spiled_mem_base = map_phys_address(SPILED_REG_BASE_PHYS, SPILED_REG_SIZE, 0);
    if (!parlcd_mem_base || !spiled_mem_base) exit(1);
    
    parlcd_hx8357_init(parlcd_mem_base); 
    srand(time(NULL));

    // Keyboard Setup (Raw Mode)
    struct termios old_tio, new_tio;
    tcgetattr(STDIN_FILENO, &old_tio);
    new_tio = old_tio;
    new_tio.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);

    int first_start = 1;

    // --- OUTER LOOP: KEEPS RESTARTING GAME ---
    while(1) {
        if (first_start) {
            start_screen();
            first_start = 0;
        }

        int grid_size = 20;               
        int max_x = 480 / grid_size;      
        int max_y = (320 - TOP_BAR_HEIGHT - BOTTOM_BAR_HEIGHT) / grid_size; 

        int snake_x[200] = {10, 9, 8};
        int snake_y[200] = {10, 10, 10};
        int snake_len = 3;
        int dir_x = 1, dir_y = 0;         

        int apple_x[MAX_APPLES];
        int apple_y[MAX_APPLES];
        int apple_active[MAX_APPLES];
        for(int i=0; i<MAX_APPLES; i++) apple_active[i] = 0;
        
        int score = 0;
        int goal = 10;
        int speed_delay = 150000;
        
        int is_paused = 0;
        int was_paused = 0;
        int pause_was_pressed = 0;
        int blue_was_pressed = 0;
        
        int flash_timer = 0;
        int game_over = 0;
        int skip_end_screen = 0;
        int spawn_counter = 0;
        int afk_time = 0; 

        draw_rect(0, 0, 480, 320, BLACK);

        // --- MAIN GAME LOOP ---
        while (!game_over) {
            
            // --- READ KNOBS ---
            uint32_t knobs = *(volatile uint32_t*)(spiled_mem_base + SPILED_REG_KNOBS_8BIT_o);
            int red_knob = (knobs >> 16) & 0xFF;   
            int green_knob = (knobs >> 8) & 0xFF;  
            int blue_knob = (knobs >> 0) & 0xFF;
            
            int red_btn = (knobs >> 24) & 4;
            int green_btn = (knobs >> 24) & 2;
            int blue_btn = (knobs >> 24) & 1;

            // Mapping Knobs to Level 1 - 5
            int speed_lvl = (red_knob / 52) + 1; 
            if (speed_lvl > 5) speed_lvl = 5;
            speed_delay = 250000 - (speed_lvl * 40000); 

            int spawn_lvl = (blue_knob / 52) + 1;
            if (spawn_lvl > 5) spawn_lvl = 5;
            int spawn_threshold = 60 - (spawn_lvl * 10); 

            goal = 5 + (green_knob * 45 / 255);

            // Blue Button = Hard Reset instantly mid-game
            if (blue_btn && !blue_was_pressed) {
                game_over = 1;
                skip_end_screen = 1;
                break;
            }
            blue_was_pressed = blue_btn;

            // Red/Green Button = Pause
            if ((red_btn || green_btn) && !pause_was_pressed) {
                is_paused = !is_paused;
                was_paused = 1;
            }
            pause_was_pressed = (red_btn || green_btn);

            if (is_paused) {
                // "PAUSED" -> 6 chars * 16px = 96px. Center = 192.
                draw_string(192, 150, "PAUSED", RED);
                usleep(50000);
                continue; 
            } else if (was_paused) {
                // Erase exactly the 96x32 pixel block where "PAUSED" was
                draw_rect(192, 150, 96, 32, BLACK); 
                was_paused = 0;
                afk_time = 0; 
            }

            // --- READ KEYBOARD (WASD) ---
            struct pollfd fds;
            fds.fd = STDIN_FILENO; 
            fds.events = POLLIN;

            int dir_changed = 0;
            if (poll(&fds, 1, 0) > 0) {
                char c;
                if (read(STDIN_FILENO, &c, 1) > 0) {
                    if ((c == 'w' || c == 'W') && dir_y != 1)  { dir_x = 0; dir_y = -1; dir_changed = 1; }
                    if ((c == 's' || c == 'S') && dir_y != -1) { dir_x = 0; dir_y = 1; dir_changed = 1; }
                    if ((c == 'a' || c == 'A') && dir_x != 1)  { dir_x = -1; dir_y = 0; dir_changed = 1; }
                    if ((c == 'd' || c == 'D') && dir_x != -1) { dir_x = 1; dir_y = 0; dir_changed = 1; }
                }
            }

            // Reset AFK Timer if user is playing
            if (dir_changed) afk_time = 0;

            // --- CONSTANT APPLE SPAWNER ---
            spawn_counter++;
            if (spawn_counter >= spawn_threshold) {
                spawn_counter = 0;
                for(int i=0; i<MAX_APPLES; i++) {
                    if (!apple_active[i]) {
                        apple_x[i] = rand() % max_x;
                        apple_y[i] = rand() % max_y;
                        apple_active[i] = 1;
                        break; 
                    }
                }
            }

            // Erase old tail
            draw_rect(snake_x[snake_len - 1] * grid_size, (snake_y[snake_len - 1] * grid_size) + TOP_BAR_HEIGHT, grid_size, grid_size, BLACK);

            // Shift body
            for (int i = snake_len - 1; i > 0; i--) {
                snake_x[i] = snake_x[i - 1];
                snake_y[i] = snake_y[i - 1];
            }

            // Move head
            snake_x[0] += dir_x;
            snake_y[0] += dir_y;

            // Wall Wrap-around
            if (snake_x[0] >= max_x) snake_x[0] = 0;
            if (snake_x[0] < 0) snake_x[0] = max_x - 1;
            if (snake_y[0] >= max_y) snake_y[0] = 0;
            if (snake_y[0] < 0) snake_y[0] = max_y - 1;

            // Collision: Self
            for(int i = 1; i < snake_len; i++) {
                if(snake_x[0] == snake_x[i] && snake_y[0] == snake_y[i]) {
                    game_over = 1;
                }
            }

            // Collision: Apples
            for(int i=0; i<MAX_APPLES; i++) {
                if (apple_active[i] && snake_x[0] == apple_x[i] && snake_y[0] == apple_y[i]) {
                    if (snake_len < 199) snake_len++; 
                    score++;
                    apple_active[i] = 0; 
                    flash_timer = 2; 
                    break; 
                }
            }

            if (score >= goal) {
                game_over = 1;
            }

            // --- RENDER TOP SCORE ---
            draw_rect(0, 0, 480, TOP_BAR_HEIGHT, BLACK); 
            char score_text[30];
            sprintf(score_text, "SCORE: %d", score); 
            draw_string(5, 4, score_text, WHITE);

            // --- RENDER BOTTOM UI BAR (Small Text) ---
            draw_rect(0, 320 - BOTTOM_BAR_HEIGHT, 480, BOTTOM_BAR_HEIGHT, BLACK); 
            char bottom_text[60];
            sprintf(bottom_text, "SPEED: %d/5   SPAWN: %d/5   GOAL: %d", speed_lvl, spawn_lvl, goal);
            draw_string_small(5, 320 - BOTTOM_BAR_HEIGHT + 2, bottom_text, YELLOW);

            // --- RENDER GRAPHICS ---
            for (int i = 0; i < MAX_APPLES; i++) {
                if (apple_active[i]) {
                    draw_rect(apple_x[i] * grid_size, (apple_y[i] * grid_size) + TOP_BAR_HEIGHT, grid_size, grid_size, RED);
                }
            }
            for (int i = 0; i < snake_len; i++) {
                draw_rect(snake_x[i] * grid_size, (snake_y[i] * grid_size) + TOP_BAR_HEIGHT, grid_size, grid_size, GREEN);
            }

            // --- LED EFFECTS & PROGRESS BAR ---
            if (flash_timer > 0) {
                *(volatile uint32_t*)(spiled_mem_base + SPILED_REG_LED_RGB1_o) = 0x0000FF00; 
                *(volatile uint32_t*)(spiled_mem_base + SPILED_REG_LED_RGB2_o) = 0x0000FF00;
                flash_timer--;
            } else {
                *(volatile uint32_t*)(spiled_mem_base + SPILED_REG_LED_RGB1_o) = 0;
                *(volatile uint32_t*)(spiled_mem_base + SPILED_REG_LED_RGB2_o) = 0;
            }

            // Progress Bar (Left to Right Sweep)
            int progress_leds = (score * 32) / goal;
            if (progress_leds > 32) progress_leds = 32;
            
            uint32_t line_val = 0;
            for(int i = 0; i < progress_leds; i++) {
                line_val |= (1 << (31 - i)); 
            }
            *(volatile uint32_t*)(spiled_mem_base + SPILED_REG_LED_LINE_o) = line_val;

            // --- AFK PAUSE TRACKER ---
            afk_time += speed_delay;
            if (afk_time >= 10000000) { 
                is_paused = 1;
                was_paused = 1;
                afk_time = 0; 
            }

            usleep(speed_delay);
        }

        // --- END OF GAME ---
        draw_rect(0, 0, 480, 320, BLACK); 
        if (!skip_end_screen) {
            end_screen(score >= goal);
        }
    }
    
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
    return 0;
}
*/

/*******************************************************************
  Snake Game for MicroZed based MZ_APO board
  Hardware Bridge: ESP32 Joystick & Buzzer via /dev/ttyUSB0
 *******************************************************************/

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

#include "mzapo_parlcd.h"
#include "mzapo_phys.h"
#include "mzapo_regs.h"
#include "serialize_lock.h"
#include "font_types.h" 

extern font_descriptor_t font_rom8x16;

#define BLACK  0x0000
#define WHITE  0xFFFF
#define RED    0xF800
#define GREEN  0x07E0
#define BLUE   0x001F
#define YELLOW 0xFFE0

#define TOP_BAR_HEIGHT 40
#define BOTTOM_BAR_HEIGHT 20
#define MAX_APPLES 20

unsigned char *parlcd_mem_base;
unsigned char *spiled_mem_base;

// ==========================================
// GRAPHICS & HARDWARE INITIALIZATION
// ==========================================

// Pushes raw pixel data to the LCD screen bounds
void draw_rect(int x, int y, int w, int h, uint16_t color) {
    *(volatile uint16_t*)(parlcd_mem_base + PARLCD_REG_CMD_o) = 0x2a;
    *(volatile uint16_t*)(parlcd_mem_base + PARLCD_REG_DATA_o) = x >> 8;
    *(volatile uint16_t*)(parlcd_mem_base + PARLCD_REG_DATA_o) = x & 0xFF;
    *(volatile uint16_t*)(parlcd_mem_base + PARLCD_REG_DATA_o) = (x+w-1) >> 8;
    *(volatile uint16_t*)(parlcd_mem_base + PARLCD_REG_DATA_o) = (x+w-1) & 0xFF;

    *(volatile uint16_t*)(parlcd_mem_base + PARLCD_REG_CMD_o) = 0x2b;
    *(volatile uint16_t*)(parlcd_mem_base + PARLCD_REG_DATA_o) = y >> 8;
    *(volatile uint16_t*)(parlcd_mem_base + PARLCD_REG_DATA_o) = y & 0xFF;
    *(volatile uint16_t*)(parlcd_mem_base + PARLCD_REG_DATA_o) = (y+h-1) >> 8;
    *(volatile uint16_t*)(parlcd_mem_base + PARLCD_REG_DATA_o) = (y+h-1) & 0xFF;

    *(volatile uint16_t*)(parlcd_mem_base + PARLCD_REG_CMD_o) = 0x2c;
    for (int i = 0; i < w * h; i++) {
        *(volatile uint16_t*)(parlcd_mem_base + PARLCD_REG_DATA_o) = color;
    }
}

// Draws large text (scaled 2x for titles)
void draw_char(int x, int y, char ch, uint16_t color) {
    if (ch < font_rom8x16.firstchar || ch >= font_rom8x16.firstchar + font_rom8x16.size) return;
    const uint16_t *bits = font_rom8x16.bits + (ch - font_rom8x16.firstchar) * font_rom8x16.height;
    for (int i = 0; i < font_rom8x16.height; i++) {
        uint16_t row = bits[i];
        for (int j = 0; j < font_rom8x16.maxwidth; j++) {
            if ((row >> (15 - j)) & 1) draw_rect(x + j*2, y + i*2, 2, 2, color); 
            else draw_rect(x + j*2, y + i*2, 2, 2, BLACK);
        }
    }
}

void draw_string(int x, int y, const char *str, uint16_t color) {
    while(*str) { draw_char(x, y, *str, color); x += (font_rom8x16.maxwidth * 2); str++; }
}

// Draws standard text (scaled 1x for UI elements)
void draw_char_small(int x, int y, char ch, uint16_t color) {
    if (ch < font_rom8x16.firstchar || ch >= font_rom8x16.firstchar + font_rom8x16.size) return;
    const uint16_t *bits = font_rom8x16.bits + (ch - font_rom8x16.firstchar) * font_rom8x16.height;
    for (int i = 0; i < font_rom8x16.height; i++) {
        uint16_t row = bits[i];
        for (int j = 0; j < font_rom8x16.maxwidth; j++) {
            if ((row >> (15 - j)) & 1) draw_rect(x + j, y + i, 1, 1, color); 
            else draw_rect(x + j, y + i, 1, 1, BLACK);
        }
    }
}

void draw_string_small(int x, int y, const char *str, uint16_t color) {
    while(*str) { draw_char_small(x, y, *str, color); x += font_rom8x16.maxwidth; str++; }
}

// Opens the USB port to the ESP32 in "Non-Blocking" mode so the game loop doesn't freeze
int init_serial(const char *device) {
    int fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) return -1;
    struct termios options;
    tcgetattr(fd, &options);
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);
    options.c_cflag &= ~(CSIZE | PARENB);
    options.c_cflag |= (CLOCAL | CREAD | CS8);
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG | ECHONL | ISIG | IEXTEN);
    options.c_oflag &= ~OPOST;
    options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    options.c_cc[VMIN]  = 0;
    options.c_cc[VTIME] = 0; 
    tcsetattr(fd, TCSANOW, &options);
    fcntl(fd, F_SETFL, O_NONBLOCK);
    return fd;
}

// ==========================================
// CINEMATIC SCREENS
// ==========================================

void start_screen() {
    draw_rect(0, 0, 480, 320, BLACK);
    draw_string(160, 120, "SNAKE GAME", GREEN); // Perfectly centered
    draw_string_small(140, 180, "PRESS ANY BUTTON TO START", WHITE);

    // Trap the code here until any physical knob button is clicked
    while(1) {
        uint32_t knobs = *(volatile uint32_t*)(spiled_mem_base + SPILED_REG_KNOBS_8BIT_o);
        if ((knobs >> 24) & 7) break;
        usleep(50000);
    }
    usleep(300000); // Debounce to prevent double-clicks
    draw_rect(0, 0, 480, 320, BLACK); 
}

void end_screen(int win, int serial_fd) {
    uint32_t led_color = win ? 0x0000FF00 : 0x00FF0000; 
    draw_rect(0, 0, 480, 320, BLACK);
    
    // Send Buzzer Trigger to ESP32
    if (serial_fd != -1) {
        if (win) write(serial_fd, "W", 1); // Triggers 3 happy beeps
        else write(serial_fd, "L", 1);     // Triggers 1 sad beep
    }

    if (win) draw_string(176, 140, "YOU WIN!", GREEN);
    else draw_string(168, 140, "GAME OVER", RED);
    
    draw_string_small(132, 200, "PRESS ANY BUTTON TO RESTART", WHITE);
    
    // Hardware LED Strobe effect
    for(int i = 0; i < 5; i++) {
        *(volatile uint32_t*)(spiled_mem_base + SPILED_REG_LED_RGB1_o) = led_color;
        *(volatile uint32_t*)(spiled_mem_base + SPILED_REG_LED_RGB2_o) = led_color;
        *(volatile uint32_t*)(spiled_mem_base + SPILED_REG_LED_LINE_o) = 0xFFFFFFFF;
        usleep(150000);
        *(volatile uint32_t*)(spiled_mem_base + SPILED_REG_LED_RGB1_o) = 0;
        *(volatile uint32_t*)(spiled_mem_base + SPILED_REG_LED_RGB2_o) = 0;
        *(volatile uint32_t*)(spiled_mem_base + SPILED_REG_LED_LINE_o) = 0;
        usleep(150000);
    }

    // Wait for restart click
    while(1) {
        uint32_t knobs = *(volatile uint32_t*)(spiled_mem_base + SPILED_REG_KNOBS_8BIT_o);
        if ((knobs >> 24) & 7) break;
        usleep(50000);
    }
    usleep(300000);
}


// ==========================================
// CORE GAME ENGINE
// ==========================================

int main(int argc, char *argv[]) {
    // Acquire Physical Memory Maps
    parlcd_mem_base = map_phys_address(PARLCD_REG_BASE_PHYS, PARLCD_REG_SIZE, 0);
    spiled_mem_base = map_phys_address(SPILED_REG_BASE_PHYS, SPILED_REG_SIZE, 0);
    if (!parlcd_mem_base || !spiled_mem_base) exit(1);
    
    parlcd_hx8357_init(parlcd_mem_base); // Boot the screen
    srand(time(NULL));

    // Connect to ESP32
    int serial_fd = init_serial("/dev/ttyUSB0");

    int first_start = 1;

    // --- INFINITE RESTART LOOP ---
    while(1) {
        if (first_start) {
            start_screen();
            first_start = 0;
        }

        // Grid Math
        int grid_size = 20;               
        int max_x = 480 / grid_size;      
        int max_y = (320 - TOP_BAR_HEIGHT - BOTTOM_BAR_HEIGHT) / grid_size; 

        // Snake Arrays
        int snake_x[200] = {10, 9, 8};
        int snake_y[200] = {10, 10, 10};
        int snake_len = 3;
        int dir_x = 1, dir_y = 0;         

        // Apple Arrays
        int apple_x[MAX_APPLES];
        int apple_y[MAX_APPLES];
        int apple_active[MAX_APPLES];
        for(int i=0; i<MAX_APPLES; i++) apple_active[i] = 0;
        
        // Game State Variables
        int score = 0;
        int goal = 10;
        int speed_delay = 150000;
        int is_paused = 0, was_paused = 0, pause_was_pressed = 0, blue_was_pressed = 0;
        int flash_timer = 0, game_over = 0, skip_end_screen = 0, spawn_counter = 0, afk_time = 0; 

        draw_rect(0, 0, 480, 320, BLACK);

        // --- ACTIVE FRAME LOOP ---
        while (!game_over) {
            
            // 1. Process Hardware Knobs (Speed, Goal, Spawn Rate)
            uint32_t knobs = *(volatile uint32_t*)(spiled_mem_base + SPILED_REG_KNOBS_8BIT_o);
            int red_knob = (knobs >> 16) & 0xFF;   
            int green_knob = (knobs >> 8) & 0xFF;  
            int blue_knob = (knobs >> 0) & 0xFF;
            
            int red_btn = (knobs >> 24) & 4;
            int green_btn = (knobs >> 24) & 2;
            int blue_btn = (knobs >> 24) & 1;

            // Map Red Knob to 5 discrete Speed Levels
            int speed_lvl = (red_knob / 52) + 1; 
            if (speed_lvl > 5) speed_lvl = 5;
            speed_delay = 250000 - (speed_lvl * 40000); 

            // Map Blue Knob to 5 discrete Spawn Rate Levels
            int spawn_lvl = (blue_knob / 52) + 1;
            if (spawn_lvl > 5) spawn_lvl = 5;
            int spawn_threshold = 60 - (spawn_lvl * 10); 

            // Map Green Knob to win Goal
            goal = 5 + (green_knob * 45 / 255);

            // Blue Button = Hard Reset (Instant Death without screen)
            if (blue_btn && !blue_was_pressed) {
                game_over = 1;
                skip_end_screen = 1;
                break;
            }
            blue_was_pressed = blue_btn;

            // Red/Green Button = Toggle Pause
            if ((red_btn || green_btn) && !pause_was_pressed) {
                is_paused = !is_paused;
                was_paused = 1;
            }
            pause_was_pressed = (red_btn || green_btn);

            if (is_paused) {
                draw_string(192, 150, "PAUSED", RED);
                usleep(50000);
                continue; // Freeze frame execution
            } else if (was_paused) {
                draw_rect(192, 150, 96, 32, BLACK); // Wipe pause text
                was_paused = 0;
                afk_time = 0; 
            }

            // 2. Read ESP32 Serial Joystick
            int dir_changed = 0;
            if (serial_fd != -1) {
                char buf;
                char latest_cmd = 0;
                
                // Flush buffer to prevent input lag, grab only the freshest char
                while (read(serial_fd, &buf, 1) > 0) {
                    if (buf == 'U' || buf == 'D' || buf == 'L' || buf == 'R') latest_cmd = buf; 
                }
                
                if (latest_cmd != 0) { // joystick
                    if (latest_cmd == 'U' && dir_y != 1)  { dir_x = 0; dir_y = -1; dir_changed = 1; }
                    if (latest_cmd == 'D' && dir_y != -1) { dir_x = 0; dir_y = 1; dir_changed = 1; }
                    if (latest_cmd == 'L' && dir_x != 1)  { dir_x = -1; dir_y = 0; dir_changed = 1; }
                    if (latest_cmd == 'R' && dir_x != -1) { dir_x = 1; dir_y = 0; dir_changed = 1; }
                }
            }

            if (dir_changed) afk_time = 0; // Reset 10-second AFK timer on movement

            // 3. Apple Spawner Logic
            spawn_counter++;
            if (spawn_counter >= spawn_threshold) {
                spawn_counter = 0;
                for(int i=0; i<MAX_APPLES; i++) {
                    if (!apple_active[i]) {
                        apple_x[i] = rand() % max_x;
                        apple_y[i] = rand() % max_y;
                        apple_active[i] = 1; // Activate apple
                        break; 
                    }
                }
            }

            // 4. Movement Array Shifting
            draw_rect(snake_x[snake_len - 1] * grid_size, (snake_y[snake_len - 1] * grid_size) + TOP_BAR_HEIGHT, grid_size, grid_size, BLACK);
            for (int i = snake_len - 1; i > 0; i--) {
                snake_x[i] = snake_x[i - 1];
                snake_y[i] = snake_y[i - 1];
            }
            snake_x[0] += dir_x;
            snake_y[0] += dir_y;

            // Wall Wrap-around
            if (snake_x[0] >= max_x) snake_x[0] = 0;
            if (snake_x[0] < 0) snake_x[0] = max_x - 1;
            if (snake_y[0] >= max_y) snake_y[0] = 0;
            if (snake_y[0] < 0) snake_y[0] = max_y - 1;

            // 5. Collision Checks
            // Hit self = Death
            for(int i = 1; i < snake_len; i++) {
                if(snake_x[0] == snake_x[i] && snake_y[0] == snake_y[i]) game_over = 1;
            }

            // Hit Apple = Grow
            for(int i=0; i<MAX_APPLES; i++) {
                if (apple_active[i] && snake_x[0] == apple_x[i] && snake_y[0] == apple_y[i]) {
                    if (snake_len < 199) snake_len++; 
                    score++;
                    apple_active[i] = 0; // Delete apple
                    flash_timer = 2; // Trigger RGB LEDs
                    
                    // --- BEAM AUDIO COMMAND TO ESP32 ---
                    if (serial_fd != -1) write(serial_fd, "A", 1); 
                    
                    break; 
                }
            }

            if (score >= goal) game_over = 1;

            // 6. Draw User Interface
            //draw_rect(0, 0, 480, TOP_BAR_HEIGHT, BLACK); 
            char score_text[30];
            sprintf(score_text, "SCORE: %d", score); 
            draw_string(5, 4, score_text, WHITE);

            //draw_rect(0, 320 - BOTTOM_BAR_HEIGHT, 480, BOTTOM_BAR_HEIGHT, BLACK); 
            char bottom_text[60];
            sprintf(bottom_text, "SPEED: %d/5   SPAWN: %d/5   GOAL: %d", speed_lvl, spawn_lvl, goal);
            draw_string_small(5, 320 - BOTTOM_BAR_HEIGHT + 2, bottom_text, YELLOW);

            // 7. Draw Gameplay grid
            for (int i = 0; i < MAX_APPLES; i++) {
                if (apple_active[i]) draw_rect(apple_x[i] * grid_size, (apple_y[i] * grid_size) + TOP_BAR_HEIGHT, grid_size, grid_size, RED);
            }
            for (int i = 0; i < snake_len; i++) {
                draw_rect(snake_x[i] * grid_size, (snake_y[i] * grid_size) + TOP_BAR_HEIGHT, grid_size, grid_size, GREEN);
            }

            // 8. Push LED Register Data
            if (flash_timer > 0) {
                *(volatile uint32_t*)(spiled_mem_base + SPILED_REG_LED_RGB1_o) = 0x0000FF00; 
                *(volatile uint32_t*)(spiled_mem_base + SPILED_REG_LED_RGB2_o) = 0x0000FF00;
                flash_timer--;
            } else {
                *(volatile uint32_t*)(spiled_mem_base + SPILED_REG_LED_RGB1_o) = 0;
                *(volatile uint32_t*)(spiled_mem_base + SPILED_REG_LED_RGB2_o) = 0;
            }

            // Calculate overall progress across the 32 LED line
            int progress_leds = (score * 32) / goal;
            if (progress_leds > 32) progress_leds = 32;
            
            uint32_t line_val = 0;
            for(int i = 0; i < progress_leds; i++) {
                line_val |= (1 << (31 - i)); // Shift bit to build array from Left to Right
            }
            *(volatile uint32_t*)(spiled_mem_base + SPILED_REG_LED_LINE_o) = line_val;

            // 9. Frame Control & AFK
            afk_time += speed_delay;
            if (afk_time >= 10000000) { // 10,000,000 us = 10 Seconds of no input
                is_paused = 1;
                was_paused = 1;
                afk_time = 0; 
            }

            usleep(speed_delay); // Hold frame rate
        }

        // --- END OF GAME SEQUENCE ---
        draw_rect(0, 0, 480, 320, BLACK); 
        if (!skip_end_screen) {
            end_screen(score >= goal, serial_fd);
        }
    }
    
    return 0;
}