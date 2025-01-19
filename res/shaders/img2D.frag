#version 330 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D image; // The texture sampler

void main() {
    FragColor = texture(image, TexCoords); // Sample the texture using the passed texture coordinates
}
