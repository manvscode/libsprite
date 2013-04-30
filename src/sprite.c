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
#include <stdlib.h>
#include "sprite.h"

struct sprite_state {
	uint16_t        name_length;
	char*           name;
	uint16_t        fps;
	uint16_t        count;
	sprite_frame_t* frames;
};

struct sprite {
	uint16_t name_length;
	char*    name;
	uint16_t width;
	uint16_t height;
	uint8_t  bytes_per_pixel;
	uint8_t* pixels;

	uint8_t         state_count;
	sprite_state_t* states;
};


sprite_t* sprite_create_ex( const char* name, bool use_transparency )
{
	sprite_t* p_sprite = malloc( sizeof(sprite_t) );

	if( p_sprite )
	{
		sprite_create( p_sprite, name, use_transparency );
	}

	return p_sprite;
}

void sprite_create( sprite_t* p_sprite, const char* name, bool use_transparency )
{
	assert( p_sprite );
	p_sprite->name            = strdup( name );
	p_sprite->name_length     = strlen( name );
	p_sprite->width           = 0;
	p_sprite->height          = 0;
	p_sprite->bytes_per_pixel = use_transparency ? 4 : 3;
	p_sprite->pixels          = NULL;
	p_sprite->states          = NULL;
	p_sprite->state_count     = 0;
}

void sprite_destroy_ex( sprite_t** p_sprite )
{
	if( *p_sprite )
	{
		sprite_destroy( *p_sprite );

		free( *p_sprite );
		*p_sprite = NULL;
	}
}

void sprite_destroy( sprite_t* p_sprite )
{
	assert( p_sprite );

	if( p_sprite->name )
	{
		free( p_sprite->name );
		#ifdef SPRITE_DEBUG
		p_sprite->name        = NULL;
		p_sprite->name_length = 0;
		#endif
	}

	if( p_sprite->pixels )
	{
		free( p_sprite->pixels );
		#ifdef SPRITE_DEBUG
		p_sprite->width           = 0;
		p_sprite->height          = 0;
		p_sprite->bytes_per_pixel = 0;
		p_sprite->pixels          = NULL;
		#endif
	}

	if( p_sprite->states )
	{
		for( size_t i = 0; i < p_sprite->state_count; i++ )
		{
			sprite_state_t* p_state = p_sprite->states[ i ];

			if( p_state->name )
			{
				free( p_state->name );
				#ifdef SPRITE_DEBUG
				p_state->name_length = 0;
				p_state->name        = NULL;
				#endif
			}

			if( p_state->frames )
			{
				free( p_state->frames );
				#ifdef SPRITE_DEBUG
				p_state->fps    = 0;
				p_state->count  = 0;
				p_state->frames = NULL;
				#endif
			}

			free( p_state );
		}

		#ifdef SPRITE_DEBUG
		p_sprite->state_count = 0;
		#endif
	}
}

bool sprite_add_state( sprite_t* p_sprite, const char* name, uint16_t* index )
{
	bool result = false;

	if( p_sprite )
	{
		p_sprite->states = calloc( p_sprite->state_count + 1, sizeof(sprite_state_t) );

		if( p_sprite->states )
		{
			sprite_state_t* p_state = &p_sprite->states[ p_sprite->state_count ];

			p_state->name_length = strlen( name );
			p_state->name        = strdup( name );
			p_state->fps         = 60;
			p_state->count       = 0;
			p_state->frames      = NULL;

			*index = p_sprite->state_count;
			p_sprite->state_count++;

			result = true;
		}
	}

	return result;
}

bool sprite_add_frame( sprite_t* p_sprite, const char* state, uint16_t x, uint16_t y, uint16_t width, uint16_t height )
{
	bool result = false;

	if( p_sprite && p_sprite->states )
	{
		for( size_t i = 0; i < p_sprite->states_count; i++ )
		{
			sprite_state_t* p_state = &p_sprite->states[ i ];

			if( strncasecmp( state, p_state->name, p_state->name_length ) == 0 )
			{
				p_state->frames = calloc( p_state->count + 1, sizeof(sprite_frame_t) );

				if( p_state->frames )
				{
					sprite_frame_t* p_frame = &p_a->states[ p_state->count ];

					p_frame->x      = x;
					p_frame->y      = y;
					p_frame->width  = width;
					p_frame->height = height;

					p_state->count++;

					result = true;
				}

				break;
			}
		}
	}

	return result;
}

