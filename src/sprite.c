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
#include <libutility/utility.h>
#include "sprite.h"

#define UNKNOWN_NAME       ("<unknown>")

struct sprite_state {
	char     name[ SPRITE_MAX_STATE_NAME_LENGTH + 1 ];
	uint16_t const_time; /* optional. 0 means to ignore and use frame's time */
	uint16_t loop_count; /* optional, 0 if loops forever */
	array_t  frames;
};

struct sprite {
	char     marker_and_bom[ 4 ]; // "SPR"0
	uint16_t name_length;
	char*    name;
	uint16_t width;
	uint16_t height;
	uint8_t  bytes_per_pixel;
	void*    pixels;

	tree_map_t states;  /* name -> state */
	tree_map_iterator_t state_itr;
};

static void   _sprite_create            ( sprite_t* p_sprite, const char* name, bool use_transparency );
static void   _sprite_destroy           ( sprite_t* p_sprite );


sprite_state_t* sprite_state_create( const char* name )
{
	sprite_state_t* p_state = malloc( sizeof(sprite_state_t) );

	if( p_state )
	{
		strncpy( p_state->name, name, SPRITE_MAX_STATE_NAME_LENGTH );
		p_state->name[ SPRITE_MAX_STATE_NAME_LENGTH ] = '\0';

		p_state->const_time  = 0;
		p_state->loop_count  = 0;

		array_create( &p_state->frames, sizeof(sprite_frame_t), 0, malloc, free );
	}

	return p_state;
}

void sprite_state_destroy( sprite_state_t* p_state )
{
	assert( p_state );
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

	if( !name || *name == '\0' )
	{
		name = "unknown";
	}

	p_sprite->marker_and_bom[ 0 ] = 'S';
	p_sprite->marker_and_bom[ 1 ] = 'P';
	p_sprite->marker_and_bom[ 2 ] = 'R';

	#ifdef SPRITE_USE_MACHINE_ENDIANNESS
	p_sprite->marker_and_bom[ 3 ] = is_big_endian( );
	#else
	p_sprite->marker_and_bom[ 3 ] = 0; /* use little endian encoding */
	#endif

	p_sprite->name            = strdup( name );
	p_sprite->name_length     = strlen( name );
	p_sprite->width           = 0;
	p_sprite->height          = 0;
	p_sprite->bytes_per_pixel = use_transparency ? 4 : 3;
	p_sprite->pixels          = NULL;

	tree_map_create( &p_sprite->states, (tree_map_element_function) sprite_state_map_destroy,
  	                 (tree_map_compare_function) sprite_state_name_compare, malloc, free );
	p_sprite->state_itr = NULL;
}

void sprite_destroy( sprite_t** p_sprite )
{
	if( *p_sprite )
	{
		#ifdef DEBUG_SPRITE
		printf( "[Sprite] Destroyed: %s\n", sprite_name( *p_sprite ) );
		#endif

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

void sprite_set_name( sprite_t* p_sprite, const char* name )
{
	if( p_sprite->name )
	{
		free( p_sprite->name );
	}

	if( !name || *name == '\0' )
	{
		name = "unknown";
	}

	p_sprite->name        = strdup( name );
	p_sprite->name_length = strlen( name );
}

void sprite_set_texture( sprite_t* p_sprite, uint16_t w, uint16_t h, uint16_t bytes_per_pixel, const void* pixels )
{
	assert( p_sprite );
	p_sprite->width           = w;
	p_sprite->height          = h;
	p_sprite->bytes_per_pixel = bytes_per_pixel;

	if( p_sprite->pixels )
	{
		free( p_sprite->pixels );
	}

	size_t size = sizeof(uint8_t) * p_sprite->width * p_sprite->height * p_sprite->bytes_per_pixel;
	p_sprite->pixels = malloc( size );
	memcpy( p_sprite->pixels, pixels, size );
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

bool sprite_add_frame( sprite_t* p_sprite, const char* state, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t time )
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
			p_frame->time   = time;

			result = true;
		}
	}

	return result;
}

