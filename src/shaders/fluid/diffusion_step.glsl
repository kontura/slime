//gauss seidel step shader for diffusion
//from https://www.youtube.com/watch?v=qsYE1wMEMPA

layout(local_size_x=20, local_size_y=20) in;
layout(binding=0) buffer pblock { Cell cells[]; };

layout (location = 0) uniform float dt;
layout (location = 1, rgba8) readonly uniform image2D srcTex;
layout (location = 2, rgba8) writeonly uniform image2D destTex;
layout (location = 3) uniform int width;
layout (location = 4) uniform int height;

float k = 2.11;

void main() {
    if (gl_GlobalInvocationID.x < 1 ||
        gl_GlobalInvocationID.y < 1 ||
        gl_GlobalInvocationID.x >= width-1 ||
        gl_GlobalInvocationID.y >= height-1) {
        return;
    }

    int my_index = int((gl_GlobalInvocationID.y) * width + gl_GlobalInvocationID.x);

    Cell cell = cells[my_index];
    Cell cell_left = cells[int((gl_GlobalInvocationID.y) * width + gl_GlobalInvocationID.x-1)];
    Cell cell_top = cells[int((gl_GlobalInvocationID.y+1) * width + gl_GlobalInvocationID.x)];
    Cell cell_right = cells[int((gl_GlobalInvocationID.y) * width + gl_GlobalInvocationID.x+1)];
    Cell cell_bottom = cells[int((gl_GlobalInvocationID.y-1) * width + gl_GlobalInvocationID.x)];

    float avg_surr_dens = (cell_left.density_next +
                           cell_top.density_next +
                           cell_right.density_next +
                           cell_bottom.density_next)/4.0;
    float den_top = cell.density + avg_surr_dens * k;

    vec2 avg_surr_vel = (cell_left.velocity_next +
                           cell_top.velocity_next +
                           cell_right.velocity_next +
                           cell_bottom.velocity_next)/4.0;
    vec2 vec_top = cell.velocity + avg_surr_vel * k;

    cells[my_index].density_next = den_top / (1.0+k);
    cells[my_index].velocity_next = vec_top / (1.0+k);

    //TODO(amatej): this is not the shader to store the color
    vec4 color = vec4(cell.density, 0, 0, 1);
    imageStore(destTex, ivec2(gl_GlobalInvocationID.xy), color);
}
