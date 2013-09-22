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
#include <assert.h>
#include <getopt.h>
#include <libimageio/imageio.h>
#include <libutility/utility.h>
#include "../src/texture-packer.h"
#include "../src/sprite.h"

#define DEFAULT_FRAME_TIME    100

typedef struct sprite_info {
	sprite_t* sprite;
	char*     state;
	size_t    frame_idx;
	uint16_t  frame_time;
	uint16_t state_loop_count;
} sprite_info_t;

static void           on_packed_image     ( uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t bytes_per_pixel, uint8_t* pixels, void* data );
static sprite_info_t* sprite_info_create  ( sprite_t* sprite, const char* state, size_t idx );
static void           sprite_info_destroy ( sprite_info_t* si );

static void help( void );
static void info( sprite_t* sprite );
static void add( sprite_t* sprite, const char* state, const char* image );


struct {
	sprite_t* sprite;
	tp_t* tp;
	bool verbose;
	const char* state;
	uint16_t frame_time;
	uint16_t state_loop_count;
	uint16_t frame_count_for_state;
} sprite_compiler = { NULL, NULL, false, NULL, DEFAULT_FRAME_TIME, 0, 0 };

static struct option long_options[] =
{
	{"verbose",       no_argument,       0, 'v'},
	{"help",          no_argument,       0, 'h'},
	{"create",        required_argument, 0, 'c'},
	{"file",          required_argument, 0, 'f'},
	{"info",          no_argument,       0, 'i'},
	{"export",        no_argument,       0, 'x'},

	{"time",          required_argument, 0, 't'},
	{"loop-count",    required_argument, 0, 'l'},
	{"add",           required_argument, 0, 'a'},
	{"delete",        required_argument, 0, 'd'},

	{0, 0, 0, 0}
};




int main( int argc, char* argv[] )
{

	if( argc <= 1 )
	{
		help();
		return 0;
	}

	sprite_compiler.tp = texture_packer_create( );
	texture_packer_data_fxns( sprite_compiler.tp, on_packed_image, (tp_data_destroy_fxn) sprite_info_destroy );
	int opt;
	int opt_idx;

	while( (opt = getopt_long(argc, argv, "vhixc:f:a:t:l:", long_options, &opt_idx)) != -1 )
	{
		switch( opt )
		{
			case 'v':
				sprite_compiler.verbose = true;
				break;
			case 'c':
				sprite_compiler.sprite = sprite_create( optarg, true );
				break;
			case 'f':
				sprite_compiler.sprite = sprite_from_file( optarg );
				break;
			case 'i':
				info( sprite_compiler.sprite );
				break;
			case 't':
				sprite_compiler.frame_time = atoi( optarg );
				break;
			case 'l':
				sprite_compiler.state_loop_count = atoi( optarg );
				break;
			case 'a':
			{
				char* comma = strchr( optarg, ':' );
				char* image = comma + 1;
				*comma = '\0';
				char* state = optarg;

				string_trim( image, "\n\r\t " );
				string_trim( state, "\n\r\t " );

				add( sprite_compiler.sprite, state, image );
				break;
			}
			case 'd':
				break;
			case 'x':
			{
				texture_packer_fit_and_pack( sprite_compiler.tp, 4 );


				uint16_t width           = texture_packer_width( sprite_compiler.tp );
				uint16_t height          = texture_packer_height( sprite_compiler.tp );
				uint16_t bytes_per_pixel = texture_packer_bytes_per_pixel( sprite_compiler.tp );
				const void* pixels       = texture_packer_pixels( sprite_compiler.tp );

				#if 0
				image_t dstImage;
				dstImage.width          = width;
				dstImage.height         = height;
				dstImage.bits_per_pixel = bytes_per_pixel * 8;
				dstImage.pixels         = (uint8_t*) pixels;
				imageio_image_save( &dstImage, "/Users/manvscode/projects/libsprite/bin/output.tga", TGA );
				#endif

				sprite_set_texture( sprite_compiler.sprite, width, height, bytes_per_pixel, pixels );

				char out_file[ 256 ] = {0};
				strcat( out_file, sprite_name(sprite_compiler.sprite) );
				strcat( out_file, ".spr" );
				printf( "Saving %s\n", out_file );
				sprite_save( sprite_compiler.sprite, out_file );
				break;
			}
			default:
			case 'h':
				help();
				break;
		}

	}



	/*

	*/


	texture_packer_destroy ( &sprite_compiler.tp );

	return 0;
}



