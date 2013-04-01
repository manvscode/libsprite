/*	imageio.c
 *
 *	Various Data Structures and Image loading routines.
 *
 *	Coded by Joseph A. Marrero
 *	http://www.ManVsCode.com/
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>	
#include "imageio.h"
/*
 *	Miscellaneous Utility functions...
 */
/* (x,y) to bitmap array index mapping macros */
#define pixel_index( x, y, byteCount, width )		((size_t) ((width) * (y) * (byteCount) + (x) * (byteCount)))
#define linear_interpolate( alpha, x2, x1 ) ( x1 + alpha * ( x2 - x1 ) )
#define lerp linear_interpolate
#define bilinear_interpolate( alpha, beta, x1, x2, x3, x4 )		(lerp( beta, lerp( alpha, x1, x2 ), lerp( alpha, x3, x4 ) ))
#define bilerp bilinear_interpolate


#define linear_interpolate( alpha, x2, x1 ) ( x1 + alpha * ( x2 - x1 ) )
#define lerp linear_interpolate
#define bilinear_interpolate( alpha, beta, x1, x2, x3, x4 )		(lerp( beta, lerp( alpha, x1, x2 ), lerp( alpha, x3, x4 ) ))
#define bilerp bilinear_interpolate

#define convertRGBtoBGR    imageio_swap_red_and_blue
#define convertBGRtoRGB    imageio_swap_red_and_blue

/*
 *	Windows Bitmap
 */
#define	BITMAP_ID	0x4D42

#define BI_RGB			0L
#define BI_RLE8			1L
#define BI_RLE4			2L
#define BI_BITFIELDS	3L
#define BI_JPEG			4L
#define BI_PNG			5L

#pragma pack(push, 2)
imageio_api typedef struct imageio_bitmap_file_header {
	uint16_t bfType;       // Specifies File Type; Must be BM (0x4D42)
	uint32_t bfSize;       // Specifies the size in bytes of the bitmap
	uint16_t bfReserved1;  // Reserved; Must be zero!
	uint16_t bfReserved2;  // Reserved; Must be zero!
	uint32_t bfOffBits;    // Specifies the offset, in bytes, from the beginning to the bitmap bits
} bitmap_file_header_t;

imageio_api typedef struct imageio_bitmap_info_header {
	uint32_t   biSize;          // Specifies number of bytes required by the structure
	int32_t    biWidth;	        // Specifies the width of the bitmap, in pixels
	int32_t    biHeight;        // Specifies the height of the bitmap, in pixels
	uint16_t   biPlanes;        // Specifies the number of color planes, must be 1
	uint16_t   biBitCount;      // Specifies the bits per pixel, must be 1, 4,
			                    // 8, 16, 24, or 32
	uint32_t   biCompression;   // Specifies the type of compression
	uint32_t   biSizeImage;     // Specifies the size of the image in bytes
	int32_t    biXPelsPerMeter; // Specifies the number of pixels per meter in x axis
	int32_t    biYPelsPerMeter; // Specifies the number of pixels per meter in y axis
	uint32_t   biClrUsed;       // Specifies the number of colors used by the bitmap
	uint32_t   biClrImportant;  // Specifies the number of colors that are important
} bitmap_info_header_t;
#pragma pack(pop)

/*
 *  Targa
 */
#pragma pack(push, 1)
imageio_api typedef struct imageio_targa_file_header {
	uint8_t imageIDLength;      // number of bytes in identification field;
                                // 0 denotes no identification is included.
	uint8_t colorMapType;       // type of color map; always 0
	uint8_t imageTypeCode;      //  0  -  No image data included.
                                //	1  -  Uncompressed, color-mapped images.
                                //	2  -  Uncompressed, RGB images.
                                //	3  -  Uncompressed, black and white images.
                                //	9  -  Runlength encoded color-mapped images.
                                //  10 -  Runlength encoded RGB images.
                                //  11 -  Compressed, black and white images.
                                //  32 -  Compressed color-mapped data, using Huffman, Delta, and
                                //	  	  runlength encoding.
                                //  33 -  Compressed color-mapped data, using Huffman, Delta, and
                                //		  runlength encoding.  4-pass quadtree-type process.
	int16_t colorMapOrigin;     // origin of color map (lo-hi); always 0
	int16_t colorMapLength;     // length of color map (lo-hi); always 0
	uint8_t colorMapEntrySize;  // color map entry size (lo-hi); always 0;
	int16_t imageXOrigin;       // x coordinate of lower-left corner of image; (lo-hi); always 0
	int16_t imageYOrigin;       // y coordinate of lower-left corner of image; (lo-hi); always 0
	uint16_t width;             // width of image in pixels (lo-hi)
	uint16_t height;            // height of image in pixels (lo-hi)
	uint8_t bitCount;           // number of bits; 16, 24, 32
	uint8_t imageDescriptor;    // 24 bit = 0x00; 32-bit = 0x08
} targa_file_header_t;
#pragma pack(pop)


static inline bool imageio_bitmap_load ( const char* filename, bitmap_info_header_t* info_header, uint8_t** bitmap );
static inline bool imageio_bitmap_save ( const char* filename, uint32_t width, uint32_t height, uint32_t bitsPerPixel, uint8_t* imageData );
static inline bool imageio_targa_load  ( const char* filename, targa_file_header_t* p_file_header, uint8_t** bitmap );
static inline bool imageio_targa_save  ( const char* filename, targa_file_header_t* p_file_header, uint8_t* bitmap );

static inline void imageio_resize_nearest_neighbor      ( uint32_t srcWidth, uint32_t srcHeight, uint32_t srcBitsPerPixel, const uint8_t* srcBitmap, uint32_t dstWidth, uint32_t dstHeight, uint32_t dstBitsPerPixel, uint8_t* dstBitmap );
static inline void imageio_resize_bilinear_rgba         ( uint32_t srcWidth, uint32_t srcHeight, const uint8_t* srcBitmap, uint32_t dstWidth, uint32_t dstHeight, uint8_t* dstBitmap, uint32_t byteCount );
static inline void imageio_resize_bilinear_sharper_rgba ( uint32_t srcWidth, uint32_t srcHeight, const uint8_t* srcBitmap, uint32_t dstWidth, uint32_t dstHeight, uint8_t* dstBitmap, uint32_t byteCount );
static inline void imageio_resize_bilinear_rgb          ( uint32_t srcWidth, uint32_t srcHeight, const uint8_t* srcBitmap, uint32_t dstWidth, uint32_t dstHeight, uint8_t* dstBitmap, uint32_t byteCount );
static inline void imageio_resize_bilinear_sharper_rgb  ( uint32_t srcWidth, uint32_t srcHeight, const uint8_t* srcBitmap, uint32_t dstWidth, uint32_t dstHeight, uint8_t* dstBitmap, uint32_t byteCount ); 
static inline void imageio_resize_bicubic_rgba          ( uint32_t srcWidth, uint32_t srcHeight, const uint8_t* srcBitmap, uint32_t dstWidth, uint32_t dstHeight, uint8_t* dstBitmap, uint32_t byteCount );
static inline void imageio_resize_bicubic_rgb           ( uint32_t srcWidth, uint32_t srcHeight, const uint8_t* srcBitmap, uint32_t dstWidth, uint32_t dstHeight, uint8_t* dstBitmap, uint32_t byteCount );




