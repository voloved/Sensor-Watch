/*
 * MIT License
 *
 * Copyright (c) 2024 <David Volovskiy>
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

#ifndef FESTIVAL_SCHEDULE_FACE_H_
#define FESTIVAL_SCHEDULE_FACE_H_

#include "movement.h"


typedef enum FestivalStage {
    RANCH_ARENA = 0,
    SHERWOOD_COURT,
    TRIPOLEE,
    CAROUSEL_CLUB,
    OBSERVATORY,
    HONEYCOMB,
    STAGE_COUNT
} FestivalStage;

typedef enum FestivalGenre {
    BASS = 0,
    HOUSE,
    INDIE,
    POP,
    JAM,
    TRAP,
    RAP,
    SOUL,
    GENRE_COUNT
} FestivalGenre;

typedef struct {
    char artist[6];
    FestivalStage stage;
    watch_date_time start_time;
    watch_date_time end_time;
    FestivalGenre genre;
    uint8_t popularity;
} schedule_t;

#define NUM_ACTS 120
#define SHOW_EMPTY_STAGES false

typedef struct {
    // Anything you need to keep track of, put it here!
    FestivalStage curr_stage;
    uint8_t curr_act;
    bool cyc_fest_not_occ;
    
} festival_schedule_state_t;

void festival_schedule_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr);
void festival_schedule_face_activate(movement_settings_t *settings, void *context);
bool festival_schedule_face_loop(movement_event_t event, movement_settings_t *settings, void *context);
void festival_schedule_face_resign(movement_settings_t *settings, void *context);

#define festival_schedule_face ((const watch_face_t){ \
    festival_schedule_face_setup, \
    festival_schedule_face_activate, \
    festival_schedule_face_loop, \
    festival_schedule_face_resign, \
    NULL, \
})

#endif // FESTIVAL_SCHEDULE_FACE_H_

