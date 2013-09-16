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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libcollections/array.h>
#include <libcollections/tree-map.h>
#include "sprite.h"

struct sprite_state {
	uint16_t name_length;
	char*    name;
	uint16_t fps;
	array_t  frames;
};

struct sprite {
	uint16_t name_length;
	char*    name;
	uint16_t width;
	uint16_t height;
	uint8_t  bytes_per_pixel;
	void*    pixels;

	tree_map_t states;  /* name -> state */
};

static void   _sprite_create            ( sprite_t* p_sprite, const char* name, bool use_transparency );
static void   _sprite_destroy           ( sprite_t* p_sprite );


sprite_state_t* sprite_state_create( const char* name )
{
	sprite_state_t* p_state = malloc( sizeof(sprite_state_t) );

	if( p_state )
	{
		p_state->name_length = strlen( name );
		p_state->name        = strdup( name );
		p_state->fps         = 60;

		array_create( &p_state->frames, sizeof(sprite_frame_t), 0, malloc, free );
	}

	return p_state;
}

void sprite_state_destroy( sprite_state_t* p_state )
{
	assert( p_state );

	if( p_state->name )
	{
		free( p_state->name );
		#ifdef SPRITE_DEBUG
		p_state->name_length = 0;
		p_state->name        = NULL;
		p_state->fps         = 0;
		#endif
	}

	array_destroy( &p_state->frames );
	free( p_state );
}


boolean sprite_state_map_destroy( const char* name, sprite_state_t* state )
{
	sprite_state_destroy( state );
	return true;
}


int sprite_state_name_compare( const char* name1, const char* name2 )
{
	return strcasecmp( name1, name2 );
}


sprite_t* sprite_create( const char* name, bool use_transparency )
{
	sprite_t* p_sprite = malloc( sizeof(sprite_t) );

	if( p_sprite )
	{
		_sprite_create( p_sprite, name, use_transparency );
	}

	return p_sprite;
}

void _sprite_create( sprite_t* p_sprite, const char* name, bool use_transparency )
{
	assert( p_sprite );
	p_sprite->name            = strdup( name );
	p_sprite->name_length     = strlen( name );
	p_sprite->width           = 0;
	p_sprite->height          = 0;
	p_sprite->bytes_per_pixel = use_transparency ? 4 : 3;
	p_sprite->pixels          = NULL;

	tree_map_create( &p_sprite->states, (tree_map_element_function) sprite_state_map_destroy,
  	                 (tree_map_compare_function) sprite_state_name_compare, malloc, free );
}

void sprite_destroy( sprite_t** p_sprite )
{
	if( *p_sprite )
	{
		_sprite_destroy( *p_sprite );

		free( *p_sprite );
		*p_sprite = NULL;
	}
}

void _sprite_destroy( sprite_t* p_sprite )
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

	tree_map_destroy( &p_sprite->states );
}

bool sprite_add_state( sprite_t* p_sprite, const char* name )
{
	bool result = false;

	if( p_sprite )
	{
		sprite_state_t* p_state = sprite_state_create( name );

		result = tree_map_insert( &p_sprite->states, p_state->name, p_state );
	}

	return result;
}

bool sprite_add_frame( sprite_t* p_sprite, const char* state, uint16_t x, uint16_t y, uint16_t width, uint16_t height )
{
	bool result = false;

	if( p_sprite && state )
	{
		sprite_state_t* p_state = NULL;

		if( tree_map_find( &p_sprite->states, state, (void**) &p_state ) )
		{
			size_t new_array_size = array_size(&p_state->frames) + 1;
			array_resize( &p_state->frames, new_array_size );

			sprite_frame_t* p_frame = array_elem( &p_state->frames, new_array_size - 1, sprite_frame_t );

			p_frame->x      = x;
			p_frame->y      = y;
			p_frame->width  = width;
			p_frame->height = height;

			result = true;
		}
	}

	return result;
}

bool sprite_remove_state( sprite_t* p_sprite, const char* name )
{
	bool result = false;

	if( p_sprite && name )
	{
		result = tree_map_remove( &p_sprite->states, name );
	}

	return result;
}

bool sprite_remove_frame( sprite_t* p_sprite, const char* state, uint16_t index )
{
	bool result = false;

	if( p_sprite && state )
	{
		sprite_state_t* p_state = NULL;

		if( tree_map_find( &p_sprite->states, state, (void**) &p_state ) )
		{
			/* shift the array */
			for( uint16_t i = index; i < array_size(&p_state->frames); i++ )
			{
				sprite_frame_t* current  = array_elem( &p_state->frames, i, sprite_frame_t );
				sprite_frame_t* neighbor = array_elem( &p_state->frames, i + 1, sprite_frame_t );

				*current = *neighbor;
			}

			/* decrease the array by 1 */
			size_t new_array_size = array_size(&p_state->frames) - 1;
			array_resize( &p_state->frames, new_array_size );
			result = true;
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

const sprite_state_t* sprite_state( const sprite_t* p_sprite, const char* state )
{
	if( p_sprite && state )
	{
		sprite_state_t* p_state = NULL;
		if( tree_map_find( &p_sprite->states, state, (void**) &p_state ) )
		{
			return p_state;
		}
	}

	return NULL;
}

uint8_t sprite_state_count( const sprite_t* p_sprite )
{
	return p_sprite ? tree_map_size( &p_sprite->states ) : 0;
}

const char* sprite_state_name( const sprite_state_t* p_state )
{
	return p_state ? p_state->name : "<uninitialized state>";
}

uint16_t sprite_state_frame_count( const sprite_state_t* p_state )
{
	assert( p_state );
	return array_size( &p_state->frames );
}

const sprite_frame_t* sprite_state_frame( const sprite_state_t* p_state, uint16_t index )
{
	assert( p_state );
	return array_elem( (array_t*) &p_state->frames, index, sprite_frame_t );
}



bool sprite_load( sprite_t* p_sprite, const char* filename )
{
	/* TODO */
	return true;
}

bool sprite_save( sprite_t* p_sprite, const char* filename )
{
	/* TODO */
	return true;
}
