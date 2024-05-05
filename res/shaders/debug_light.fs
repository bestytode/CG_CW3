#version 330 core
out vec4 FragColor;

uniform bool use_red_color; 
uniform bool use_blue_color; 
uniform bool use_green_color;
uniform bool use_orange_color;

void main()
{
	if (use_red_color){
		FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
	}
	else if (use_blue_color){
		FragColor = vec4(0.0f, 0.0f, 1.0f, 1.0f);
	}
	else if (use_green_color){
		FragColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
	}
	else if (use_orange_color){
		FragColor = vec4(1.0f, 0.5f, 0.0f, 1.0f);
	}
	else {
		FragColor = vec4(1.0f);
	}
}