bool imageio_image_load( image_t* img, const char* filename, image_file_format_t format )
{
	bool result = false;
	#ifdef NDEBUG
	img->pixels = NULL;
	#endif

	switch( format )
	{
		case BITMAP:
		{
			bitmap_info_header_t bmpInfoHeader;
			result = imageio_bitmap_load( filename, &bmpInfoHeader, &img->pixels );
			assert( img->pixels != NULL );
			img->bitsPerPixel = bmpInfoHeader.biBitCount;
			img->width = bmpInfoHeader.biWidth;
			img->height = bmpInfoHeader.biHeight;
			break;
		}
		case TARGA:
		{
			targa_file_header_t tgaHeader;
			result = imageio_targa_load( filename, &tgaHeader, &img->pixels );
			assert( img->pixels != NULL );	
			img->bitsPerPixel = tgaHeader.bitCount;
			img->width = tgaHeader.width;
			img->height = tgaHeader.height;
			break;
		}
		default:
			break;
	}
	

	return result;
}

bool imageio_image_save( image_t* img, const char* filename, image_file_format_t format )
{
	bool result = false;

	switch( format )
	{
		case BITMAP:
		{
			result = imageio_bitmap_save( filename, img->width, img->height, img->bitsPerPixel, img->pixels );
			assert( img->pixels != NULL );
			break;
		}
		case TARGA:
		{				
			targa_file_header_t tgaFileHeader;
			tgaFileHeader.bitCount = img->bitsPerPixel;
			tgaFileHeader.width = img->width;
			tgaFileHeader.height = img->height;
			result = imageio_targa_save( filename, &tgaFileHeader, img->pixels );
			break;
		}
		default:
			break;
	}
	

	return result;
}

void imageio_image_destroy( image_t* img )
{
	free( img->pixels );
	#ifdef NDEBUG
	img->pixels = NULL;
	#endif
}

bool imageio_bitmap_load( const char* filename, bitmap_info_header_t* info_header, uint8_t** bitmap )
{
	FILE				*filePtr;
	bitmap_file_header_t bmp_file_header;
	register uint32_t		imageIdx = 0;
	unsigned short bytesPerPixel = 0;
	uint32_t bitmapSize = 0;
	uint32_t scanlineBytes = 0;
	uint32_t stride = 0;

	if( (filePtr = fopen( filename, "rb" )) == NULL )
	{
		*bitmap = NULL;
		return false;
	}

	fread( &bmp_file_header.bfType, sizeof(uint16_t), 1, filePtr );
	fread( &bmp_file_header.bfSize, sizeof(uint32_t), 1, filePtr );
	fread( &bmp_file_header.bfReserved1, sizeof(uint16_t), 1, filePtr );
	fread( &bmp_file_header.bfReserved2, sizeof(uint16_t), 1, filePtr );
	fread( &bmp_file_header.bfOffBits, sizeof(uint32_t), 1, filePtr );

	if( bmp_file_header.bfType != BITMAP_ID )
	{
		fclose(filePtr);
		*bitmap = NULL;
		return false;
	}

	fread( info_header, sizeof(bitmap_info_header_t), 1, filePtr );	
	bytesPerPixel = info_header->biBitCount >> 3;
	scanlineBytes = info_header->biWidth * bytesPerPixel;
	stride = (info_header->biWidth * bytesPerPixel + 3) & ~3;
	fseek( filePtr, bmp_file_header.bfOffBits, SEEK_SET );

	bitmapSize = scanlineBytes * info_header->biHeight;	
	*bitmap = (uint8_t*) malloc( bitmapSize );

#ifdef NDEBUG
	memset( *bitmap, 0, bitmapSize );
#endif
	
	/* check if allocation failed... */
	if( *bitmap == NULL )
	{
		fclose( filePtr );
		return false;
	}

	for( imageIdx = 0; imageIdx < bitmapSize; imageIdx += scanlineBytes )
	{
		// read in pixels and skip padding...
		fread( *bitmap + imageIdx, scanlineBytes, 1, filePtr );		
		fseek( filePtr, stride - scanlineBytes, SEEK_CUR );
	}
	
	convertBGRtoRGB( info_header->biWidth, info_header->biHeight, bytesPerPixel, *bitmap );

	fclose( filePtr );
	return true;
}

bool imageio_bitmap_save( const char* filename, uint32_t width, uint32_t height, uint32_t bitsPerPixel, uint8_t* imageData )
{
	FILE* filePtr;
	bitmap_file_header_t bmp_file_header;
	bitmap_info_header_t info_header;
	uint8_t nullByte;
	uint32_t imageIdx = 0;
	uint32_t bitmapIdx = 0;
	uint32_t bytesPerPixel = bitsPerPixel >> 3;
	uint32_t stride = (width * bytesPerPixel + 3) & ~3;
	uint32_t scanlineBytes = width * bytesPerPixel;

	if( (filePtr = fopen(filename, "wb")) == NULL )
	{
		return false;
	}

	/* Define the bmp_file_header */
	bmp_file_header.bfSize = sizeof(bitmap_file_header_t);
	bmp_file_header.bfType = BITMAP_ID;
	bmp_file_header.bfReserved1 = 0;
	bmp_file_header.bfReserved2 = 0;
	bmp_file_header.bfOffBits = sizeof(bitmap_file_header_t) + sizeof(bitmap_info_header_t);
	/* Define the info_header */
	info_header.biSize = sizeof(bitmap_info_header_t);
	info_header.biPlanes = 1;
	info_header.biBitCount = bitsPerPixel;	
	info_header.biCompression = BI_RGB;		/* No compression */
	info_header.biSizeImage = stride * height;  /* w * h * (4 bytes) */
	info_header.biXPelsPerMeter = 0;
	info_header.biYPelsPerMeter = 0;
	info_header.biClrUsed = 0;
	info_header.biClrImportant = 0;
	info_header.biWidth = width;
	info_header.biHeight = height;

	convertRGBtoBGR( width, height, bytesPerPixel, imageData );

	fwrite( &bmp_file_header, sizeof(bitmap_file_header_t), 1, filePtr );
	fwrite( &info_header, sizeof(bitmap_info_header_t), 1, filePtr );

	for( imageIdx = 0, bitmapIdx = 0; imageIdx <  info_header.biSizeImage; imageIdx += stride, bitmapIdx += scanlineBytes )
	{
		fwrite( imageData + bitmapIdx, scanlineBytes, 1, filePtr );
		fwrite( &nullByte, 1, stride - scanlineBytes, filePtr );
	}

	fclose( filePtr );
	return true;
}





bool imageio_targa_load( const char* filename, targa_file_header_t* p_file_header, uint8_t** bitmap )
{
	FILE* filePtr;
	uint32_t colorMode;		/* 4 for RGBA or 3 for RGB */
	uint32_t imageSize = 0;

	if( (filePtr = fopen( filename, "rb" )) == NULL )
	{
		return false;
	}

	/* read in first two bytes we don't need */
	fread( p_file_header, sizeof(targa_file_header_t), 1, filePtr );

	
	if( (p_file_header->imageTypeCode != 2) && (p_file_header->imageTypeCode != 3) )
	{
		fclose( filePtr );
		return false;
	}

	/* colorMode-> 3 = BGR, 4 = BGRA */
	colorMode = p_file_header->bitCount >> 3; // bytes per pixel
	

	imageSize = p_file_header->width * p_file_header->height * colorMode;

	*bitmap = (uint8_t*) malloc( imageSize );
	
	/* check if allocation failed... */
	if( *bitmap == NULL )
	{
		return false;
	}

	fread( *bitmap, 1, imageSize, filePtr );

	/* indexed color mode not handled!!! */
	convertBGRtoRGB( p_file_header->width, p_file_header->height, colorMode, *bitmap );

	fclose( filePtr );
	return true;
}





