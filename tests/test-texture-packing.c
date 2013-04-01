#include <stdio.h>
#include <texture-packer.h>
#include <imageio.h>

int main( int argc, char* argv[] )
{
	image_t image1;
	imageio_image_load( &image1, "/home/joe/projects/libsprite/bin/troll.tga", TARGA );
	image_t image2;
	imageio_image_load( &image2, "/home/joe/projects/libsprite/bin/warrior.tga", TARGA );


	tp_t* tp = texture_packer_create( );

	texture_packer_add( tp, image1.width, image1.height, image1.bitsPerPixel / 8, image1.pixels );
	texture_packer_add( tp, image2.width, image2.height, image2.bitsPerPixel / 8, image2.pixels );

	texture_packer_pack( tp, 100, 32, 4, true );

	image_t dstImage;
	dstImage.width        = texture_packer_width( tp );
	dstImage.height       = texture_packer_height( tp );
	dstImage.bitsPerPixel = texture_packer_bpp( tp ) * 8;
	dstImage.pixels       = texture_packer_pixels( tp );

	imageio_image_save( &dstImage, "/home/joe/projects/libsprite/bin/output.tga", TARGA );
	texture_packer_destroy ( &tp );

	return 0;
}
