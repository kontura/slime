layout(local_size_x=20, local_size_y=20) in;
//Finish helmholtz method -> to remove divergence (uses output from gauss-seidel)

layout(binding=0) buffer pblock { Cell cells[]; };
layout (location = 1) uniform float speed;
layout (location = 3) uniform int width;
layout (location = 4) uniform int height;

void main() {

    int my_index = int((gl_GlobalInvocationID.y) * width + gl_GlobalInvocationID.x);

    Cell cell = cells[my_index];
    Cell cell_right = cells[int((gl_GlobalInvocationID.y) * width + gl_GlobalInvocationID.x+1)];
    Cell cell_left = cells[int((gl_GlobalInvocationID.y) * width + gl_GlobalInvocationID.x-1)];
    Cell cell_top = cells[int((gl_GlobalInvocationID.y+1) * width + gl_GlobalInvocationID.x)];
    Cell cell_bottom = cells[int((gl_GlobalInvocationID.y-1) * width + gl_GlobalInvocationID.x)];

    vec2 grad_p_value;
    grad_p_value.x = (cell_right.p_value.x - cell_left.p_value.x)/2;
    grad_p_value.y = (cell_top.p_value.x - cell_bottom.p_value.x)/2;

    cells[my_index].velocity = cell.velocity - grad_p_value;
    //if (speed > 1.0) {
    //    cells[my_index].velocity = cells[my_index].velocity * 1.1;
    //}
}
