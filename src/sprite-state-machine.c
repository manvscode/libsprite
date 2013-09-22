#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <SDL2/SDL.h>
#include "sprite.h"
#include "sprite-state-machine.h"


void sprite_state_machine_initialize( sprite_state_machine_t* ssm, const sprite_t* sprite, const char* initial_state, sprite_render_fxn render )
{
	assert( ssm );
	assert( sprite );

	ssm->sprite          = sprite;
	ssm->render          = render;
	ssm->user_data       = NULL;

	sprite_state_machine_transition( ssm, initial_state );
	ssm->initial_state   = ssm->state;
}

void sprite_state_machine_set_user_data( sprite_state_machine_t* ssm, const void* data )
{
	assert( ssm );
	ssm->user_data = (void*) data;
}

static void sprite_state_machine_goto_initial_state( sprite_state_machine_t* ssm )
{
	ssm->state       = ssm->initial_state;
	ssm->frame_index = 0;
	ssm->loop_count  = sprite_state_loop_count( ssm->initial_state ) - 1;
	ssm->is_playing  = true;

	assert( ssm->loop_count >= 0 );
}

void sprite_state_machine_transition( sprite_state_machine_t* ssm, const char* state_name )
{
	const sprite_state_t* state = sprite_state( ssm->sprite, state_name );
	assert( state );

	if( state != ssm->state )
	{
		ssm->state           = state;
		ssm->frame_index     = 0;
		ssm->last_frame_time = SDL_GetTicks( );
		ssm->loop_count      = sprite_state_loop_count( state ) - 1;
		ssm->is_playing      = true;
		assert( ssm->loop_count >= 0 );
	}
}

bool sprite_state_machine_is_state( sprite_state_machine_t* ssm, const char* state )
{
	assert( ssm );
	return strcmp( sprite_state_name(ssm->state), state ) == 0;
}

void sprite_state_machine_cancel( sprite_state_machine_t* ssm )
{
	assert( ssm );
	ssm->is_playing = false;
}

void sprite_state_machine_render( sprite_state_machine_t* ssm )
{
	if( ssm->frame_index >= sprite_state_frame_count(ssm->state) )
	{
		ssm->frame_index = 0;

 		uint16_t states_loop_count = sprite_state_loop_count(ssm->state);

 		if( states_loop_count > 0 )
		{
			if( ssm->loop_count > 0 )
			{
				ssm->loop_count--;
			}
			else
			{
				/* TODO: go to initial state or previous state? */
				sprite_state_machine_goto_initial_state( ssm );
			}
		}
	}


	const sprite_frame_t* frame = sprite_state_frame( ssm->state, ssm->frame_index );
	uint32_t now = SDL_GetTicks( );
	int32_t diff = now - ssm->last_frame_time;

	ssm->render( frame );

	if( ssm->is_playing && diff > frame->time )
	{
		/* Show the next sprite frame */
		ssm->frame_index++;
		ssm->last_frame_time = now;
	}
}
