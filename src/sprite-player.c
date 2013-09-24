#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <SDL2/SDL.h>
#include <libcollections/array.h>
#include <libcollections/vector.h>
#include "sprite.h"

//#define USE_ANIMATION_STACK

struct sprite_player {
	const sprite_t* sprite;
	const sprite_state_t* initial_state;
	const sprite_state_t* state;
	uint16_t frame_index;
	uint16_t loop_count;
	uint32_t last_frame_time;
	sprite_render_fxn render;
	bool is_playing;
	void* user_data;

	#ifdef USE_ANIMATION_STACK
	size_t stack_position;
	lc_pstack_t stack;
	#endif
};

static void  sprite_player_initialize( sprite_player_t* sp, const sprite_t* sprite, const char* initial_state, sprite_render_fxn render );
static void  sprite_player_goto_initial_state( sprite_player_t* sp );
#ifdef USE_ANIMATION_STACK
static void  sprite_player_push( sprite_player_t* sp );
static void  sprite_player_pop( sprite_player_t* sp );
#endif


sprite_player_t* sprite_player_create( const sprite_t* sprite, const char* initial_state, sprite_render_fxn render )
{
	sprite_player_t* sp = (sprite_player_t*) malloc( sizeof(sprite_player_t) );

	if( sp )
	{
		sprite_player_initialize( sp, sprite, initial_state, render );
	}

	return sp;
}

void sprite_player_destroy( sprite_player_t** sp )
{
	if( *sp )
	{
		#ifdef USE_ANIMATION_STACK
		pstack_destroy( &(*sp)->stack );
		#endif
		free( *sp );
		*sp = NULL;
	}
}


void sprite_player_initialize( sprite_player_t* sp, const sprite_t* sprite, const char* initial_state, sprite_render_fxn render )
{
	assert( sp );
	assert( sprite );

	sp->sprite          = sprite;
	sp->render          = render;
	sp->user_data       = NULL;

	#ifdef USE_ANIMATION_STACK
	sp->stack_position  = 0;
	pstack_create( &sp->stack, SPRITE_ANIMATION_STACK_DEPTH, malloc, free );
	#endif

	sprite_player_play( sp, initial_state );
	sp->initial_state   = sp->state;
}

void sprite_player_set_user_data( sprite_player_t* sp, const void* data )
{
	assert( sp );
	sp->user_data = (void*) data;
}

void sprite_player_goto_initial_state( sprite_player_t* sp )
{
	sp->state       = sp->initial_state;
	sp->frame_index = 0;
	sp->loop_count  = sprite_state_loop_count( sp->initial_state ) - 1;
	sp->is_playing  = true;

	assert( sp->loop_count >= 0 );
}

void sprite_player_play( sprite_player_t* sp, const char* name )
{
	const sprite_state_t* state = sprite_state( sp->sprite, name );
	assert( state );
	sprite_player_play_state( sp, state );
}

void sprite_player_play_state( sprite_player_t* sp, const sprite_state_t* state )
{
	if( state != sp->state )
	{
		#ifdef USE_ANIMATION_STACK
		sprite_player_push( sp );
		#endif

		sp->state           = state;
		sp->frame_index     = 0;
		sp->last_frame_time = SDL_GetTicks( );
		sp->loop_count      = sprite_state_loop_count( state ) - 1;
		sp->is_playing      = true;
		assert( sp->loop_count >= 0 );
	}
}

bool sprite_player_is_playing( sprite_player_t* sp, const char* name )
{
	assert( sp );
	return sp->is_playing && strcmp( sprite_state_name(sp->state), name ) == 0;
}

void sprite_player_stop( sprite_player_t* sp )
{
	assert( sp );
	sp->is_playing = false;
}

void sprite_player_pause( sprite_player_t* sp )
{
	assert( sp );
	sp->is_playing = false;
}

void sprite_player_unpause( sprite_player_t* sp )
{
	assert( sp );
	sp->is_playing = true;
}


void sprite_player_render( sprite_player_t* sp )
{
	assert( sp && sp->state );

	if( sp->frame_index >= sprite_state_frame_count(sp->state) )
	{
		sp->frame_index = 0;

 		uint16_t states_loop_count = sprite_state_loop_count(sp->state);

 		if( states_loop_count > 0 )
		{
			if( sp->loop_count > 0 )
			{
				sp->loop_count--;
			}
			else
			{
				/* TODO: go to initial state or previous state? */
				#ifdef USE_ANIMATION_STACK
				sprite_player_pop( sp );
				#else
				sprite_player_goto_initial_state( sp );
				#endif
			}
		}
	}


	const sprite_frame_t* frame = sprite_state_frame( sp->state, sp->frame_index );
	assert( frame );
	uint32_t now = SDL_GetTicks( );
	int32_t diff = now - sp->last_frame_time;

	sp->render( frame );

	if( sp->is_playing && diff > frame->time )
	{
		/* Show the next sprite frame */
		sp->frame_index++;
		sp->last_frame_time = now;
	}
}

#ifdef USE_ANIMATION_STACK
void sprite_player_push( sprite_player_t* sp )
{
	if( (sp->stack_position + 1) < SPRITE_ANIMATION_STACK_DEPTH )
	{
		pstack_push( &sp->stack, (void*) sp->state );
		sp->stack_position++;
	}
}

void sprite_player_pop( sprite_player_t* sp )
{
	if( sp->stack_position > 0 )
	{
		sp->state = (sprite_state_t*) pvector_peek( &sp->stack );
		pstack_pop( &sp->stack );
		sp->stack_position--;
	}
	else
	{
		sprite_player_goto_initial_state( sp );
	}
}
#endif
