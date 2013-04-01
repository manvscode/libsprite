/*
 * Copyright (C) 2012 by Joseph A. Marrero and Shrewd LLC. http://www.manvscode.com/
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
#ifndef _TEXTURE_PACKER_H_
#define _TEXTURE_PACKER_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif 

struct texture_packer_image;
typedef struct texture_packer_image tp_image_t;


/*
 *  ------[ Texture Packer ]-------------------------------
 */
struct texture_packer;
typedef struct texture_packer tp_t;

tp_t*    texture_packer_create  ( void );
void     texture_packer_destroy ( tp_t** tp );
bool     texture_packer_add     ( tp_t* tp, uint16_t width, uint16_t height, uint8_t bytes_per_pixel, uint8_t* pixels );
void     texture_packer_clear   ( tp_t* tp );
bool     texture_packer_pack    ( tp_t* tp, uint16_t width, uint16_t height, uint8_t bytes_per_pixel );
uint16_t texture_packer_width   ( const tp_t* tp );
uint16_t texture_packer_height  ( const tp_t* tp );
uint8_t  texture_packer_bpp     ( const tp_t* tp );
uint8_t* texture_packer_pixels  ( const tp_t* tp );

#ifdef __cplusplus
}
#endif 
#endif /* _TEXTURE_PACKER_H_ */