bool imageio_targa_save( const char* filename, targa_file_header_t* p_file_header, uint8_t* bitmap )
{
	FILE* filePtr;
	long imageSize;		/* size of the Targa image */
	uint32_t colorMode = p_file_header->bitCount >> 3; /* 4 for RGBA or 3 for RGB */


	if( ( filePtr = fopen( filename, "wb" ) ) == NULL )
	{
		return false;
	}

	p_file_header->imageIDLength     = 0;
	p_file_header->imageTypeCode     = 2;
	p_file_header->colorMapOrigin    = 0;
	p_file_header->colorMapLength    = 0;
	p_file_header->colorMapEntrySize = 0;
	p_file_header->colorMapType      = 0;
	p_file_header->imageXOrigin      = 0;
	p_file_header->imageYOrigin      = 0;
	p_file_header->imageDescriptor   = colorMode == 4 ? 0x08 : 0x00;

	assert( p_file_header->imageTypeCode == 0x2 || p_file_header->imageTypeCode == 0x3 ); // must be 2 or 3
	
	fwrite( p_file_header, sizeof(targa_file_header_t), 1, filePtr );
	
	convertRGBtoBGR( p_file_header->width, p_file_header->height, colorMode, bitmap );

	imageSize = p_file_header->width * p_file_header->height * colorMode;	
	fwrite( bitmap, imageSize, 1, filePtr );

	fclose( filePtr );
	return true;
}

/*
 *  Image stretching functions...
 */
void imageio_image_resize( uint32_t srcWidth, uint32_t srcHeight, const uint8_t* srcBitmap,
                           uint32_t dstWidth, uint32_t dstHeight, uint8_t* dstBitmap, uint32_t bitsPerPixel,
                           resize_algorithm_t algorithm )
{
	uint32_t byteCount = bitsPerPixel >> 3;
	assert( srcBitmap != NULL || dstBitmap != NULL );
	assert( srcWidth != 0 || srcHeight != 0 );
	assert( byteCount > 2 );

	switch( algorithm )
	{
		case ALG_NEARESTNEIGHBOR:
			/* forces both bitmaps to have the same bit depth */
			imageio_resize_nearest_neighbor( srcWidth, srcHeight, bitsPerPixel, srcBitmap,
				             dstWidth, dstHeight, bitsPerPixel, dstBitmap );
			break;
		case ALG_BILINEAR:
			switch( byteCount )
			{
				case 0:
				case 1:
				case 2:
					assert( false ); /* unsupported bpp's */
					break;
				case 3:
						imageio_resize_bilinear_rgb( srcWidth, srcHeight, srcBitmap, dstWidth, dstHeight, dstBitmap, byteCount );
						break;
				case 4:
						imageio_resize_bilinear_rgba( srcWidth, srcHeight, srcBitmap, dstWidth, dstHeight, dstBitmap, byteCount );
						break;
				default: break;
			}
			break;
		case ALG_BILINEAR_SHARPER:
			switch( byteCount )
			{
				case 0:
				case 1:
				case 2:
					assert( false ); /* unsupported bpp's */
					break;
				case 3:
						imageio_resize_bilinear_sharper_rgb( srcWidth, srcHeight, srcBitmap, dstWidth, dstHeight, dstBitmap, byteCount );
						break;
				case 4:
						imageio_resize_bilinear_sharper_rgba( srcWidth, srcHeight, srcBitmap, dstWidth, dstHeight, dstBitmap, byteCount );
						break;
				default: break;
			}
			break;
		case ALG_BICUBIC: 
			switch( byteCount )
			{
				case 0:
				case 1:
				case 2:
					assert( false ); /* unsupported bpp's */
					break;
				case 3:
						imageio_resize_bicubic_rgb( srcWidth, srcHeight, srcBitmap, dstWidth, dstHeight, dstBitmap, byteCount );
						break;
				case 4:
						imageio_resize_bicubic_rgba( srcWidth, srcHeight, srcBitmap, dstWidth, dstHeight, dstBitmap, byteCount );
						break;
				default: break;
			}
			break;
		default:
			assert( false ); /* bad algorithm... */
	}
}

/* uses nearest neighbor... */
void imageio_resize_nearest_neighbor( uint32_t srcWidth, uint32_t srcHeight, uint32_t srcBitsPerPixel, const uint8_t* srcBitmap,
                                      uint32_t dstWidth, uint32_t dstHeight, uint32_t dstBitsPerPixel, uint8_t* dstBitmap ) 
{
	uint32_t srcByteCount = srcBitsPerPixel >> 3;
	uint32_t dstByteCount = dstBitsPerPixel >> 3;
	register uint32_t x = 0;
	register uint32_t y = 0;
	float horizontalStretchFactor = 0;
	float verticalStretchFactor = 0;
	/* uint32_t srcY, srcX;  don't remove */
	uint32_t dstPos, srcPos;
	register uint32_t srcYtimesWidth;
	register uint32_t yTimesWidth;

	assert( srcBitsPerPixel != 0 || dstBitsPerPixel != 0 );
	assert( srcBitmap != NULL || dstBitmap != NULL );
	assert( srcWidth != 0 || srcHeight != 0 );
	horizontalStretchFactor = (float) srcWidth / (float) dstWidth;
	verticalStretchFactor = (float) srcHeight / (float) dstHeight;
	
	/* don't remove...
	 * x_in = x_out * (w_in / w_out), using nearest neighbor
	 */
	for( y = 0; y < dstHeight; y++ )
	{
		srcYtimesWidth = ((uint32_t)(y * verticalStretchFactor)) * srcWidth * srcByteCount;
		yTimesWidth = y * dstWidth * dstByteCount;

		for( x = 0; x < dstWidth; x++ )
		{
			dstPos = yTimesWidth + x * dstByteCount;
			srcPos = srcYtimesWidth + ((uint32_t)(x * horizontalStretchFactor)) * srcByteCount;		

			memcpy( &dstBitmap[ dstPos ], &srcBitmap[ srcPos ], dstByteCount * sizeof(uint8_t) );
		}
	}

}



