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
#include <libimageio/imageio.h>
#include <texture-packer.h>
#include <sprite.h>

typedef struct sprite_info {
	sprite_t* sprite;
	char*     state;
	size_t    frame_idx;
} sprite_info_t;

static void              on_packed_image  ( uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t bytes_per_pixel, uint8_t* pixels, void* data );
static sprite_info_t* sprite_info_create  ( sprite_t* sprite, const char* state, size_t idx );
static void           sprite_info_destroy ( sprite_info_t* si );



int main( int argc, char* argv[] )
{
	image_t image1;
	imageio_image_load( &image1, "/Users/manvscode/projects/libsprite/bin/troll.tga", TGA );
	image_t image2;
	imageio_image_load( &image2, "/Users/manvscode/projects/libsprite/bin/warrior.tga", TGA );


	sprite_t* sprite = sprite_create( "Test Sprite", true /*uses RGBA*/ );

	tp_t* tp = texture_packer_create( );
	texture_packer_data_fxns( tp, on_packed_image, (tp_data_destroy_fxn) sprite_info_destroy );

	texture_packer_add( tp, image1.width, image1.height, image1.bits_per_pixel / 8, image1.pixels, sprite_info_create(sprite, "idle", 0) );
	texture_packer_add( tp, image2.width, image2.height, image2.bits_per_pixel / 8, image2.pixels, sprite_info_create(sprite, "idle", 1) );

	texture_packer_fit_and_pack( tp, 4 );

	image_t dstImage;
	dstImage.width          = texture_packer_width( tp );
	dstImage.height         = texture_packer_height( tp );
	dstImage.bits_per_pixel = texture_packer_bpp( tp ) * 8;
	dstImage.pixels         = texture_packer_pixels( tp );

	imageio_image_save( &dstImage, "/Users/manvscode/projects/libsprite/bin/output.tga", TGA );
	texture_packer_destroy ( &tp );

	return 0;
}



void on_packed_image( uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t bytes_per_pixel, uint8_t* pixels, void* data )
{
	sprite_info_t* sprite_info = data;

	if( !sprite_state( sprite_info->sprite, sprite_info->state ) )
	{
		/* if this is the first frame being added for this state
		 * then we need to add the state first.
		 */
		sprite_add_state( sprite_info->sprite, sprite_info->state );
	}

	sprite_add_frame( sprite_info->sprite, sprite_info->state, x, y, width, height, 16 /* 16ms for 60 fps */ );
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