bool sprite_remove_state( sprite_t* p_sprite, const char* name )
{
	bool result = false;

	if( p_sprite && p_sprite->states )
	{
		for( uint16_t i = 0; i < p_sprite->states_count; i++ )
		{
			sprite_state_t* p_state = &p_sprite->states[ i ];

			if( strncasecmp( state, p_state->name, p_state->name_length ) == 0 )
			{
				free( p_state->name );
				free( p_state->frames );

				/* shift the collection down */
				for( uint16_t j = i; < p_sprite->states_count - 1; j++ )
				{
					p_sprite->states[ j ] = p_sprite->states[ j + 1 ];
				}

				p_sprite->states_count--;
				result = true;
			}
		}
	}

	return result;
}

bool sprite_remove_frame( sprite_t* p_sprite, const char* state, uint16_t index )
{
	bool result = false;

	if( p_sprite && p_sprite->states )
	{
		for( uint16_t i = 0; i < p_sprite->states_count; i++ )
		{
			sprite_state_t* p_state = &p_sprite->states[ i ];

			if( strncasecmp( state, p_state->name, p_state->name_length ) == 0 )
			{
				for( uint16_t j = index; j < p_state->count - 1; j++ )
				{
					p_state->frames[ j ] = p_state->frames[ j + 1 ];
					result = true;
				}

				break;
			}
		}
	}

	return result;
}

const char* sprite_name( const sprite_t* p_sprite )
{
	return p_sprite && p_sprite->name ? p_sprite->name : "<uninitialized sprite>";
}

uint16_t sprite_width( const sprite_t* p_sprite )
{
	return p_sprite ? p_sprite->width : 0;
}

uint16_t sprite_height( const sprite_t* p_sprite )
{
	return p_sprite ? p_sprite->height : 0;
}

uint16_t sprite_bytes_per_pixel( const sprite_t* p_sprite )
{
	return p_sprite ? p_sprite->bytes_per_pixel : 0;
}

const void* sprite_pixels( const sprite_t* p_sprite )
{
	return p_sprite ? p_sprite->pixels : NULL;
}

bool sprite_find_state( const sprite_t* p_sprite, const char* state, uint8_t* index )
{
	bool result = false;

	if( p_sprite )
	{
		for( uint8_t i = 0; i < p_sprite->states_count; i++ )
		{
			if( strncasecmp( state, p_sprite->states[ i ].name, p_sprite->states[ i ].name_length ) == 0 )
			{
				*index = i;
				result = true;
				break;
			}
		}
	}

	return result;
}

uint8_t sprite_state_count( const sprite_t* p_sprite )
{
	return p_sprite ? p_sprite->states_count : 0;
}

const sprite_state_t* sprite_state( const sprite_t* p_sprite, const char* state )
{
	if( p_sprite )
	{
		for( uint8_t i = 0; i < p_sprite->states_count; i++ )
		{
			if( strncasecmp( state, p_sprite->states[ i ].name, p_sprite->states[ i ].name_length ) == 0 )
			{
				return &p_sprite->states[ i ];
			}
		}
	}

	return NULL;
}

const sprite_state_t* sprite_state_by_index( const sprite_t* p_sprite, uint16_t index )
{
	assert( p_sprite && index < p_sprite->states_count );
	return &p_sprite->states[ index ];
}

const char* sprite_state_name( const sprite_state_t* p_state )
{
	return p_state ? p_state->name : "<uninitialized state>";
}

const char* sprite_state_frame_count( const sprite_state_t* p_state )
{
	assert( p_state );
	return p_state->count;
}

const sprite_frame_t* sprite_state_frames( const sprite_state_t* p_state )
{
	assert( p_state );
	return p_state->frames;
}