/* bi-linear: nearest neighbor with bilinear interpolation */
void imageio_resize_bilinear_rgba( uint32_t srcWidth, uint32_t srcHeight, const uint8_t* srcBitmap,
                                   uint32_t dstWidth, uint32_t dstHeight, uint8_t* dstBitmap,
                                   uint32_t byteCount )
{
	register uint32_t x = 0;
	register uint32_t y = 0;
	float horizontalStretchFactor = 0;
	float verticalStretchFactor = 0;
	register uint32_t srcY = 0;
	register uint32_t srcX = 0;
	register uint32_t dstPos,
		 neighborPos1,
		 neighborPos2,
		 neighborPos3,
		 neighborPos4;
	register uint8_t n1R, n1G, n1B, n1A,	/* color vectors */
				  n2R, n2G, n2B, n2A,
				  n3R, n3G, n3B, n3A,
				  n4R, n4G, n4B, n4A;
	uint32_t largestSrcIndex = srcWidth * srcHeight * byteCount;

	assert( byteCount == 4 );

	horizontalStretchFactor = (float) srcWidth / (float) dstWidth;  /* stretch vector */
	verticalStretchFactor = (float) srcHeight / (float) dstHeight;

	for( y = 0; y < dstHeight; y++ )
	{
		srcY = (uint32_t) (y * verticalStretchFactor);
		if( srcY == 0 ) srcY = 1;

		for( x = 0; x < dstWidth; x++ )
		{
			srcX = (uint32_t) (x * horizontalStretchFactor);
			dstPos = y * dstWidth * byteCount + x * byteCount;

			neighborPos1 = (srcY) * srcWidth * byteCount + (srcX) * byteCount;
			neighborPos2 = (srcY) * srcWidth * byteCount + (srcX+1) * byteCount;
			neighborPos3 = (srcY+1) * srcWidth * byteCount + (srcX) * byteCount;
			neighborPos4 = (srcY+1) * srcWidth * byteCount + (srcX+1) * byteCount;
			if( neighborPos1 >= largestSrcIndex ) neighborPos1 = largestSrcIndex - byteCount;
			if( neighborPos2 >= largestSrcIndex ) neighborPos2 = largestSrcIndex - byteCount;
			if( neighborPos3 >= largestSrcIndex ) neighborPos3 = largestSrcIndex - byteCount;
			if( neighborPos4 >= largestSrcIndex ) neighborPos4 = largestSrcIndex - byteCount;

			/* source pixel... */
			n1R = srcBitmap[ neighborPos1 ];
			n1G = srcBitmap[ neighborPos1 + 1 ];
			n1B = srcBitmap[ neighborPos1 + 2 ];
			n1A = srcBitmap[ neighborPos1 + 3 ];

			/* source pixel's neighbor2... */
			n2R = srcBitmap[ neighborPos2 ];
			n2G = srcBitmap[ neighborPos2 + 1 ];
			n2B = srcBitmap[ neighborPos2 + 2 ];
			n2A = srcBitmap[ neighborPos2 + 3 ];
				
			/* source pixel's neighbor3... */
			n3R = srcBitmap[ neighborPos3 ];
			n3G = srcBitmap[ neighborPos3 + 1 ];
			n3B = srcBitmap[ neighborPos3 + 2 ];
			n3A = srcBitmap[ neighborPos3 + 3 ];

			/* source pixel's neighbor4... */
			n4R = srcBitmap[ neighborPos4 ];
			n4G = srcBitmap[ neighborPos4 + 1 ];
			n4B = srcBitmap[ neighborPos4 + 2 ];
			n4A = srcBitmap[ neighborPos4 + 3 ];

			dstBitmap[ dstPos ] = (uint8_t) bilerp( 0.5, 0.5, n4R, n3R, n2R, n1R );
			dstBitmap[ dstPos + 1 ] = (uint8_t) bilerp( 0.5, 0.5, n4G, n3G, n2G, n1G );
			dstBitmap[ dstPos + 2 ] = (uint8_t) bilerp( 0.5, 0.5, n4B, n3B, n2B, n1B );
			dstBitmap[ dstPos + 3 ] = (uint8_t) bilerp( 0.5, 0.5, n4A, n3A, n2A, n1A );
		}
	}
}

void imageio_resize_bilinear_sharper_rgba( uint32_t srcWidth, uint32_t srcHeight, const uint8_t* srcBitmap,
									uint32_t dstWidth, uint32_t dstHeight, uint8_t* dstBitmap,
									uint32_t byteCount )
{
	register uint32_t x = 0;
	register uint32_t y = 0;
	float horizontalStretchFactor = 0;
	float verticalStretchFactor = 0;
	register uint32_t srcY = 0;
	register uint32_t srcX = 0;
	register uint32_t dstPos, srcPos,
		 neighborPos1,
		 neighborPos2,
		 neighborPos3,
		 neighborPos4;
	register uint8_t R, G, B, A,
			 n1R, n1G, n1B, n1A,	/* color vectors */
			 n2R, n2G, n2B, n2A,
			 n3R, n3G, n3B, n3A,
			 n4R, n4G, n4B, n4A;
	uint32_t largestSrcIndex = srcWidth * srcHeight * byteCount;

	assert( byteCount == 4 );

	horizontalStretchFactor = (float) srcWidth / (float) dstWidth;  /* stretch vector */
	verticalStretchFactor = (float) srcHeight / (float) dstHeight;

	

	for( y = 0; y < dstHeight; y++ )
	{
		srcY = (uint32_t) (y * verticalStretchFactor);
		if( srcY == 0 ) srcY = 1;

		for( x = 0; x < dstWidth; x++ )
		{
			srcX = (uint32_t) (x * horizontalStretchFactor);
			dstPos = y * dstWidth * byteCount + x * byteCount;

			neighborPos1 = (srcY-1) * srcWidth * byteCount + (srcX-1) * byteCount;
			neighborPos2 = (srcY-1) * srcWidth * byteCount + (srcX+1) * byteCount;
			neighborPos3 = (srcY+1) * srcWidth * byteCount + (srcX-1) * byteCount;
			neighborPos4 = (srcY+1) * srcWidth * byteCount + (srcX+1) * byteCount;
			srcPos = (uint32_t) srcY * srcWidth * byteCount + srcX * byteCount;

			if( neighborPos1 < 0 ) neighborPos1 = 0;
			if( neighborPos2 < 0 ) neighborPos2 = 0;
			if( neighborPos3 < 0 ) neighborPos3 = 0;
			if( neighborPos4 < 0 ) neighborPos4 = 0;
			if( neighborPos1 >= largestSrcIndex ) neighborPos1 = largestSrcIndex - byteCount;
			if( neighborPos2 >= largestSrcIndex ) neighborPos2 = largestSrcIndex - byteCount;
			if( neighborPos3 >= largestSrcIndex ) neighborPos3 = largestSrcIndex - byteCount;
			if( neighborPos4 >= largestSrcIndex ) neighborPos4 = largestSrcIndex - byteCount;

			/* source pixel... */
			R = srcBitmap[ srcPos ];
			G = srcBitmap[ srcPos + 1 ];
			B = srcBitmap[ srcPos + 2 ];
			A = srcBitmap[ srcPos + 3 ];

			/* source pixel's neighbor1... */
			n1R = srcBitmap[ neighborPos1 ];
			n1G = srcBitmap[ neighborPos1 + 1 ];
			n1B = srcBitmap[ neighborPos1 + 2 ];
			n1A = srcBitmap[ neighborPos1 + 3 ];

			/* source pixel's neighbor2... */
			n2R = srcBitmap[ neighborPos2 ];
			n2G = srcBitmap[ neighborPos2 + 1 ];
			n2B = srcBitmap[ neighborPos2 + 2 ];
			n2A = srcBitmap[ neighborPos2 + 3 ];
				
			/* source pixel's neighbor3... */
			n3R = srcBitmap[ neighborPos3 ];
			n3G = srcBitmap[ neighborPos3 + 1 ];
			n3B = srcBitmap[ neighborPos3 + 2 ];
			n3A = srcBitmap[ neighborPos3 + 3 ];

			/* source pixel's neighbor4... */
			n4R = srcBitmap[ neighborPos4 ];
			n4G = srcBitmap[ neighborPos4 + 1 ];
			n4B = srcBitmap[ neighborPos4 + 2 ];
			n4A = srcBitmap[ neighborPos4 + 3 ];

			/* comes out a little more burry than I expected, so I lerp between R and the 
			 * output from the bilerp of the other 4 samples.
			 */
			dstBitmap[ dstPos ] = (uint8_t) lerp( 0.5, bilerp( 0.5, 0.5, n4R, n3R, n2R, n1R ), R );
			dstBitmap[ dstPos + 1 ] = (uint8_t) lerp( 0.5, bilerp( 0.5, 0.5, n4G, n3G, n2G, n1G ), G );
			dstBitmap[ dstPos + 2 ] = (uint8_t) lerp( 0.5, bilerp( 0.5, 0.5, n4B, n3B, n2B, n1B ), B );
			dstBitmap[ dstPos + 3 ] = (uint8_t) lerp( 0.5, bilerp( 0.5, 0.5, n4A, n3A, n2A, n1A ), A );
		}
	}
}

