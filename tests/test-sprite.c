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
#include <SDL2/SDL.h>
#include <libsimplegl/simplegl.h>
#include "../src/sprite.h"

static void initialize     ( void );
static void deinitialize   ( void );
static void render         ( void );
static void dump_sdl_error ( void );

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_GLContext ctx = NULL;

GLuint vao = 0;
GLuint vbo_vertices = 0;
GLuint vbo_tex_coords = 0;

GLuint program             =  0;
GLint attribute_vertex     = -1;
GLint attribute_tex_coord  = -1;
GLint uniform_model_view   = -1;
GLint uniform_texture      = -1;
GLint uniform_sprite_frame_position = -1;
GLint uniform_sprite_frame_frame_dimensions = -1;
GLint uniform_sprite_frame_texture_dimensions = -1;
GLuint texture             =  0;

sprite_t* sprite = NULL;


static GLfloat sprite_mesh[] = {
	-0.5f, -0.5f,
	 0.5f, -0.5f,
	-0.5f,  0.5f,
	-0.5f,  0.5f,
	 0.5f, -0.5f,
	 0.5f,  0.5f,
};

static GLfloat sprite_uvs[] = {
	0.0f, 0.0f,  // bottom-left
	1.0f, 0.0f,  // bottom-right
	0.0f, 1.0f,
	0.0f, 1.0f,
	1.0f, 0.0f,
	1.0f, 1.0f,
};


int main( int argc, char* argv[] )
{
	if( SDL_Init(SDL_INIT_VIDEO) < 0 )
	{
		goto quit;
	}

	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 2 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );

	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );

	int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
	//flags |= SDL_WINDOW_FULLSCREEN;
	window = SDL_CreateWindow( "Test Shaders", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, flags );

	renderer = SDLCALL SDL_CreateRenderer( window, -1, SDL_RENDERER_ACCELERATED );

	if( window == NULL )
	{
		dump_sdl_error( );
		goto quit;
	}

	ctx = SDL_GL_CreateContext( window );

	if( !ctx )
	{
		dump_sdl_error( );
		goto quit;
	}

	initialize( );

	SDL_Event e;

	while( e.type != SDL_KEYDOWN && e.type != SDL_QUIT )
	{
		SDL_PollEvent( &e );      // Check for events.
		render( );
	}

	deinitialize( );

quit:
	if( ctx ) SDL_GL_DeleteContext( ctx );
	if( renderer ) SDL_DestroyRenderer( renderer );
	if( window ) SDL_DestroyWindow( window );
	SDL_Quit( );
	return 0;
}

void initialize( void )
{
	sprite = sprite_from_file( "./tests/robot.spr" );
	dump_gl_info( );
	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );

	glDisable( GL_DEPTH_TEST );
	glDisable( GL_CULL_FACE );
	glCullFace( GL_BACK );
	GL_ASSERT_NO_ERROR( );
	glPointSize( 4.0 );

	//glEnable( GL_LINE_SMOOTH );
	//glEnable( GL_POLYGON_SMOOTH );


	//glEnable( GL_BLEND );
	//glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	texture = tex2d_create( );

	if( texture )
	{
		glActiveTexture( GL_TEXTURE0 );
		tex2d_setup_texture ( texture, sprite_width(sprite), sprite_height(sprite), sprite_bit_depth(sprite), sprite_pixels(sprite), GL_NEAREST, GL_NEAREST, true );
		GL_ASSERT_NO_ERROR( );
	}

	GLchar* shader_log  = NULL;
	GLchar* program_log = NULL;
	const shader_info_t shaders[] = {
		{ GL_VERTEX_SHADER,   "/Users/manvscode/projects/libsprite/tests/sprite-shader.v.glsl" },
		{ GL_FRAGMENT_SHADER, "/Users/manvscode/projects/libsprite/tests/sprite-shader.f.glsl" }
	};

	if( !glsl_program_from_shaders( &program, shaders, shader_info_count(shaders), &shader_log, &program_log ) )
	{
		GL_ASSERT_NO_ERROR( );
		if( shader_log )
		{
			printf( " [Shader Log] %s\n", shader_log );
			free( shader_log );
		}
		if( program_log )
		{
			printf( "[Program Log] %s\n", program_log );
			free( program_log );
		}

		printf( "Shaders did not compile and link!!\n" );
		return;
	}

	assert( program > 0 );

	attribute_vertex                        = glsl_bind_attribute( program, "a_vertex" );
	attribute_tex_coord                     = glsl_bind_attribute( program, "a_tex_coord" );
	uniform_model_view                      = glsl_bind_uniform( program, "u_model_view" );
	uniform_texture                         = glsl_bind_uniform( program, "u_texture" );
	uniform_sprite_frame_position           = glsl_bind_uniform( program, "u_sprite_frame.position" );
	uniform_sprite_frame_frame_dimensions   = glsl_bind_uniform( program, "u_sprite_frame.frame_dimensions" );
	uniform_sprite_frame_texture_dimensions = glsl_bind_uniform( program, "u_sprite_frame.texture_dimensions" );
	assert( uniform_sprite_frame_texture_dimensions  >= 0 );



	buffer_create( &vbo_vertices, sprite_mesh, sizeof(GLfloat), sizeof(sprite_mesh) / sizeof(sprite_mesh[0]), GL_ARRAY_BUFFER, GL_STATIC_DRAW );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );

	buffer_create( &vbo_tex_coords, sprite_uvs, sizeof(GLfloat), sizeof(sprite_uvs) / sizeof(sprite_uvs[0]), GL_ARRAY_BUFFER, GL_STATIC_DRAW );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );





	glGenVertexArrays( 1, &vao );
	glBindVertexArray( vao );
	glBindBuffer( GL_ARRAY_BUFFER, vbo_vertices );
	glVertexAttribPointer( attribute_vertex, 2, GL_FLOAT, GL_FALSE, 0, 0 );
	glEnableVertexAttribArray( attribute_vertex );
	glBindBuffer( GL_ARRAY_BUFFER, vbo_tex_coords );
	glVertexAttribPointer( attribute_tex_coord, 2, GL_FLOAT, GL_FALSE, 0, 0 );
	glEnableVertexAttribArray( attribute_tex_coord );


	//glDisableVertexAttribArray( attribute_vertex );
	//GL_ASSERT_NO_ERROR( );

	//glDisableVertexAttribArray( attribute_tex_coord );
	//GL_ASSERT_NO_ERROR( );


	int width; int height;
	SDL_GetWindowSize( window, &width, &height );
	assert( width > 0  && height > 0 );
	glViewport(0, 0, width, height );
	GL_ASSERT_NO_ERROR( );
}

