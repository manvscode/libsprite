#version 150

struct sprite_frame {
	vec2 position;
	vec2 frame_dimensions;
	vec2 texture_dimensions;
};




in vec2 f_tex_coord;
in vec4 f_color;
uniform sampler2D u_texture;
uniform sprite_frame u_sprite_frame;
uniform int u_orientation;
out vec4 color;



void main( ) {

	vec2 oriented_tex_coord;

	if( u_orientation > 0 )
	{
		oriented_tex_coord = f_tex_coord;
	}
	else
	{
		oriented_tex_coord = vec2( 1.0 - f_tex_coord.s, f_tex_coord.t );
	}

	mat2 scale = mat2( (u_sprite_frame.frame_dimensions.x / u_sprite_frame.texture_dimensions.x), 0.0f,
					    0.0f, (u_sprite_frame.frame_dimensions.y / u_sprite_frame.texture_dimensions.y) );

	vec2 offset = vec2( u_sprite_frame.position.x / u_sprite_frame.texture_dimensions.x, u_sprite_frame.position.y / u_sprite_frame.texture_dimensions.y );

	vec2 sprite_tex_coord = offset + scale * oriented_tex_coord;
	color = texture( u_texture, sprite_tex_coord );
}