void imageio_resize_bilinear_rgb( uint32_t srcWidth, uint32_t srcHeight, const uint8_t* srcBitmap,
				  uint32_t dstWidth, uint32_t dstHeight, uint8_t* dstBitmap,
				  uint32_t byteCount )
{
	register uint32_t x = 0;
	register uint32_t y = 0;
	float horizontalStretchFactor = 0;
	float verticalStretchFactor = 0;
	register uint32_t srcY = 0;
	register uint32_t srcX = 0;
	register uint32_t dstPos, 
                  neighborPos1,
                  neighborPos2,
                  neighborPos3,
                  neighborPos4;
	register uint8_t n1R, n1G, n1B, 	/* color vectors */
                  n2R, n2G, n2B, 
                  n3R, n3G, n3B, 
                  n4R, n4G, n4B;

	assert( byteCount == 3 );

	horizontalStretchFactor = (float) srcWidth / (float) dstWidth;  /* stretch vector */
	verticalStretchFactor = (float) srcHeight / (float) dstHeight;

	
	for( y = 0; y < dstHeight; y++ )
	{
		srcY = (uint32_t) (y * verticalStretchFactor);
		assert( srcY < srcHeight );

		for( x = 0; x < dstWidth; x++ )
		{
			srcX = (uint32_t) (x * horizontalStretchFactor);
			dstPos = y * dstWidth * byteCount + x * byteCount;

			neighborPos1 = (srcY) * srcWidth * byteCount + (srcX) * byteCount;
			neighborPos2 = (srcY) * srcWidth * byteCount + (srcX+1) * byteCount;
			neighborPos3 = (srcY+1) * srcWidth * byteCount + (srcX) * byteCount;
			neighborPos4 = (srcY+1) * srcWidth * byteCount + (srcX+1) * byteCount;

			/* source pixel... */
			n1R = srcBitmap[ neighborPos1 ];
			n1G = srcBitmap[ neighborPos1 + 1 ];
			n1B = srcBitmap[ neighborPos1 + 2 ];

			/* source pixel's neighbor2... */
			n2R = srcBitmap[ neighborPos2 ];
			n2G = srcBitmap[ neighborPos2 + 1 ];
			n2B = srcBitmap[ neighborPos2 + 2 ];
				
			/* source pixel's neighbor3... */
			n3R = srcBitmap[ neighborPos3 ];
			n3G = srcBitmap[ neighborPos3 + 1 ];
			n3B = srcBitmap[ neighborPos3 + 2 ];

			/* source pixel's neighbor4... */
			n4R = srcBitmap[ neighborPos4 ];
			n4G = srcBitmap[ neighborPos4 + 1 ];
			n4B = srcBitmap[ neighborPos4 + 2 ];

			dstBitmap[ dstPos ]     = (uint8_t) bilerp( 0.5, 0.5, n4R, n3R, n2R, n1R );
			dstBitmap[ dstPos + 1 ] = (uint8_t) bilerp( 0.5, 0.5, n4G, n3G, n2G, n1G );
			dstBitmap[ dstPos + 2 ] = (uint8_t) bilerp( 0.5, 0.5, n4B, n3B, n2B, n1B );
		}
	}
}


void imageio_resize_bilinear_sharper_rgb( uint32_t srcWidth, uint32_t srcHeight, const uint8_t* srcBitmap,
								   uint32_t dstWidth, uint32_t dstHeight, uint8_t* dstBitmap,
								   uint32_t byteCount )
{
	register uint32_t x = 0;
	register uint32_t y = 0;
	float horizontalStretchFactor = 0;
	float verticalStretchFactor = 0;
	register uint32_t srcY = 0;
	register uint32_t srcX = 0;
	register uint32_t dstPos, srcPos,
		 neighborPos1,
		 neighborPos2,
		 neighborPos3,
		 neighborPos4;
	register uint8_t R, G, B,
			 n1R, n1G, n1B, 	/* color vectors */
			 n2R, n2G, n2B, 
			 n3R, n3G, n3B, 
			 n4R, n4G, n4B;
	uint32_t largestSrcIndex = srcWidth * srcHeight * byteCount;

	assert( byteCount == 3 );

	horizontalStretchFactor = (float) srcWidth / (float) dstWidth;  /* stretch vector */
	verticalStretchFactor   = (float) srcHeight / (float) dstHeight;
	
	for( y = 0; y < dstHeight; y++ )
	{
		srcY = (uint32_t) (y * verticalStretchFactor);
		assert( srcY < srcHeight );

		for( x = 0; x < dstWidth; x++ )
		{
			srcX = (uint32_t) (x * horizontalStretchFactor);
			dstPos = y * dstWidth * byteCount + x * byteCount;

			neighborPos1 = (srcY-1) * srcWidth * byteCount + (srcX-1) * byteCount;
			neighborPos2 = (srcY-1) * srcWidth * byteCount + (srcX+1) * byteCount;
			neighborPos3 = (srcY+1) * srcWidth * byteCount + (srcX-1) * byteCount;
			neighborPos4 = (srcY+1) * srcWidth * byteCount + (srcX+1) * byteCount;
			srcPos = (uint32_t) srcY * srcWidth * byteCount + srcX * byteCount;

			if( neighborPos1 < 0 ) neighborPos1 = 0;
			if( neighborPos2 < 0 ) neighborPos2 = 0;
			if( neighborPos3 < 0 ) neighborPos3 = 0;
			if( neighborPos4 < 0 ) neighborPos4 = 0;
			if( neighborPos1 >= largestSrcIndex ) neighborPos1 = largestSrcIndex - byteCount;
			if( neighborPos2 >= largestSrcIndex ) neighborPos2 = largestSrcIndex - byteCount;
			if( neighborPos3 >= largestSrcIndex ) neighborPos3 = largestSrcIndex - byteCount;
			if( neighborPos4 >= largestSrcIndex ) neighborPos4 = largestSrcIndex - byteCount;

			/* source pixel... */
			R = srcBitmap[ srcPos ];
			G = srcBitmap[ srcPos + 1 ];
			B = srcBitmap[ srcPos + 2 ];

			/* source pixel's neighbor1... */
			n1R = srcBitmap[ neighborPos1 ];
			n1G = srcBitmap[ neighborPos1 + 1 ];
			n1B = srcBitmap[ neighborPos1 + 2 ];

			/* source pixel's neighbor2... */
			n2R = srcBitmap[ neighborPos2 ];
			n2G = srcBitmap[ neighborPos2 + 1 ];
			n2B = srcBitmap[ neighborPos2 + 2 ];
				
			/* source pixel's neighbor3... */
			n3R = srcBitmap[ neighborPos3 ];
			n3G = srcBitmap[ neighborPos3 + 1 ];
			n3B = srcBitmap[ neighborPos3 + 2 ];

			/* source pixel's neighbor4... */
			n4R = srcBitmap[ neighborPos4 ];
			n4G = srcBitmap[ neighborPos4 + 1 ];
			n4B = srcBitmap[ neighborPos4 + 2 ];

			/* comes out a little more burry than I expected, so I lerp between R and the 
			 * output from the bilerp of the other 4 samples.
			 */
			dstBitmap[ dstPos ]     = (uint8_t) lerp( 0.5, bilerp( 0.5, 0.5, n4R, n3R, n2R, n1R ), R );
			dstBitmap[ dstPos + 1 ] = (uint8_t) lerp( 0.5, bilerp( 0.5, 0.5, n4G, n3G, n2G, n1G ), G );
			dstBitmap[ dstPos + 2 ] = (uint8_t) lerp( 0.5, bilerp( 0.5, 0.5, n4B, n3B, n2B, n1B ), B );
		}
	}
}

