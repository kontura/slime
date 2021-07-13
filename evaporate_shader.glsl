#version 430

layout(local_size_x=32, local_size_y=32) in;

layout(location = 0) uniform float dt;
layout(std430, binding=0) buffer pblock { vec4 positions[]; };
layout(std430, binding=1) buffer vblock { vec4 velocities[]; };

layout (location = 4, rgba8) readonly uniform image2D srcTex;
layout (location = 1, rgba8) writeonly uniform image2D destTex;

layout(location = 2) uniform float evaporate_speed;
layout(location = 3) uniform float diffuse_speed;
layout (location = 5) uniform int width;
layout (location = 6) uniform int height;

void main() {
    ivec2 id = ivec2(gl_GlobalInvocationID.xy);

    vec4 originalValue = imageLoad(srcTex, id);

    vec4 sum = vec4(0.0);
    for (int offsetX = -1; offsetX <= 1; offsetX++) {
        for (int offsetY = -1; offsetY <= 1; offsetY++) {
            int sampleX = id.x + offsetX;
            int sampleY = id.y + offsetY;

            if (sampleX >= 0 && sampleX <= width && sampleY >= 0 && sampleY <= height) {
                sum += imageLoad(srcTex, ivec2(sampleX, sampleY));
            }
        }
    }

    vec4 blurResult = sum/9;

    vec4 diffusedValue = mix(originalValue, blurResult, diffuse_speed * dt);

    vec4 evaporatedValue = max(vec4(0), diffusedValue - evaporate_speed * dt);

    imageStore(destTex, id, evaporatedValue);

}
