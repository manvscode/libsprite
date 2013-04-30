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
#ifndef _SPRITE_H_
#define _SPRITE_H_
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif 

struct sprite;
typedef struct sprite sprite_t;

struct sprite_state;
typedef struct sprite_state sprite_state_t;

typedef struct sprite_frame {
	uint16_t x;
	uint16_t y;
	uint16_t width;
	uint16_t height;
} sprite_frame_t;


sprite_t*             sprite_create_ex          ( const char* name, bool use_transparency );
void                  sprite_create             ( sprite_t* p_sprite, const char* name, bool use_transparency );
void                  sprite_destroy_ex         ( sprite_t** p_sprite );
void                  sprite_destroy            ( sprite_t* p_sprite );

bool                  sprite_add_state          ( sprite_t* p_sprite, const char* state );
bool                  sprite_add_frame          ( sprite_t* p_sprite, const char* state, uint16_t x, uint16_t y, uint16_t width, uint16_t height );
bool                  sprite_remove_state       ( sprite_t* p_sprite, const char* state );
bool                  sprite_remove_frame       ( sprite_t* p_sprite, const char* state, uint16_t index );

const char*           sprite_name               ( const sprite_t* p_sprite );
uint16_t              sprite_width              ( const sprite_t* p_sprite );
uint16_t              sprite_height             ( const sprite_t* p_sprite );
uint16_t              sprite_bytes_per_pixel    ( const sprite_t* p_sprite );
const void*           sprite_pixels             ( const sprite_t* p_sprite );
const sprite_state_t* sprite_state              ( const sprite_t* p_sprite, const char* state );
uint8_t               sprite_state_count        ( const sprite_t* p_sprite );
const char*           sprite_state_name         ( const sprite_state_t* p_state );
uint16_t              sprite_state_frame_count  ( const sprite_state_t* p_state );
const sprite_frame_t* sprite_state_frame        ( const sprite_state_t* p_state, uint16_t index );


bool sprite_load( sprite_t* p_sprite, const char* filename )
{
	return true;
}

bool sprite_save( sprite_t* p_sprite, const char* filename )
{
	return true;
}

#ifdef __cplusplus
}
#endif 
#endif /* _SPRITE_H_ */