#define P(x)	( (x) > 0 ? (x) : 0 )
#define R(x)	( (1.0/6) * (P((x)+2) * P((x)+2) * P((x)+2) - 4.0 * P((x)+1) * P((x)+1) * P((x)+1) + 6.0 * P((x)) * P((x)) * P(x) - 4.0 * P((x)-1) * P((x)-1) * P((x)-1)) )

void imageio_resize_bicubic_rgba( uint32_t srcWidth, uint32_t srcHeight, const uint8_t* srcBitmap,
							 uint32_t dstWidth, uint32_t dstHeight, uint8_t* dstBitmap,
							 uint32_t byteCount )
{
	register uint32_t x = 0;
	register uint32_t y = 0;
	register int m = 0, n = 0, i = 0, j = 0;
	float horizontalStretchFactor = 0;
	float verticalStretchFactor = 0;
	register float srcY = 0;
	register float srcX = 0;
	register float dy = 0;
	register float dx = 0;
	register uint32_t dstPos, srcPos;
	uint32_t largestSrcIndex = srcWidth * srcHeight * byteCount;
	register uint32_t sumR = 0;
	register uint32_t sumG = 0;
	register uint32_t sumB = 0;
	register uint32_t sumA = 0;

	assert( byteCount == 4 );

	horizontalStretchFactor = (float) srcWidth / (float) dstWidth;  /* stretch vector */
	verticalStretchFactor = (float) srcHeight / (float) dstHeight;

	

	for( y = 0; y < dstHeight; y++ )
	{
		srcY = y * verticalStretchFactor;
		j = (int) floor( srcY );
		if( srcY == 0 ) srcY = 1;
		

		for( x = 0; x < dstWidth; x++ )
		{
			srcX = x * horizontalStretchFactor;
			i = (int) floor( srcX );
			
			dstPos = y * dstWidth * byteCount + x * byteCount;
			sumR = 0;
			sumG = 0;
			sumB = 0;
			sumA = 0;
			dy = srcY - j;
			dx = srcX - i;
			
			for( m = -1; m <= 2; m++ )
				for( n = -1; n <= 2; n++ )
				{
					srcPos = pixel_index( i + m, j + n, byteCount, srcWidth );
					if( srcPos > largestSrcIndex ) srcPos = largestSrcIndex - byteCount;

					sumR +=	(uint32_t) srcBitmap[ srcPos ] * R( m - dx ) * R( dy - n );
					sumG +=	(uint32_t) srcBitmap[ srcPos + 1 ] * R( m - dx ) * R( dy - n );
					sumB +=	(uint32_t) srcBitmap[ srcPos + 2 ] * R( m - dx ) * R( dy - n );
					sumA +=	(uint32_t) srcBitmap[ srcPos + 3 ] * R( m - dx ) * R( dy - n );
				}

			dstBitmap[ dstPos ]     = (uint8_t) sumR;
			dstBitmap[ dstPos + 1 ] = (uint8_t) sumG;
			dstBitmap[ dstPos + 2 ] = (uint8_t) sumB;
			dstBitmap[ dstPos + 3 ] = (uint8_t) sumA;
		}
	}
}

void imageio_resize_bicubic_rgb( uint32_t srcWidth, uint32_t srcHeight, const uint8_t* srcBitmap,
							uint32_t dstWidth, uint32_t dstHeight, uint8_t* dstBitmap,
							uint32_t byteCount )
{
	register uint32_t x = 0;
	register uint32_t y = 0;
	register int m = 0, n = 0, i = 0, j = 0;
	float horizontalStretchFactor = 0;
	float verticalStretchFactor = 0;
	register float srcY = 0;
	register float srcX = 0;
	register float dy = 0;
	register float dx = 0;
	register uint32_t dstPos, srcPos;
	uint32_t largestSrcIndex = srcWidth * srcHeight * byteCount;
	register uint32_t sumR = 0;
	register uint32_t sumG = 0;
	register uint32_t sumB = 0;
	assert( byteCount == 4 );

	horizontalStretchFactor = (float) srcWidth / (float) dstWidth;  /* stretch vector */
	verticalStretchFactor = (float) srcHeight / (float) dstHeight;

	

	for( y = 0; y < dstHeight; y++ )
	{
		srcY = y * verticalStretchFactor;
		j = (int) floor( srcY );
		if( srcY == 0 ) srcY = 1;
		

		for( x = 0; x < dstWidth - 2; x++ )
		{
			srcX = x * horizontalStretchFactor;
			i    = (int) floor( srcX );
			
			dstPos = y * dstWidth * byteCount + x * byteCount;
			sumR   = 0;
			sumG   = 0;
			sumB   = 0;
			dy     = srcY - j;
			dx     = srcX - i;
			
			for( m = -1; m <= 2; m++ )
			{
				for( n = -1; n <= 2; n++ )
				{
					srcPos = pixel_index( i + m, j + n, byteCount, srcWidth );
					if( srcPos > largestSrcIndex ) srcPos = largestSrcIndex - byteCount;

					sumR +=	(uint32_t) srcBitmap[ srcPos ] * R( m - dx ) * R( dy - n );
					sumG +=	(uint32_t) srcBitmap[ srcPos + 1 ] * R( m - dx ) * R( dy - n );
					sumB +=	(uint32_t) srcBitmap[ srcPos + 2 ] * R( m - dx ) * R( dy - n );
				}
			}

			dstBitmap[ dstPos ]     = (uint8_t) sumR;
			dstBitmap[ dstPos + 1 ] = (uint8_t) sumG;
			dstBitmap[ dstPos + 2 ] = (uint8_t) sumB;
		}
	}
}

/*
 *	Swap Red and blue colors in RGB abd RGBA functions
 */
