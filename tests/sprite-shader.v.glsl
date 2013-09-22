#version 150

in vec2 a_vertex;
in vec2 a_tex_coord;
out vec2 f_tex_coord;
out vec4 f_color;

uniform mat4 u_model_view;

void main( ) {
	gl_Position = u_model_view * vec4( a_vertex, 0.0, 1.0 );
	f_tex_coord = a_tex_coord;	
	f_color = vec4(1.0);
}