void on_packed_image( uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t bytes_per_pixel, uint8_t* pixels, void* data )
{
	sprite_info_t* sprite_info = data;

	sprite_state_t* state = sprite_state( sprite_info->sprite, sprite_info->state );

	if( !state )
	{
		/* if this is the first frame being added for this state
		 * then we need to add the state first.
		 */
		sprite_add_state( sprite_info->sprite, sprite_info->state );
		state = sprite_state( sprite_info->sprite, sprite_info->state );
	}

	//sprite_state_set_const_time( state, sprite_info->const_time );
	sprite_state_set_loop_count( state, sprite_info->state_loop_count );

	printf( "Adding frame to '%s'\n", sprite_info->state );
	sprite_state_add_frame( state, x, y, width, height, sprite_info->frame_time );
}

sprite_info_t* sprite_info_create( sprite_t* sprite, const char* state, size_t idx )
{
	sprite_info_t* ssf = (sprite_info_t*) malloc( sizeof(sprite_info_t) );

	if( ssf )
	{
		ssf->sprite    = sprite;
		ssf->state     = strdup( state );
		ssf->frame_idx = idx;

		assert( ssf->sprite );
		assert( ssf->state );
	}

	return ssf;
}

void sprite_info_destroy( sprite_info_t* si )
{
	if( si )
	{
		free( si->state );
		free( si );
	}
}

void help( void )
{
	printf( "----[ Sprite Compiler ]-----------------------------\n" );
	printf( "Copyright 2013, Joseph A. Marrero\n" );
	printf( "http://www.manvscode.com/\n\n" );
	printf( "Command line options:\n" );

	//for( size_t i = 0; i < sizeof(long_options)/sizeof(long_options[0]); i++ )
	{
		printf( "  -%c, --%-12s %-s\n", 'h', "help",   "Get help on how to use this program." );
		printf( "  -%c, --%-12s %-s\n", 'c', "create", "Create a new sprite." );
		printf( "  -%c, --%-12s %-s\n", 't', "time",   "Set the frame time." );
		printf( "  -%c, --%-12s %-s\n", 'l', "loop-count",   "Set the state loop count. Zero is interpreted as infinitely looped." );
	}

	printf( "----------------------------------------------------\n" );
}

void info( sprite_t* sprite )
{
	printf( "----[ Sprite Info ]------------------------------------------\n" );
	printf( "Name: %-40s\n", sprite_name( sprite ) );
	printf( "Width: %-6d  Height: %-6d  Bit Depth: %-dbpp\n", sprite_width(sprite), sprite_height(sprite), sprite_bytes_per_pixel(sprite) == 4 ? 32 : 24 );
	printf( "Number of States: %6d\n", sprite_state_count(sprite) );
	printf( "----[ States ]-----------------------------------------------\n" );
	const sprite_state_t* state = sprite_first_state( sprite );

	while( state )
	{
		printf( "Name: \"%-s\"  ", sprite_state_name(state) );

		if( sprite_state_loop_count(state) > 0 )
		{
			printf( "Loop Count: %-3d ", sprite_state_loop_count(state) );
		}
		else
		{
			printf( "Loop Count: inf  " );
		}

		if( sprite_state_const_time(state) > 0 )
		{
			printf( "Constant Time: %-3d\n", sprite_state_const_time(state) );
		}
		else
		{
			printf( "Constant Time: Unused\n" );
		}

		for( size_t i = 0; i < sprite_state_frame_count( state ); i++ )
		{
			const sprite_frame_t* frame = sprite_state_frame( state, i );

			printf( "        |\n" );

			printf( "        +--- Frame %2zd: [x:%3d, y:%3d, W:%3d, H:%3d] @ %dms\n", i, frame->x, frame->y, frame->width, frame->height, frame->time );
		}

		printf( "\n" );

		state = sprite_next_state( sprite );
	}
	printf( "-------------------------------------------------------------\n" );
}

void add( sprite_t* sprite, const char* state, const char* filename )
{
	printf( "Added %s to state '%s'\n", filename, state );

	const char* extension = strrchr( filename, '.' );
	image_file_format_t format = PNG;
	image_t image;

	if( extension )
	{
		extension += 1;

		if( strcasecmp( "png", extension ) == 0 )
		{
			format = IMAGEIO_PNG;
		}
		else if( strcasecmp( "bmp", extension ) == 0 )
		{
			format = IMAGEIO_BMP;
		}
		else if( strcasecmp( "tga", extension ) == 0 )
		{
			format = IMAGEIO_TGA;
		}
	}

	imageio_image_load( &image, filename, format );


	sprite_info_t* info = sprite_info_create( sprite, state, sprite_compiler.frame_count_for_state );
	info->state_loop_count = sprite_compiler.state_loop_count;
	info->frame_time = sprite_compiler.frame_time;

	texture_packer_add( sprite_compiler.tp, image.width, image.height, image.bits_per_pixel / 8, image.pixels, info );
	sprite_compiler.frame_count_for_state++;

	imageio_image_destroy( &image );
}
