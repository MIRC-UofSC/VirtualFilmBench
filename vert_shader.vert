#version 330

out vec2 vTexCoord;

layout (location = 0) in vec4 vertex;
layout (location = 1) in vec2 texCoord;
void main()
{

    vTexCoord = texCoord;
  gl_Position = vec4(vertex.xyz,1.0);

}
