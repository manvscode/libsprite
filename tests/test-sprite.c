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
static void handle_event   ( const SDL_Event* e );


typedef struct entity {
	sprite_t* sprite;
	sprite_player_t* sp;

	int orientation;
	vec3_t position;
	vec2_t speed;
	vec2_t target_speed;
} entity_t;

static void entity_initialize ( entity_t* e, const char* filename );
static void entity_update     ( entity_t* e, const uint32_t delta );
static void entity_render     ( entity_t* e );
static void sprite_render     ( const sprite_frame_t* frame );

SDL_Window* window = NULL;
SDL_GLContext ctx = NULL;

GLuint vao = 0;
GLuint vbo_vertices = 0;
GLuint vbo_tex_coords = 0;

GLuint program                                =  0;
GLint attribute_vertex                        = -1;
GLint attribute_tex_coord                     = -1;
GLint uniform_model_view                      = -1;
GLint uniform_texture                         = -1;
GLint uniform_sprite_frame_position           = -1;
GLint uniform_sprite_frame_frame_dimensions   = -1;
GLint uniform_sprite_frame_texture_dimensions = -1;
GLint uniform_orientation                     = -1;
GLuint texture                                =  0;
static uint8_t keys[ SDL_NUM_SCANCODES ];

static entity_t robot;
static bool exiting         = false;
static uint32_t delta       = 0;

static const GLfloat sprite_mesh[] = {
	-0.5f, -0.5f,
	 0.5f, -0.5f,
	-0.5f,  0.5f,
	-0.5f,  0.5f,
	 0.5f, -0.5f,
	 0.5f,  0.5f,
};

static const GLfloat sprite_uvs[] = {
	0.0f, 0.0f,  // bottom-left
	1.0f, 0.0f,  // bottom-right
	0.0f, 1.0f,
	0.0f, 1.0f,
	1.0f, 0.0f,
	1.0f, 1.0f,
};



int main( int argc, char* argv[] )
{
	if( SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO) < 0 )
	{
		goto quit;
	}

	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 2 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );

	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 8 );
	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 1 );
	SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, 2 );

	int flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
	//flags |= SDL_WINDOW_FULLSCREEN;
	window = SDL_CreateWindow( "Test Robot Sprite", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, flags );


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

	while( !exiting )
	{
		SDL_PollEvent( &e );      // Check for events.
		handle_event( &e );
		render( );
	}

	deinitialize( );

quit:
	if( ctx ) SDL_GL_DeleteContext( ctx );
	if( window ) SDL_DestroyWindow( window );
	SDL_Quit( );
	return 0;
}

void handle_event( const SDL_Event* e )
{
	switch( e->type )
	{
		case SDL_KEYDOWN:
		{
			uint8_t scancode = e->key.keysym.scancode;
			keys[ scancode ] = 1;

			switch( scancode )
			{
    			case SDL_SCANCODE_ESCAPE:
					exiting = true;
					break;
				default:
					break;
			}
			break;
		}
		case SDL_KEYUP:
		{
			uint16_t scancode = e->key.keysym.scancode;
			keys[ scancode ] = 0;
			break;
		}

		case SDL_QUIT:
			break;
		default:
			break;
	}
}

void initialize( void )
{
	entity_initialize( &robot, "./tests/robot.spr" );
	dump_gl_info( );
	glClearColor( 0.0f, 0.0f, 0.0f, 1.0f );

	glEnable( GL_DEPTH_TEST );
	glEnable( GL_CULL_FACE );
	glCullFace( GL_BACK );
	GL_ASSERT_NO_ERROR( );
	glPointSize( 4.0 );

	glEnable( GL_LINE_SMOOTH );
	glEnable( GL_POLYGON_SMOOTH );


	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	texture = tex_create( );

	if( texture )
	{
		GLubyte flags = TEX_CLAMP_S | TEX_CLAMP_T;
		glActiveTexture( GL_TEXTURE0 );
		tex_setup_texture( texture, sprite_width(robot.sprite), sprite_height(robot.sprite), 0, sprite_bit_depth(robot.sprite), sprite_pixels(robot.sprite), GL_NEAREST, GL_NEAREST, flags, 2 );
		GL_ASSERT_NO_ERROR( );
	}

	GLchar* shader_log  = NULL;
	GLchar* program_log = NULL;
	const shader_info_t shaders[] = {
		{ GL_VERTEX_SHADER,   "./tests/sprite-shader.v.glsl" },
		{ GL_FRAGMENT_SHADER, "./tests/sprite-shader.f.glsl" }
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
		exit( EXIT_FAILURE );
	}

	assert( program > 0 );

	attribute_vertex                        = glsl_bind_attribute( program, "a_vertex" );
	attribute_tex_coord                     = glsl_bind_attribute( program, "a_tex_coord" );
	uniform_model_view                      = glsl_bind_uniform( program, "u_model_view" );
	uniform_texture                         = glsl_bind_uniform( program, "u_texture" );
	uniform_sprite_frame_position           = glsl_bind_uniform( program, "u_sprite_frame.position" );
	uniform_sprite_frame_frame_dimensions   = glsl_bind_uniform( program, "u_sprite_frame.frame_dimensions" );
	uniform_sprite_frame_texture_dimensions = glsl_bind_uniform( program, "u_sprite_frame.texture_dimensions" );
	uniform_orientation                     = glsl_bind_uniform( program, "u_orientation" );
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
	tex_destroy( texture );
	glDeleteVertexArrays( 1, &vao );
	glDeleteBuffers( 1, &vbo_vertices );
	glDeleteBuffers( 1, &vbo_tex_coords );
	glDeleteProgram( program );
	sprite_destroy( &robot.sprite );
}

