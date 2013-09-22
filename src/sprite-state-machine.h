/*
 * Copyright (C) 2012 by Joseph A. Marrero.  http://www.manvscode.com/
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef _SPRITE_STATE_MACHINE_H_
#define _SPRITE_STATE_MACHINE_H_

#include <stddef.h>
#include <stdlib.h>

typedef void (*sprite_render_fxn) ( const sprite_frame_t* frame );


typedef struct sprite_state_machine {
	const sprite_t* sprite;
	const sprite_state_t* initial_state;
	const sprite_state_t* state;
	uint16_t frame_index;
	uint16_t loop_count;
	uint32_t last_frame_time;
	sprite_render_fxn render;
	bool is_playing;
	void* user_data;
} sprite_state_machine_t;

void sprite_state_machine_initialize    ( sprite_state_machine_t* ssm, const sprite_t* sprite, const char* initial_state, sprite_render_fxn render );
void sprite_state_machine_set_user_data ( sprite_state_machine_t* ssm, const void* data );
void sprite_state_machine_transition    ( sprite_state_machine_t* ssm, const char* state );
bool sprite_state_machine_is_state      ( sprite_state_machine_t* ssm, const char* state );
void sprite_state_machine_cancel        ( sprite_state_machine_t* ssm );
void sprite_state_machine_render        ( sprite_state_machine_t* ssm );


#endif /* _SPRITE_STATE_MACHINE_H_ */
