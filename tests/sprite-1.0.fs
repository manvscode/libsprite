#version 100

struct sprite_frame {
	mediump vec2 position;
	mediump vec2 frame_dimensions;
	mediump vec2 texture_dimensions;
};

varying mediump vec2 f_tex_coord;
varying mediump vec4 f_color;

uniform sampler2D u_texture;
uniform sprite_frame u_sprite_frame;


void main( )
{
	mediump mat2 scale = mat2( (u_sprite_frame.frame_dimensions.x / u_sprite_frame.texture_dimensions.x), 0.0,
					    0.0, (u_sprite_frame.frame_dimensions.y / u_sprite_frame.texture_dimensions.y) );

	mediump vec2 the_offset = vec2( u_sprite_frame.position.x / u_sprite_frame.texture_dimensions.x, u_sprite_frame.position.y / u_sprite_frame.texture_dimensions.y );

	mediump vec2 sprite_tex_coord = the_offset + scale * f_tex_coord;

	gl_FragColor = texture2D( u_texture, sprite_tex_coord ) * f_color;

	gl_FragColor = vec4( 0.0, 0.0, 0.0, 1.0 );
}