static inline float framerate( uint32_t time_in_ms )
{
	return (1000.0f) / time_in_ms;
}

void render( )
{
	uint32_t now = SDL_GetTicks( );
	static uint32_t last_render = 0;
	delta = now - last_render;
	last_render = now;

	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

	entity_render( &robot );
	entity_update( &robot, delta );

	SDL_GL_SwapWindow( window );

	printf( "fps %.2f\n", framerate(delta) );
}

void dump_sdl_error( void )
{
	const char* sdl_error = SDL_GetError( );

	if( sdl_error && *sdl_error != '\0' )
	{
		fprintf( stderr, "[SDL] %s\n", sdl_error );
	}
}

void entity_initialize( entity_t* e, const char* sprite_file )
{
	e->sprite      = sprite_from_file( sprite_file );
	e->sp          = sprite_player_create( e->sprite );
	assert( e->sprite );
	e->orientation = 1;
	e->position.x  = 0.0f;
	e->position.y  = 0.0f;
	e->position.z  = 0.0f;
	e->speed.x     = 0.0f;
	e->speed.y     = 0.0f;
	e->target_speed.x = 0.0f;
	e->target_speed.y = 0.0f;

	sprite_player_set_timer( SDL_GetTicks );
	sprite_player_play( e->sp, "idle" );
}

void entity_update( entity_t* e, const uint32_t delta )
{
	if( !sprite_player_is_playing( e->sp, "jump" ) )
	{
		e->speed.y = 0.0f;

		if( e->position.y > 0.0 )
		{
			e->position.y -= 0.01f * delta;
		}
	}


	if( keys[ SDL_SCANCODE_A ] )
	{
		robot.orientation = -1;

		if( keys[ SDL_SCANCODE_LSHIFT ] )
		{
			robot.target_speed.x = 0.006f;
			sprite_player_play( robot.sp, "run" );
		}
		else
		{
			robot.target_speed.x = 0.003f;
			sprite_player_play( robot.sp, "walk" );
		}
	}
	else if( keys[ SDL_SCANCODE_D ] )
	{
		robot.orientation = 1;

		if( keys[ SDL_SCANCODE_LSHIFT ] )
		{
			robot.target_speed.x = 0.006f;
			sprite_player_play( robot.sp, "run" );
		}
		else
		{
			robot.target_speed.x = 0.003f;
			sprite_player_play( robot.sp, "walk" );
		}
	}
	else if( keys[ SDL_SCANCODE_W ] )
	{
		sprite_player_play( robot.sp, "climb" );
	}
	else
	{
		robot.target_speed.x = 0.0f;
		sprite_player_play( robot.sp, "idle" );
	}

	if( keys[ SDL_SCANCODE_SPACE ] )//&& !sprite_player_is_playing( robot.sp, "jump") )
	{
		sprite_player_play( robot.sp, "jump" );
		robot.target_speed.y = 0.05f;
	}
	else
	{
		robot.target_speed.y = 0.0f;
	}

	robot.speed.x = lerp( 0.6f, robot.speed.x, robot.target_speed.x );
	robot.speed.y = lerp( 0.2f, robot.speed.y, robot.target_speed.y );

	e->position.x += e->orientation * e->speed.x * delta;
	e->position.y += e->speed.y * delta;

}

void entity_render( entity_t* e )
{
	/* render the sprite */
	#if 1
	const sprite_frame_t* frame = sprite_player_frame( e->sp );
	if( frame )
	{
		sprite_render( frame );
	}
	#else
	sprite_player_render( e->sp, sprite_render );
	#endif
}

void sprite_render( const sprite_frame_t* frame )
{
	assert( frame );
	int width; int height;
	SDL_GetWindowSize( window, &width, &height );
	GLfloat aspect = ((GLfloat)width) / height;
	//vec3_t translation = VEC3_VECTOR( 0.0, 0.0, -10 );
	mat4_t projection = orthographic( -5.0*aspect, 5.0*aspect, -2.0, 9.0, -10.0, 10.0 );

	mat4_t transform = MAT4_IDENTITY;// ;translate( &translation );
	mat4_t position = translate( &robot.position );
	transform = mat4_mult_matrix( &transform, &position );
	mat4_t model_view = mat4_mult_matrix( &projection, &transform );


	glUseProgram( program );

	glEnableVertexAttribArray( attribute_vertex );
	glEnableVertexAttribArray( attribute_tex_coord );
	glUniformMatrix4fv( uniform_model_view, 1, GL_FALSE, model_view.m );
	glUniform1ui( uniform_texture, texture );

	glUniform2f( uniform_sprite_frame_position, frame->x, frame->y );
	glUniform2f( uniform_sprite_frame_frame_dimensions, frame->width, frame->height );
	glUniform2f( uniform_sprite_frame_texture_dimensions, sprite_width(robot.sprite), sprite_height(robot.sprite) );
	glUniform1i( uniform_orientation, robot.orientation );

	glBindTexture( GL_TEXTURE_2D, texture );
	glBindVertexArray( vao );
	glDrawArrays( GL_TRIANGLES, 0, sizeof(sprite_mesh) / sizeof(sprite_mesh[0]) );

	glDisableVertexAttribArray( attribute_vertex );
	glDisableVertexAttribArray( attribute_tex_coord );
}