void sprite_remove_all_states( sprite_t* p_sprite )
{

	if( p_sprite )
	{
		while( tree_map_size( &p_sprite->states ) > 0 )
		{
			tree_map_clear( &p_sprite->states );
		}
	}

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
			assert( index < array_size(&p_state->frames) );

			/* shift the array */
			for( int16_t i = index; i < array_size(&p_state->frames) - 1; i++ )
			{
				assert( i < array_size(&p_state->frames) );
				assert( (i+1) < array_size(&p_state->frames) );
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
	return p_sprite && p_sprite->name ? p_sprite->name : UNKNOWN_NAME;
}

uint16_t sprite_width( const sprite_t* p_sprite )
{
	return p_sprite ? p_sprite->width : 0;
}

uint16_t sprite_height( const sprite_t* p_sprite )
{
	return p_sprite ? p_sprite->height : 0;
}

uint16_t sprite_bit_depth( const sprite_t* p_sprite )
{
	return p_sprite ? p_sprite->bytes_per_pixel << 3 : 0;
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

const sprite_state_t* sprite_first_state( sprite_t* p_sprite )
{
	if( p_sprite && sprite_state_count(p_sprite) > 0 )
	{
		p_sprite->state_itr = tree_map_begin( &p_sprite->states );

		if( p_sprite->state_itr )
		{
			return p_sprite->state_itr->value;
		}
	}

	return NULL;
}

const sprite_state_t* sprite_next_state( sprite_t* p_sprite )
{
	if( p_sprite && p_sprite->state_itr )
	{
		p_sprite->state_itr = tree_map_next( p_sprite->state_itr );

		if( p_sprite->state_itr )
		{
			return p_sprite->state_itr->value;
		}
	}

	return NULL;
}

uint8_t sprite_state_count( const sprite_t* p_sprite )
{
	assert( p_sprite );
	return p_sprite ? tree_map_size( &p_sprite->states ) : 0;
}

const char* sprite_state_name( const sprite_state_t* p_state )
{
	assert( p_state );
	return p_state ? p_state->name : UNKNOWN_NAME;
}

uint16_t sprite_state_const_time( const sprite_state_t* p_state )
{
	assert( p_state );
	return p_state->const_time;
}

uint16_t sprite_state_loop_count( const sprite_state_t* p_state )
{
	assert( p_state );
	return p_state->loop_count;
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

#define SPRITE_USE_LITTLE_ENDIAN

static inline size_t sprite_writef( void* ptr, size_t size, FILE* file, bool is_big_endian )
{
	size_t result = 0;

	assert( file );
	assert( ptr );
	assert( size > 0 );

	if( !feof(file) && !ferror(file) )
	{
		hton( ptr, size );
		result = fwrite( ptr, size, 1, file );
		ntoh( ptr, size ); /* don't mutate the sprite's data */
	}


	return result;
}


static inline size_t sprite_readf( void* ptr, size_t size, FILE* file, bool is_big_endian )
{
	size_t result = 0;

	assert( file );
	assert( ptr );
	assert( size > 0 );

	if( !feof(file) && !ferror(file) )
	{
		result = fread( ptr, size, 1, file );
		ntoh( ptr, size );
	}


	return result;
}

#ifdef DEBUG_SPRITE
#define sprite_read(ptr, size, file, is_big_endian)   if( sprite_readf(ptr, size, file, is_big_endian) != 1 )  {assert( false && "read failed" ); goto failure; }
#define sprite_write(ptr, size, file, is_big_endian)  if( sprite_writef(ptr, size, file, is_big_endian) != 1 ) {assert( false && "write failed" ); goto failure; }
#else
#define sprite_read(ptr, size, file, is_big_endian)   if( sprite_readf(ptr, size, file, is_big_endian) == 1 )  goto failure;
#define sprite_write(ptr, size, file, is_big_endian)  if( sprite_writef(ptr, size, file, is_big_endian) == 1 ) goto failure;
#endif


sprite_t* sprite_from_file( const char* filename )
{
	sprite_t* p_sprite = NULL;
	FILE* file = fopen( filename, "rb" );

	if( !file )
	{
		goto failure;
	}

	#ifdef DEBUG_SPRITE
	printf( "[Sprite] Opening: %s\n", filename );
	#endif

	char marker_and_bom[ 4 ] = { 0 };
	fread( marker_and_bom, sizeof(char), sizeof(marker_and_bom), file );

	if( marker_and_bom[ 0 ] != 'S' || marker_and_bom[ 1 ] != 'P' || marker_and_bom[ 2 ] != 'R' )
	{
		goto failure;
	}

	bool is_big_endian = marker_and_bom[ 3 ];
	p_sprite = sprite_create( NULL, true );
	memcpy( p_sprite->marker_and_bom, marker_and_bom, sizeof(char) * 4 );


	sprite_read( &p_sprite->name_length, sizeof(p_sprite->name_length), file, is_big_endian );
	assert( p_sprite->name_length > 0 );
	p_sprite->name = malloc( sizeof(char) * p_sprite->name_length );
	fread( p_sprite->name, sizeof(char), p_sprite->name_length + 1, file );

	sprite_read( &p_sprite->width, sizeof(p_sprite->width), file, is_big_endian );
	sprite_read( &p_sprite->height, sizeof(p_sprite->height), file, is_big_endian );
	fread( &p_sprite->bytes_per_pixel, sizeof(uint8_t), 1, file );
	p_sprite->pixels = malloc( sizeof(uint8_t) * p_sprite->width * p_sprite->height * p_sprite->bytes_per_pixel );
	sprite_read( p_sprite->pixels, sizeof(uint8_t) * p_sprite->width * p_sprite->height * p_sprite->bytes_per_pixel, file, is_big_endian );

	uint16_t state_count = 0;
	sprite_read( &state_count, sizeof(state_count), file, is_big_endian );

	while( state_count-- > 0 )
	{
		sprite_state_t* state = sprite_state_create( UNKNOWN_NAME );


		fread( state->name, sizeof(char), SPRITE_MAX_STATE_NAME_LENGTH + 1, file );
		sprite_read( &state->const_time, sizeof(state->const_time), file, is_big_endian );
		sprite_read( &state->loop_count, sizeof(state->loop_count), file, is_big_endian );

		tree_map_insert( &p_sprite->states, state->name, state );

		uint16_t frame_count = 0;
		sprite_read( &frame_count, sizeof(frame_count), file, is_big_endian );


		while( frame_count-- > 0 )
		{
			uint16_t x = 0;
			uint16_t y = 0;
			uint16_t width = 0;
			uint16_t height = 0;
			uint16_t time = 0;

			sprite_read( &x, sizeof(x), file, is_big_endian );
			sprite_read( &y, sizeof(y), file, is_big_endian );
			sprite_read( &width, sizeof(width), file, is_big_endian );
			sprite_read( &height, sizeof(height), file, is_big_endian );
			sprite_read( &time, sizeof(time), file, is_big_endian );

			sprite_add_frame( p_sprite, state->name, x, y, width, height, time );
		}
	}

	#ifdef DEBUG_SPRITE
	printf( "[Sprite] Loaded: %s\n", sprite_name( p_sprite ) );
	#endif

	return p_sprite;

failure:
	if( p_sprite ) sprite_destroy( &p_sprite );
	return NULL;
}

bool sprite_save( sprite_t* p_sprite, const char* filename )
{
	FILE* file = fopen( filename, "w+b" );

	if( !file )
	{
		return false;
	}

	bool is_big_endian = p_sprite->marker_and_bom[ 3 ];

	fwrite( p_sprite->marker_and_bom, sizeof(char), sizeof(p_sprite->marker_and_bom), file );

	sprite_write( &p_sprite->name_length, sizeof(p_sprite->name_length), file, is_big_endian );
	fwrite( p_sprite->name, sizeof(char), p_sprite->name_length + 1, file );
	sprite_write( &p_sprite->width, sizeof(p_sprite->width), file, is_big_endian );
	sprite_write( &p_sprite->height, sizeof(p_sprite->height), file, is_big_endian );
	fwrite( &p_sprite->bytes_per_pixel, sizeof(uint8_t), 1, file );
	sprite_write( p_sprite->pixels, sizeof(uint8_t) * p_sprite->width * p_sprite->height * p_sprite->bytes_per_pixel, file, is_big_endian );

	uint16_t state_count = tree_map_size( &p_sprite->states );
	sprite_write( &state_count, sizeof(state_count), file, is_big_endian );

	tree_map_iterator_t itr;
	for( itr = tree_map_begin(&p_sprite->states);
	     itr != tree_map_end( );
	     itr = tree_map_next(itr) )
	{
		sprite_state_t* state = itr->value;

		fwrite( state->name, sizeof(char), SPRITE_MAX_STATE_NAME_LENGTH + 1, file );
		sprite_write( &state->const_time, sizeof(state->const_time), file, is_big_endian );
		sprite_write( &state->loop_count, sizeof(state->loop_count), file, is_big_endian );

		uint16_t frame_count = array_size( &state->frames );
		sprite_write( &frame_count, sizeof(frame_count), file, is_big_endian );


		for( size_t i = 0; i < frame_count; i++ )
		{
			sprite_frame_t* frame = (sprite_frame_t*) array_element( &state->frames, i );

			sprite_write( &frame->x, sizeof(frame->x), file, is_big_endian );
			sprite_write( &frame->y, sizeof(frame->y), file, is_big_endian );
			sprite_write( &frame->width, sizeof(frame->width), file, is_big_endian );
			sprite_write( &frame->height, sizeof(frame->height), file, is_big_endian );
			sprite_write( &frame->time, sizeof(frame->time), file, is_big_endian );
		}
	}

	#ifdef DEBUG_SPRITE
	printf( "[Sprite] Saved: %s\n", sprite_name( p_sprite ) );
	#endif

	fclose( file );
	return true;

failure:
	if( file ) fclose( file );
	return true;
}
