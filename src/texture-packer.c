#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <libimageio/blending.h>
#include "texture-packer.h"





#define area( rect ) \
	((rect)->width * (rect)->height)

typedef struct texture_packer_image {
	uint16_t x;
	uint16_t y;
	uint16_t width;
	uint16_t height;
	uint8_t  bytes_per_pixel;
	uint8_t* pixels;
	void*    data;
} tp_image_t;


struct texture_packer_rect;
typedef struct texture_packer_rect tp_rect_t;

struct texture_packer {
	tp_image_t  final_image;
	tp_rect_t*  root;
	size_t      size;
	size_t      array_size;
	tp_image_t* images;

	tp_data_packed_fxn   data_packed;
	tp_data_destroy_fxn  data_destroy;
};


static int tp_image_compare( const void* p_left, const void* p_right )
{
	const tp_image_t* left  = p_left;
	const tp_image_t* right = p_right;

	return area(left) > area(right);
}


/*
 *  ------[ Texture Rectangle ]----------------------------
 */
enum tp_children {
	TP_CHILD_LEFT = 0,
	TP_CHILD_RIGHT,
	TP_CHILDREN_COUNT
};

struct texture_packer_rect {
	uint16_t x;
	uint16_t y;
	uint16_t width;
	uint16_t height;

	tp_image_t* image;
	struct texture_packer_rect* children[ TP_CHILDREN_COUNT ];
};

static inline tp_rect_t* tp_rect_create( uint16_t x, uint16_t y, uint16_t width, uint16_t height )
{
	tp_rect_t* rect = (tp_rect_t*) malloc( sizeof(tp_rect_t) );

	if( rect )
	{
		rect->x                          = x;
		rect->y                          = y;
		rect->width                      = width;
		rect->height                     = height;
		rect->image                      = NULL;
		rect->children[ TP_CHILD_LEFT ]  = NULL;
		rect->children[ TP_CHILD_RIGHT ] = NULL;
	}

	return rect;
}

static inline void tp_rect_destroy( tp_rect_t** rect )
{
	free( *rect );
	*rect = NULL;
}

static inline bool tp_rect_is_assigned( const tp_rect_t* rect )
{
	return rect->image != NULL;
}

static inline bool tp_rect_is_contained_within( tp_rect_t* rect, const tp_image_t* image )
{
	return image->width <= rect->width && image->height <= rect->height;
}

void tp_rect_assign_image( tp_t* tp, tp_rect_t* rect, tp_image_t* image )
{
	assert( rect->image == NULL );
	assert( image != NULL );
	assert( image->pixels != NULL );

	int dw = rect->width - image->width;
	int dh = rect->height - image->height;

	/* Form a sub-rectangle along the longest edge. */
	if( image->width < image->height ) /* form rect along height-side */
	{
		rect->children[ TP_CHILD_LEFT ]  = tp_rect_create( rect->x + image->width, rect->y, dw, image->height );
		rect->children[ TP_CHILD_RIGHT ] = tp_rect_create( rect->x, rect->y + image->height, rect->width, dh );
	}
	else /* imageWidth >= imageHeight */
	{
		rect->children[ TP_CHILD_LEFT ]  = tp_rect_create( rect->x, rect->y + image->height, image->width, dh );
		rect->children[ TP_CHILD_RIGHT ] = tp_rect_create( rect->x + image->width, rect->y, dw, rect->height );
	}

	image->x    = rect->x;
	image->y    = rect->y;
	rect->image = image;

	#ifdef DEBUG_TEXTURE_PACKER
	printf( "[Texture Packer] (x=%d, y=%d, w=%d, h=%d)\n", rect->x, rect->y, rect->width, rect->height );
	#endif
}

static inline unsigned int tp_rect_area( const tp_rect_t* rect )
{
	return area( rect );
}


/*
 *  ------[ Texture Packer ]-------------------------------
 */
tp_t* texture_packer_create( void )
{
	tp_t* tp = (tp_t*) malloc( sizeof(tp_t) );

	if( tp )
	{
		tp->root         = NULL;
		tp->size         = 0;
		tp->array_size   = 32;
		tp->data_packed  = NULL;
		tp->data_destroy = NULL;

		tp->images = (tp_image_t*) malloc( sizeof(tp_image_t) * tp->array_size );

		tp->final_image.x               = 0;
		tp->final_image.y               = 0;
		tp->final_image.width           = 0;
		tp->final_image.height          = 0;
		tp->final_image.bytes_per_pixel = 0;
		tp->final_image.pixels          = NULL;

		if( !tp->images )
		{
			free( tp );
			tp = NULL;
		}
	}

	return tp;
}

