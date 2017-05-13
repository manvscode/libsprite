#version 100

attribute vec3 a_vertex;
attribute vec3 a_normal;
attribute vec2 a_tex_coord;

varying mediump vec2 f_tex_coord;
varying mediump vec4 f_color;

uniform mat4 u_model;
uniform mat4 u_projection;
uniform int u_orientation;

void main( )
{
	/*
	mat3 scale;

	if( u_orientation > 0 )
	{
		scale = mat3( 1, 0, 0,
                      0, 1, 0,
		              0, 0, 1 );
	}
	else
	{
		scale = mat3( -1, 0, 0,
                       0, 1, 0,
		               0, 0, 1 );
	}

	gl_Position = u_projection * u_model * vec4( scale * a_vertex, 1.0 );
	*/

	gl_Position = u_projection * u_model * vec4( a_vertex, 1.0 );
	f_tex_coord = a_tex_coord;	
	f_color     = vec4(1.0, 1.0, 1.0, 1.0);
}
