layout(local_size_x=20, local_size_y=20) in;

layout(binding=0) buffer pblock { Cell cells[]; };

layout (location = 3) uniform int width;

void main() {
    int my_index = int((gl_GlobalInvocationID.y) * width + gl_GlobalInvocationID.x);
    cells[my_index].density = cells[my_index].density_next;
}
