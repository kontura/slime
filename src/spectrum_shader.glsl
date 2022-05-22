#version 430
layout(local_size_x=8, local_size_y=1) in;

layout(location = 0) uniform float dt;
layout(binding=0) buffer pblock { float spectrum_data[]; };

layout (location = 1, rgba8) readonly uniform image2D srcTex;
layout (location = 2, rgba8) writeonly uniform image2D destTex;
layout (location = 3) uniform int width;
layout (location = 4) uniform int height;

layout (location = 5) uniform int cCount;


float pserandom(vec2 uv) {
    float x = sin(dot(uv.xy, vec2(12.9898,78.233))) * 43758.5453123;
    return x - floor(x);
}


void main() {
    int index = int(gl_GlobalInvocationID.x);
    float height = spectrum_data[index] + 30.0;

    float m = mod(index, 100) * 0.1;
    vec4 color = vec4(1, m, 0, 1);
    imageStore(destTex, ivec2(index + 100.0, height), color);
}