void deinitialize( void )
{
	tex2d_destroy( texture );
	glDeleteVertexArrays( 1, &vao );
	glDeleteBuffers( 1, &vbo_vertices );
	glDeleteBuffers( 1, &vbo_tex_coords );
	glDeleteProgram( program );
	sprite_destroy( &sprite );
}

void render( )
{
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

	static float angle = 0.0;
	if( angle >= 360.0f ) angle = 0.0f;

	int width; int height;
	SDL_GetWindowSize( window, &width, &height );
	GLfloat aspect = ((GLfloat)width) / height;
	vec3_t translation = VEC3_VECTOR( 0.0, 0.0, -10 );
	mat4_t projection = orthographic( -3.0*aspect, 3.0*aspect, -3.0, 3.0, -10.0, 10.0 );
	mat4_t rotation = rotate_xyz( "yx", angle, 0.0 );
	mat4_t transform = translate( &translation );
	transform = mat4_mult_matrix( &transform, &rotation );
	mat4_t model_view = mat4_mult_matrix( &projection, &transform );


	glUseProgram( program );

	glEnableVertexAttribArray( attribute_vertex );
	glEnableVertexAttribArray( attribute_tex_coord );
	glUniformMatrix4fv( uniform_model_view, 1, GL_FALSE, model_view.m );
	glUniform1ui( uniform_texture, texture );

	static size_t state_index = 2;
	const char* state_names[] = {
		"walk",
		"run",
		"climb",
		"jump"
	};
	static size_t frame_index = 1;



	const sprite_state_t* state = sprite_state( sprite, state_names[state_index] );
	if( frame_index >= sprite_state_frame_count(state) )
	{
		state_index++;


		if( state_index >= 4 )
			state_index = 0;


		frame_index = 0;

		SDL_Delay( 500 );

		state = sprite_state( sprite, state_names[state_index] );
	}
	const sprite_frame_t* frame = sprite_state_frame( state, frame_index );


	glUniform2f( uniform_sprite_frame_position, frame->x, frame->y );
	glUniform2f( uniform_sprite_frame_frame_dimensions, frame->width, frame->height );
	glUniform2f( uniform_sprite_frame_texture_dimensions, sprite_width(sprite), sprite_height(sprite) );

	glBindTexture( GL_TEXTURE_2D, texture );
	glBindVertexArray( vao );
	glDrawArrays( GL_TRIANGLES, 0, sizeof(sprite_mesh) / sizeof(sprite_mesh[0]) );

	glDisableVertexAttribArray( attribute_vertex );
	glDisableVertexAttribArray( attribute_tex_coord );

	SDL_Delay( frame->time );


	SDL_GL_SwapWindow( window );

	frame_index++;
}

void dump_sdl_error( void )
{
	const char* sdl_error = SDL_GetError( );

	if( sdl_error && *sdl_error != '\0' )
	{
		fprintf( stderr, "[SDL] %s\n", sdl_error );
	}
}