void imageio_swap_red_and_blue( uint32_t width, uint32_t height, uint32_t byteCount, uint8_t* bitmap ) /* RGB to BGR */
{
	register uint32_t imageIdx;
	register uint32_t imageSize = width * height * byteCount;
	assert( byteCount != 0 );
	assert( bitmap != NULL );

	if( byteCount > 2 ) /* 32 bpp or 24 bpp */
	{	
		for( imageIdx = 0; imageIdx < imageSize; imageIdx += byteCount )
		{
			/* fast swap using XOR... */
			bitmap[ imageIdx ]     = bitmap[ imageIdx ] ^ bitmap[ imageIdx + 2 ];
			bitmap[ imageIdx + 2 ] = bitmap[ imageIdx + 2 ] ^ bitmap[ imageIdx ];
			bitmap[ imageIdx ]     = bitmap[ imageIdx ] ^ bitmap[ imageIdx + 2 ]; 
		}
	}
	else { /* 16 bpp */
		/* Swap ARRRRRGGGGGBBBBB to GGGBBBBBARRRRRGG, whereeach R,G,B, A is a bit, or... */
		/* Swap GGGBBBBBARRRRRGG to ARRRRRGGGGGBBBBB, whereeach R,G,B, A is a bit */
		for( imageIdx = 0; imageIdx < imageSize; imageIdx += byteCount )
		{			
			bitmap[ imageIdx ]     = bitmap[ imageIdx + 1 ];
			bitmap[ imageIdx + 1 ] = bitmap[ imageIdx ];
		}
	}

}

/*
 *  Image Flipping Routines. These are slow... :(
 */
void imageio_flip_horizontally( uint32_t width, uint32_t height, uint32_t byteCount, uint8_t* bitmap )
{
	register uint32_t x = 0;
	register uint32_t y = 0;
	uint8_t* srcBitmap = (uint8_t*) malloc( sizeof(uint8_t) * width * height * byteCount );

	memcpy( srcBitmap, bitmap, sizeof(uint8_t) * width * height * byteCount );
	
	for( y = 0; y < height; y++ )
		for( x = 0; x < width; x++ )
			memcpy( &bitmap[ pixel_index(width - x - 1, y, byteCount, width) ], &srcBitmap[ pixel_index(x, y, byteCount, width) ], sizeof(uint8_t) * byteCount );

	free( srcBitmap );
}

void imageio_flip_vertically( uint32_t width, uint32_t height, uint32_t byteCount, uint8_t* bitmap )
{
	register uint32_t x = 0;
	register uint32_t y = 0;
	uint8_t* srcBitmap = (uint8_t*) malloc( sizeof(uint8_t) * width * height * byteCount );

	memcpy( srcBitmap, bitmap, sizeof(uint8_t) * width * height * byteCount );
	
	for( y = 0; y < height; y++ )
		for( x = 0; x < width; x++ )
			memcpy( &bitmap[ pixel_index(x, height - y - 1, byteCount, width) ], &srcBitmap[ pixel_index(x, y, byteCount, width) ], sizeof(uint8_t) * byteCount );

	free( srcBitmap );
}

void imageio_flip_horizontally_nocopy( uint32_t width, uint32_t height, const uint8_t* srcBitmap, uint8_t* dstBitmap, uint32_t byteCount )
{
	register uint32_t j, i;
	register uint32_t j_times_width = 0;
	register uint8_t* temp = (uint8_t*) malloc( width * byteCount * sizeof(uint8_t) ); /* temp scan line, just in case src = dst */

	for( j = 0; j < height; j++ )
	{
		j_times_width = j* width * byteCount; /* avoids doing this twice */
		
		memcpy( &temp[ 0 ], &srcBitmap[ j_times_width ], width * byteCount * sizeof(uint8_t) );

		for( i = 0; i < width; i++ )
			memcpy( &dstBitmap[ j_times_width + (width - 1 - i) * byteCount ], &temp[ i * byteCount ], byteCount * sizeof(uint8_t) );
	}

	free( temp );
}

void imageio_flip_vertically_nocopy( uint32_t width, uint32_t height, const uint8_t* srcBitmap, uint8_t* dstBitmap, uint32_t byteCount )
{
	register uint32_t j, i;
	register uint32_t i_times_bytecount = 0;
	register uint32_t width_times_bytecount = 0;
	register uint8_t* temp = (uint8_t*) malloc( height * byteCount * sizeof(uint8_t) ); /* temp column line, just in case src = dst */

	for( i = 0; i < width; i++ )
	{
		i_times_bytecount = i * byteCount;
		width_times_bytecount = width * byteCount;

		for( j = 0; j < height; j++ )
			memcpy( &temp[ j * byteCount ], &srcBitmap[ j * width_times_bytecount + i_times_bytecount ], byteCount * sizeof(uint8_t) );

		for( j = 0; j < height; j++ )
			memcpy( &dstBitmap[ (height - 1 - j) * width_times_bytecount + i_times_bytecount ], &temp[ j * byteCount ], byteCount * sizeof(uint8_t) );
	}

	free( temp );
}



/*
 * Edge Detection: k is the maximum color distance that signifies an edge
 */
void imageio_detect_edges( uint32_t width, uint32_t height, uint32_t bitsPerPixel, const uint8_t* srcBitmap, uint8_t* dstBitmap, uint32_t k )
{
	uint32_t byteCount = bitsPerPixel >> 3; /* 4 ==> RGBA32, 3==>RGB32 , 2 ==> RGB16 */
	register uint32_t y;
	register uint32_t x;
	assert( bitsPerPixel != 0 );
	assert( srcBitmap != NULL || dstBitmap != NULL );

	for( y = 0; y < height - 1; y++ )
		for( x = 0; x < width - 1; x++ )
		{
			/* r = right, b = bottom */
			uint32_t pos = width * y * byteCount + x * byteCount;
			uint32_t rpos = width * y * byteCount + (x + 1) * byteCount;
			uint32_t bpos = width * (y + 1) * byteCount + x * byteCount;

			/* this pixel */
			uint8_t R = srcBitmap[ pos ];
			uint8_t G = srcBitmap[ pos + 1 ];
			uint8_t B = srcBitmap[ pos + 2 ];

			/* right neighbor */
			uint8_t rR = srcBitmap[ rpos ];
			uint8_t rG = srcBitmap[ rpos + 1 ];
			uint8_t rB = srcBitmap[ rpos + 2 ];

			/* bottom neighbor */
			uint8_t bR = srcBitmap[ bpos ];
			uint8_t bG = srcBitmap[ bpos + 1 ];
			uint8_t bB = srcBitmap[ bpos + 2 ];
			
			/*
			uint32_t D1 = sqrt( (R-rR)*(R-rR) + (G-rG)*(G-rG) + (B-rB)*(B-rB) );
			uint32_t D2 = sqrt( (R-bR)*(R-bR) + (G-bG)*(G-bG) + (B-bB)*(B-bB) );
			*/
			
			/* if any of the distances are greater than k, put a white pixel in the destination */
			if( (R-rR)*(R-rR) + (G-rG)*(G-rG) + (B-rB)*(B-rB) >= k*k ||
				(R-bR)*(R-bR) + (G-bG)*(G-bG) + (B-bB)*(B-bB) >= k*k ) /* if( D1 > k || D2 > k ) */
			{
				dstBitmap[ pos ]     = 0xFF;
				dstBitmap[ pos + 1 ] = 0xFF;
				dstBitmap[ pos + 2 ] = 0xFF;
			}
			else { /* otherwise place a black pixel; */
				dstBitmap[ pos ] = 0x00;
				dstBitmap[ pos + 1 ] = 0x00;
				dstBitmap[ pos + 2 ] = 0x00;			
			}

		}
}

/*
 * Color Extraction: Marks white all the pixels that are no greater than k distance to the color.
 */