void texture_packer_destroy( tp_t** tp )
{
	texture_packer_clear( *tp );
	free( (*tp)->images );

	if( (*tp)->final_image.pixels )
	{
		free( (*tp)->final_image.pixels );
	}

	free( *tp );
}

void texture_packer_data_fxns( tp_t* tp, tp_data_packed_fxn on_packed, tp_data_destroy_fxn on_destroy )
{
	assert( tp );
	assert( on_packed );
	assert( on_destroy );

	tp->data_packed  = on_packed;
	tp->data_destroy = on_destroy;
}

bool texture_packer_add( tp_t* tp, uint16_t width, uint16_t height, uint8_t bytes_per_pixel, const uint8_t* pixels, const void* data )
{
	assert( tp );

	if( tp->size == tp->array_size )
	{
		size_t new_array_size = tp->array_size + 8;
		tp_image_t* new_images_array = (tp_image_t*) realloc( tp->images, sizeof(tp_image_t) * new_array_size );

		if( new_images_array )
		{
			tp->array_size = new_array_size;
			tp->images     = new_images_array;
		}
		else
		{
			return false;
		}
	}

	tp_image_t* image = &tp->images[ tp->size ];


	size_t image_size = sizeof(uint8_t) * width * height * bytes_per_pixel;

	image->x               = 0;
	image->y               = 0;
	image->width           = width;
	image->height          = height;
	image->bytes_per_pixel = bytes_per_pixel;
	image->pixels          = (uint8_t*) malloc( image_size );
	image->data            = (void*) data;

	assert( image->pixels != NULL );

	if( image->pixels )
	{
		memcpy( image->pixels, pixels, image_size );
	}
	else
	{
		return false;
	}

	tp->size++;

	return true;
}

void texture_packer_clear( tp_t* tp )
{
	for( size_t i = 0; i < tp->size; i++ )
	{
		tp_image_t* image = tp->images + i;
		if( tp->data_destroy )
		{
			tp->data_destroy( image->data );
		}
		free( image->pixels );
	}

	tp->size = 0;

}


static inline bool blit( tp_image_t* dst, tp_image_t* src )
{
	assert( dst->bytes_per_pixel >= src->bytes_per_pixel );

	size_t last_y = src->y + src->height;
	size_t last_x = src->x + src->width;

	for( size_t y = src->y; y < last_y; y++ )
	{
		for( size_t x = src->x; x < last_x; x++ )
		{
			size_t dstIndex = (y * dst->width + x) * dst->bytes_per_pixel;
			size_t srcIndex = ((y - src->y) * src->width + (x - src->x)) * src->bytes_per_pixel;


			memcpy( &dst->pixels[ dstIndex ], &src->pixels[ srcIndex ], src->bytes_per_pixel );
		}
	}

	return true;
}

static bool tp_blit_tree( tp_image_t* dst, tp_rect_t* root )
{
	if( root == NULL || root->image == NULL ) return true;

	if( !tp_blit_tree( dst, root->children[ TP_CHILD_LEFT ] ) )  return false;
	if( !tp_blit_tree( dst, root->children[ TP_CHILD_RIGHT ] ) ) return false;

	if( !blit( dst, root->image ) )
	{
		return false;
	}

	return true;
}

static void tp_pack_images( const tp_t* tp,  const tp_rect_t* root )
{
	if( root )
	{
		tp_pack_images( tp, root->children[ TP_CHILD_LEFT ] );
		tp_pack_images( tp, root->children[ TP_CHILD_RIGHT ] );

		if( root->image )
		{
			tp_image_t* image = root->image;
			/* Pass packed image to callback for further processing */
			tp->data_packed( image->x, image->y, image->width, image->height, image->bytes_per_pixel, image->pixels, image->data );
		}
	}
}

