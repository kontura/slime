#version 430

layout(local_size_x=40, local_size_y=40) in;

layout (location = 0, rgba8) readonly uniform image2D srcTex;
layout (location = 1, rgba8) writeonly uniform image2D destTex;

void main() {
    ivec2 id = ivec2(gl_GlobalInvocationID.xy);
    vec4 originalValue = imageLoad(srcTex, id);
    imageStore(destTex, id, originalValue);

}
