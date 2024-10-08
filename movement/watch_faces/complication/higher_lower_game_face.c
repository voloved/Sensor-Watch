/*
 * MIT License
 *
 * Copyright (c) 2023 Chris Ellis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

// Emulator only: need time() to seed the random number generator.
#if __EMSCRIPTEN__
#include <time.h>
#endif

#include <stdlib.h>
#include <string.h>
#include "higher_lower_game_face.h"
#include "watch_private_display.h"

#define TITLE_TEXT "Hi-Lo"
#define GAME_BOARD_SIZE 6
#define GUESSES_PER_SCREEN 5
#define STATUS_DISPLAY_START 0
#define BOARD_SCORE_DISPLAY_START 2
#define BOARD_DISPLAY_START 4
#define BOARD_DISPLAY_END 9
#define MIN_CARD_VALUE 1
#define MAX_CARD_VALUE 12
#define DUPLICATES_OF_CARD 4
#define DECK_COUNT (DUPLICATES_OF_CARD * (MAX_CARD_VALUE - MIN_CARD_VALUE + 1))
#define FLIP_BOARD_DIRECTION false

#define KING   12
#define QUEEN  11
#define JACK   10

typedef struct card_t {
    uint8_t value;
    bool revealed;
} card_t;

typedef enum {
    A, B, C, D, E, F, G
} segment_t;

typedef enum {
    HL_GUESS_EQUAL,
    HL_GUESS_HIGHER,
    HL_GUESS_LOWER
} guess_t;

typedef enum {
    HL_GS_TITLE_SCREEN,
    HL_GS_GUESSING,
    HL_GS_WIN,
    HL_GS_LOSE,
    HL_GS_SHOW_SCORE,
} game_state_t;

static game_state_t game_state = HL_GS_TITLE_SCREEN;
static card_t game_board[GAME_BOARD_SIZE] = {0};
static uint8_t guess_position = 0;
static uint8_t score = 0;
static uint8_t completed_board_count = 0;
static uint8_t _deck[DECK_COUNT];
static uint8_t _curr_card;

static uint8_t generate_random_number(uint8_t num_values) {
    // Emulator: use rand. Hardware: use arc4random.
#if __EMSCRIPTEN__
    return rand() % num_values;
#else
    return arc4random_uniform(num_values);
#endif
}

static void stack_deck(uint8_t *array) {
    const uint8_t unique_cards = MAX_CARD_VALUE - MIN_CARD_VALUE + 1;
    for (uint8_t i = 0; i < unique_cards; i++)
    {
        for (uint8_t j = 0; j < DUPLICATES_OF_CARD; j++) 
            array[(i * DUPLICATES_OF_CARD) + j] = MIN_CARD_VALUE + i;
    }
}

static void shuffle_deck(uint8_t *array, uint8_t n) {
    // Randomize shuffle with Fisher Yates
    uint8_t i, j, tmp;
     for (i = n - 1; i > 0; i--) {
         j = generate_random_number(0xFF) % (i + 1);
         tmp = array[j];
         array[j] = array[i];
         array[i] = tmp;
     }
}

static void reset_board(bool first_round) {
    // First card is random on the first board, and carried over from the last position on subsequent boards
    const uint8_t first_card_value = first_round
                                     ? _deck[_curr_card++]
                                     : game_board[GAME_BOARD_SIZE - 1].value;

    game_board[0].value = first_card_value;
    game_board[0].revealed = true; // Always reveal first card

    // Fill remainder of board
    for (size_t i = 1; i < GAME_BOARD_SIZE; ++i) {
        game_board[i] = (card_t) {
                .value = _deck[_curr_card++],
                .revealed = false
        };
    }
}

static void init_game(void) {
    watch_clear_display();
    watch_display_string(TITLE_TEXT, BOARD_DISPLAY_START);
    watch_display_string("HL", STATUS_DISPLAY_START);
    _curr_card = 0;
    stack_deck(_deck);
    shuffle_deck(_deck, DECK_COUNT);
    reset_board(true);
    score = 0;
    completed_board_count = 0;
    guess_position = 1;
}

static void set_segment_at_position(segment_t segment, uint8_t position) {
    const uint64_t position_segment_data = (Segment_Map[position] >> (8 * (uint8_t) segment)) & 0xFF;
    const uint8_t com_pin = position_segment_data >> 6;
    const uint8_t seg = position_segment_data & 0x3F;
    watch_set_pixel(com_pin, seg);
}

static inline size_t get_display_position(size_t board_position) {
    return FLIP_BOARD_DIRECTION ? BOARD_DISPLAY_START + board_position : BOARD_DISPLAY_END - board_position;
}

static void render_board_position(size_t board_position) {
    const size_t display_position = get_display_position(board_position);
    const bool revealed = game_board[board_position].revealed;
    if (!revealed) {
        // Higher or lower indicator (currently just an empty space)
        watch_display_character(' ', display_position);
        return;
    }

    const uint8_t value = game_board[board_position].value;
    switch (value) {
        case KING: // K (≡)
            watch_display_character(' ', display_position);
            set_segment_at_position(A, display_position);
            set_segment_at_position(D, display_position);
            set_segment_at_position(G, display_position);
            break;
        case QUEEN: // Q (=)
            watch_display_character(' ', display_position);
            set_segment_at_position(A, display_position);
            set_segment_at_position(D, display_position);
            break;
        case JACK: // J (-)
            watch_display_character('-', display_position);
            break;
        default: {
            const char display_char = value + '0';
            watch_display_character(display_char, display_position);
        }
    }
}

static void render_board() {
    for (size_t i = 0; i < GAME_BOARD_SIZE; ++i) {
        render_board_position(i);
    }
}

static void render_board_count(uint8_t display_score) {
    char buf[3] = {0};
    snprintf(buf, sizeof(buf), "%2hhu", display_score);
    if ((score / 10) == 4) buf[0] = 'w';
    watch_display_string(buf, BOARD_SCORE_DISPLAY_START);
}

static void render_final_score(void) {
    watch_display_string("SC", STATUS_DISPLAY_START);
    char buf[7] = {0};
    const uint8_t complete_boards = score / GUESSES_PER_SCREEN;
    snprintf(buf, sizeof(buf), "%2hu %03hu", complete_boards, score);
    watch_set_colon();
    watch_display_string(buf, BOARD_DISPLAY_START);
}

static guess_t get_answer() {
    if (guess_position < 1 || guess_position > GAME_BOARD_SIZE)
        return HL_GUESS_EQUAL; // Maybe add an error state, shouldn't ever hit this.

    game_board[guess_position].revealed = true;
    const uint8_t previous_value = game_board[guess_position - 1].value;
    const uint8_t current_value = game_board[guess_position].value;

    if (current_value > previous_value)
        return HL_GUESS_HIGHER;
    else if (current_value < previous_value)
        return HL_GUESS_LOWER;
    else
        return HL_GUESS_EQUAL;
}

static void show_title_screen(void) {
    watch_clear_display();
    watch_display_string(TITLE_TEXT, BOARD_DISPLAY_START);
    watch_display_string("HL", STATUS_DISPLAY_START);
    game_state = HL_GS_TITLE_SCREEN;
}

static void do_game_loop(guess_t user_guess) {
    switch (game_state) {
        case HL_GS_TITLE_SCREEN:
            init_game();
            render_board();
            render_board_count(score);
            game_state = HL_GS_GUESSING;
            break;
        case HL_GS_GUESSING: {
            const guess_t answer = get_answer();

            // Render answer indicator
            switch (answer) {
                case HL_GUESS_EQUAL:
                    watch_display_string("==", STATUS_DISPLAY_START);
                    break;
                case HL_GUESS_HIGHER:
                    watch_display_string("HI", STATUS_DISPLAY_START);
                    break;
                case HL_GUESS_LOWER:
                    watch_display_string("LO", STATUS_DISPLAY_START);
                    break;
            }

            // Scoring
            if (answer == user_guess || answer == HL_GUESS_EQUAL) {
                score++;
            } else {
                // Incorrect guess, game over
                watch_display_string(" L", STATUS_DISPLAY_START);
                game_board[guess_position].revealed = true;
                watch_display_string("------", BOARD_DISPLAY_START);
                render_board_position(guess_position - 1);
                render_board_position(guess_position);
                if (game_board[guess_position].value == JACK && guess_position < GAME_BOARD_SIZE) // Adds a space in case the revealed option is '-'
                    watch_display_character(' ', get_display_position(guess_position + 1));
                game_state = HL_GS_LOSE;
                return;
            }

            if (score >= DECK_COUNT) {
                // Win, perhaps some kind of animation sequence?
                watch_display_string("WI", STATUS_DISPLAY_START);
                watch_display_string("  ", BOARD_SCORE_DISPLAY_START);
                watch_display_string("winnEr", BOARD_DISPLAY_START);
                game_state = HL_GS_WIN;
                return;
            }

            // Next guess position
            const bool final_board_guess = guess_position == GAME_BOARD_SIZE - 1;
            if (final_board_guess) {
                // Seed new board
                completed_board_count++;
                guess_position = 1;
                reset_board(false);
                render_board();
            } else {
                guess_position++;
                render_board_position(guess_position - 1);
                render_board_position(guess_position);
            }
            render_board_count(score);
            break;
        }
        case HL_GS_WIN:
        case HL_GS_LOSE:
            // Show score screen on button press from either state
            watch_clear_display();
            render_final_score();
            game_state = HL_GS_SHOW_SCORE;
            break;
        case HL_GS_SHOW_SCORE:
            show_title_screen();
            break;
        default:
            watch_display_string("ERROR", BOARD_DISPLAY_START);
            break;
    }
}

static void light_button_handler(void) {
    do_game_loop(HL_GUESS_HIGHER);
}

static void alarm_button_handler(void) {
    do_game_loop(HL_GUESS_LOWER);
}

void higher_lower_game_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void **context_ptr) {
    (void) settings;
    (void) watch_face_index;

    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(higher_lower_game_face_state_t));
        memset(*context_ptr, 0, sizeof(higher_lower_game_face_state_t));
        // Do any one-time tasks in here; the inside of this conditional happens only at boot.
        memset(game_board, 0, sizeof(game_board));
    }
    // Do any pin or peripheral setup here; this will be called whenever the watch wakes from deep sleep.
}

void higher_lower_game_face_activate(movement_settings_t *settings, void *context) {
    (void) settings;
    higher_lower_game_face_state_t *state = (higher_lower_game_face_state_t *) context;
    (void) state;
    // Handle any tasks related to your watch face coming on screen.
}

bool higher_lower_game_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    higher_lower_game_face_state_t *state = (higher_lower_game_face_state_t *) context;
    (void) state;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            show_title_screen();
            break;
        case EVENT_TICK:
            // If needed, update your display here.
            break;
        case EVENT_LIGHT_BUTTON_UP:
            light_button_handler();
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            // Don't trigger light
            break;
        case EVENT_ALARM_BUTTON_UP:
            alarm_button_handler();
            break;
        case EVENT_TIMEOUT:
            if (game_state > HL_GS_GUESSING)
                show_title_screen();
            else
                watch_display_string("HL", STATUS_DISPLAY_START);
            break;
        default:
            return movement_default_loop_handler(event, settings);
    }

    // return true if the watch can enter standby mode. Generally speaking, you should always return true.
    // Exceptions:
    //  * If you are displaying a color using the low-level watch_set_led_color function, you should return false.
    //  * If you are sounding the buzzer using the low-level watch_set_buzzer_on function, you should return false.
    // Note that if you are driving the LED or buzzer using Movement functions like movement_illuminate_led or
    // movement_play_alarm, you can still return true. This guidance only applies to the low-level watch_ functions.
    return true;
}

void higher_lower_game_face_resign(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;

    // handle any cleanup before your watch face goes off-screen.
}