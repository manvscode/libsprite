#include <stdio.h>
#include <libimageio/imageio.h>
#include <texture-packer.h>

int main( int argc, char* argv[] )
{
	image_t image1;
	imageio_image_load( &image1, "/home/joe/projects/libsprite/bin/troll.tga", TGA );
	image_t image2;
	imageio_image_load( &image2, "/home/joe/projects/libsprite/bin/warrior.tga", TGA );


	tp_t* tp = texture_packer_create( );

	texture_packer_add( tp, image1.width, image1.height, image1.bits_per_pixel / 8, image1.pixels );
	texture_packer_add( tp, image2.width, image2.height, image2.bits_per_pixel / 8, image2.pixels );

	texture_packer_pack( tp, 100, 32, 4 );

	image_t dstImage;
	dstImage.width          = texture_packer_width( tp );
	dstImage.height         = texture_packer_height( tp );
	dstImage.bits_per_pixel = texture_packer_bpp( tp ) * 8;
	dstImage.pixels         = texture_packer_pixels( tp );

	imageio_image_save( &dstImage, "/home/joe/projects/libsprite/bin/output.tga", TGA );
	texture_packer_destroy ( &tp );

	return 0;
}
