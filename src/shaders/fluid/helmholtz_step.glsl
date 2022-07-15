layout(local_size_x=20, local_size_y=20) in;
//Step using gauss seidel in helmholtz method to remove divergence

layout(binding=0) buffer pblock { Cell cells[]; };

layout (location = 3) uniform int width;
layout (location = 4) uniform int height;

void main() {
    int my_index = int((gl_GlobalInvocationID.y) * width + gl_GlobalInvocationID.x);

    Cell cell = cells[my_index];
    Cell cell_right = cells[int((gl_GlobalInvocationID.y) * width + gl_GlobalInvocationID.x+1)];
    Cell cell_left = cells[int((gl_GlobalInvocationID.y) * width + gl_GlobalInvocationID.x-1)];
    Cell cell_top = cells[int((gl_GlobalInvocationID.y+1) * width + gl_GlobalInvocationID.x)];
    Cell cell_bottom = cells[int((gl_GlobalInvocationID.y-1) * width + gl_GlobalInvocationID.x)];

    float top = (cell_right.p_value.x + cell_left.p_value.x + cell_top.p_value.x + cell_bottom.p_value.x) - cell.divergence.x;
    cells[my_index].p_value.x = top / 4.0;
}
