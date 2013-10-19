#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <libcollections/array.h>
#include "sprite.h"
#include "sprite-mem.h"

static sprite_timer_fxn_t sprite_timer = NULL;

struct sprite_player {
	const sprite_t* sprite;
	const sprite_state_t* state;
	uint16_t frame_index;
	uint16_t loop_count;
	uint32_t last_frame_time;
	bool is_playing;
	sprite_timer_fxn_t timer;
	void* user_data;

};

static void  sprite_player_initialize( sprite_player_t* sp, const sprite_t* sprite );


sprite_player_t* sprite_player_create( const sprite_t* sprite )
{
	sprite_player_t* sp = (sprite_player_t*) sprite_alloc( sizeof(sprite_player_t) );

	if( sp )
	{
		sprite_player_initialize( sp, sprite );
	}

	return sp;
}

void sprite_player_destroy( sprite_player_t** sp )
{
	if( *sp )
	{
		sprite_free( *sp );
		*sp = NULL;
	}
}

void sprite_player_initialize( sprite_player_t* sp, const sprite_t* sprite )
{
	assert( sp );
	assert( sprite );

	sp->sprite     = sprite;
	sp->state      = NULL;
	sp->user_data  = NULL;
}

void sprite_player_set_timer( sprite_timer_fxn_t timer )
{
	assert( timer );
	sprite_timer = timer;
}

void sprite_player_set_user_data( sprite_player_t* sp, const void* data )
{
	assert( sp );
	sp->user_data = (void*) data;
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
		sp->state           = state;
		sp->frame_index     = 0;
		sp->last_frame_time = sprite_timer( );
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


void sprite_player_render( sprite_player_t* sp, sprite_render_fxn_t render )
{
	//assert( sp && sp->state );

	if( !sp->state ) return;

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
		}
	}


	const sprite_frame_t* frame = sprite_state_frame( sp->state, sp->frame_index );
	assert( frame );
	uint32_t now = sprite_timer( );
	int32_t diff = now - sp->last_frame_time;

	render( frame );

	if( sp->is_playing && diff > frame->time )
	{
		/* Show the next sprite frame */
		sp->frame_index++;
		sp->last_frame_time = now;
	}
}

const sprite_frame_t* sprite_player_frame( sprite_player_t* sp )
{
	//assert( sp && sp->state );

	if( !sp->state ) return NULL;

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
		}
	}


	const sprite_frame_t* frame = sprite_state_frame( sp->state, sp->frame_index );
	assert( frame );
	uint32_t now = sprite_timer( );
	int32_t diff = now - sp->last_frame_time;

	if( sp->is_playing && diff > frame->time )
	{
		/* Show the next sprite frame */
		sp->frame_index++;
		sp->last_frame_time = now;
	}

	return frame;
}