static inline bool tp_insert_image( tp_t* tp, tp_rect_t** root, tp_image_t* image )
{
	if( tp_rect_is_contained_within( *root, image ) )
	{
		if( tp_rect_is_assigned( *root ) )
		{
			if( tp_insert_image( tp, &(*root)->children[ TP_CHILD_LEFT ], image ) )
			{
				return true;
			}
			else
			{
				return tp_insert_image( tp, &(*root)->children[ TP_CHILD_RIGHT ], image );
			}

		}
		else
		{
			tp_rect_assign_image( tp, *root, image );
			assert( (*root)->image != NULL );
			return true;
		}
	}

	return false;
}

static inline void tp_free_tree( tp_rect_t** root )
{
	if( *root == NULL ) return;

	tp_free_tree( &(*root)->children[ TP_CHILD_LEFT ] );

	(*root)->x                          = 0;
	(*root)->y                          = 0;
	(*root)->width                      = 0;
	(*root)->height                     = 0;
	(*root)->image                      = NULL;
	(*root)->children[ TP_CHILD_LEFT ]  = NULL;
	(*root)->children[ TP_CHILD_RIGHT ] = NULL;

	tp_rect_destroy( root );
}

bool texture_packer_pack( tp_t* tp, uint16_t width, uint16_t height, uint8_t bytes_per_pixel )
{
	tp->final_image.x               = 0;
	tp->final_image.y               = 0;
	tp->final_image.width           = width;
	tp->final_image.height          = height;
	tp->final_image.bytes_per_pixel = bytes_per_pixel;
	tp->final_image.pixels          = (uint8_t*) malloc( sizeof(uint8_t) * width * height * bytes_per_pixel );

	if( tp->final_image.pixels )
	{
		memset( tp->final_image.pixels, 0xFF000000, sizeof(uint8_t) * bytes_per_pixel * width * height );
	}
	else
	{
		goto failure;
	}

	// sort the images by largest area to smallest area.
	if( tp->size > 1 )
	{
		qsort( tp->images, tp->size, sizeof(tp_image_t), tp_image_compare );
		assert( area(&tp->images[ 0 ]) >= area(&tp->images[ tp->size - 1 ]) );
	}

	tp_free_tree( &tp->root );

	// Build a tree to utilize all of the space.
	tp->root = tp_rect_create( 0, 0, width, height );
	for( size_t i = 0; i < tp->size; i++ )
	{
		tp_image_t* image = &tp->images[ i ];
		assert( image->pixels );
		bool is_assigned = tp_insert_image( tp, &tp->root, image );

		if( !is_assigned )
		{
			goto failure;
		}
	}

	// Use the tree to generate the final texture.
	if( tp_blit_tree( &tp->final_image, tp->root ) )
	{
		// Call the user data packed callback on all leaf nodes for further processing.
		if( tp->data_packed )
		{
			tp_pack_images( tp,  tp->root );
		}
	}
	else
	{
		goto failure;
	}

	tp_free_tree( &tp->root );
	return true;

failure:
	tp_free_tree( &tp->root );
	return false;
}

void texture_packer_fit_and_pack( tp_t* tp, uint8_t bytes_per_pixel )
{
	if( tp->size > 0 )
	{
		uint16_t min_width  = tp->images[ 0 ].width;
		uint16_t min_height = tp->images[ 0 ].height;
		bool increase_width = false;

		for( size_t i = 1; i < tp->size; i++ )
		{
			if( tp->images[ i ].width < min_width )
			{
				min_width = tp->images[ i ].width;
			}
			if( tp->images[ i ].height < min_height )
			{
				min_height = tp->images[ i ].height;
			}
		}

		uint16_t width  = min_width;
		uint16_t height = min_height;

		while( !texture_packer_pack( tp, width, height, bytes_per_pixel ) )
		{
			if( increase_width ) width  += min_width;
			else                 height += min_height;

			increase_width = !increase_width;
		}
	}
}

uint16_t texture_packer_width( const tp_t* tp )
{
	return tp->final_image.width;
}

uint16_t texture_packer_height( const tp_t* tp )
{
	return tp->final_image.height;
}

uint8_t texture_packer_bytes_per_pixel( const tp_t* tp )
{
	return tp->final_image.bytes_per_pixel;
}

uint8_t* texture_packer_pixels( const tp_t* tp )
{
	return tp->final_image.pixels;
}

/*bool texture_packer_save( const tp_t* tp, const tchar *filename )
{
}
*/