void imageio_extract_color( uint32_t width, uint32_t height, uint32_t bitsPerPixel, const uint8_t* srcBitmap, uint8_t* dstBitmap, const uint32_t color, uint32_t k )
{
	uint32_t byteCount = bitsPerPixel >> 3; /* 4 ==> RGBA32, 3==>RGB32 , 2 ==> RGB16 */
	register uint32_t y;
	register uint32_t x;
	assert( bitsPerPixel != 0 );
	assert( srcBitmap != NULL || dstBitmap != NULL );

	for( y = 0; y < height; y++ )
		for( x = 0; x < width; x++ )
		{
			uint32_t pos = width * y * byteCount + x * byteCount;
			/* this pixel */
			uint8_t r_diff = srcBitmap[ pos     ] - r32(color);
			uint8_t g_diff = srcBitmap[ pos + 1 ] - g32(color);
			uint8_t b_diff = srcBitmap[ pos + 2 ] - b32(color);

			if( r_diff * r_diff + g_diff * g_diff + b_diff * b_diff <= k*k ) /*if( sqrt( (R-color->r) + (G-color->g) + (B-color->b) ) <= k )*/
			{
				dstBitmap[ pos ] = 0xFF;
				dstBitmap[ pos + 1 ] = 0xFF;
				dstBitmap[ pos + 2 ] = 0xFF;
			}
			else {
				dstBitmap[ pos ] = 0x00;
				dstBitmap[ pos + 1 ] = 0x00;
				dstBitmap[ pos + 2 ] = 0x00;
			}
		}
}

/*
 * Grayscale conversion
 */
void imageio_convert_to_grayscale( uint32_t width, uint32_t height, uint32_t bitsPerPixel, const uint8_t* srcBitmap, uint8_t* dstBitmap )
{
	uint32_t byteCount = bitsPerPixel >> 3; /* 4 ==> RGBA32, 3==>RGB32 , 2 ==> RGB16 */
	register uint32_t y;
	register uint32_t x;
	assert( bitsPerPixel != 0 );
	assert( srcBitmap != NULL || dstBitmap != NULL );

	for( y = 0; y < height; y++ )
		for( x = 0; x < width; x++ )
		{
			uint32_t pos = width * y * byteCount + x * byteCount;
			/* this pixel */
			uint8_t R = srcBitmap[ pos ];
			uint8_t G = srcBitmap[ pos + 1 ];
			uint8_t B = srcBitmap[ pos + 2 ];

			uint32_t colorAverage = (R + G + B) / 3;
			
			dstBitmap[ pos ] = colorAverage;
			dstBitmap[ pos + 1 ] = colorAverage;
			dstBitmap[ pos + 2 ] = colorAverage;
		}
}

/*
 * Colorscale conversion
 */
void imageio_convert_to_colorscale( uint32_t width, uint32_t height, uint32_t bitsPerPixel, const uint8_t* srcBitmap, uint8_t* dstBitmap, const uint32_t color )
{
	uint32_t byteCount = bitsPerPixel >> 3; /* 4 ==> RGBA32, 3==>RGB32 , 2 ==> RGB16 */
	register uint32_t y;
	register uint32_t x;
	uint8_t r = r32(color);
	uint8_t g = g32(color);
	uint8_t b = b32(color);
	assert( (r != 0 && g != 0 && b != 0) ||
			(r != 0 || g != 0 || b != 0) ); /* no black, because 0 vector */
	assert( srcBitmap != NULL || dstBitmap != NULL );
	assert( bitsPerPixel != 0 );

	for( y = 0; y < height; y++ )
		for( x = 0; x < width; x++ )
		{
			uint32_t pos = width * y * byteCount + x * byteCount;
			/* this pixel */
			uint8_t R = srcBitmap[ pos ];
			uint8_t G = srcBitmap[ pos + 1 ];
			uint8_t B = srcBitmap[ pos + 2 ];

			float colorScaled = (float) ( (R*r + G*g + B*b) / ( sqrt((float) R*R + G*G + B*B) * sqrt((float) r*r + g*g + b*b) ));
			
			dstBitmap[ pos ] = (uint8_t) colorScaled * R;
			dstBitmap[ pos + 1 ] = (uint8_t) colorScaled * G;
			dstBitmap[ pos + 2 ] = (uint8_t) colorScaled * B;
		}
}

/*
 * Light or contrast modification
 */
void imageio_modify_contrast( uint32_t width, uint32_t height, uint32_t bitsPerPixel, const uint8_t* srcBitmap, uint8_t* dstBitmap, int contrast )
{
	uint32_t byteCount = bitsPerPixel >> 3; /* 4 ==> RGBA32, 3==>RGB32 , 2 ==> RGB16 */
	register uint32_t y;
	register uint32_t x;
	register uint32_t colorIndex;
	uint32_t transform[ 256 ];
	assert( srcBitmap != NULL || dstBitmap != NULL );
	assert( bitsPerPixel != 0 );

	for( colorIndex = 0; colorIndex < 256; colorIndex++ )
	{
		float slope = (float) tan((float)contrast);
		if( colorIndex < (uint32_t) (128.0f + 128.0f*slope) && colorIndex > (uint32_t) (128.0f-128.0f*slope) )
			transform[ colorIndex ] = (uint32_t) ((colorIndex - 128) / slope + 128);
		else if( colorIndex >= (uint32_t) (128.0f + 128.0f*slope) )
			transform[ colorIndex ] = 255;
		else /* colorIndex <= 128 -128*slope */
			transform[ colorIndex ] = 0;
	}

	
	for( y = 0; y < height; y++ )
		for( x = 0; x < width; x++ )
		{
			uint32_t pos = width * y * byteCount + x * byteCount;
			/* this pixel */
			uint8_t R = srcBitmap[ pos ];
			uint8_t G = srcBitmap[ pos + 1 ];
			uint8_t B = srcBitmap[ pos + 2 ];

			dstBitmap[ pos ] = transform[ R ];
			dstBitmap[ pos + 1 ] = transform[ G ];
			dstBitmap[ pos + 2 ] = transform[ B ];
		}
}

void imageio_modify_brightness( uint32_t width, uint32_t height, uint32_t bitsPerPixel, const uint8_t* srcBitmap, uint8_t* dstBitmap, int brightness )
{
	uint32_t byteCount = bitsPerPixel >> 3; /* 4 ==> RGBA32, 3==>RGB32 , 2 ==> RGB16 */
	register uint32_t y;
	register uint32_t x;
	register unsigned short colorIndex;
	uint8_t transform[ 256 ] = {0};
	assert( srcBitmap != NULL || dstBitmap != NULL );
	assert( bitsPerPixel != 0 );

	for( colorIndex = 0; colorIndex < 256; colorIndex++ )
	{
		short t = colorIndex + brightness;
		if( t > 255 ) t = 255;
		if( t < 0 )	t = 0;		
		transform[ colorIndex ] = (uint8_t) t;
	}

	
	for( y = 0; y < height; y++ )
	{
		for( x = 0; x < width; x++ )
		{
			uint32_t pos = width * y * byteCount + x * byteCount;
			/* this pixel */
			uint8_t R = srcBitmap[ pos ];
			uint8_t G = srcBitmap[ pos + 1 ];
			uint8_t B = srcBitmap[ pos + 2 ];

			dstBitmap[ pos ] = transform[ R ];
			dstBitmap[ pos + 1 ] = transform[ G ];
			dstBitmap[ pos + 2 ] = transform[ B ];
		}
	}
}
