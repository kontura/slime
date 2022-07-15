layout(local_size_x=20, local_size_y=20) in;
//TODO(amatej): this is gonna execute a bijilion time -> we don't want that here
//is that fixed for spectrum?

layout(binding=0) buffer pblock { Cell cells[]; };

layout (location = 0) uniform float dt;
layout (location = 1) uniform float speed;
layout (location = 3) uniform int width;
layout (location = 4) uniform int height;

float k = 2.11;

void main() {
    int my_index = int((gl_GlobalInvocationID.y) * width + gl_GlobalInvocationID.x);

    Cell cell = cells[my_index];
    vec2 pos = vec2(gl_GlobalInvocationID.xy);
    vec2 f_origin = pos - cell.velocity*dt;
    vec2 i = floor(f_origin);
    vec2 j = fract(f_origin);

    Cell cell_1 = cells[int(i.y) * width + int(i.x)];
    Cell cell_2 = cells[int(i.y) * width + int(i.x+1)];
    Cell cell_3 = cells[int(i.y+1) * width + int(i.x)];
    Cell cell_4 = cells[int(i.y+1) * width + int(i.x+1)];

    float density_z1 = mix(cell_1.density, cell_2.density, j.x);
    float density_z2 = mix(cell_3.density, cell_4.density, j.x);

    vec2 velocity_z1 = mix(cell_1.velocity, cell_2.velocity, j.x);
    vec2 velocity_z2 = mix(cell_3.velocity, cell_4.velocity, j.x);

    cells[my_index].density = mix(density_z1, density_z2, j.y);
    cells[my_index].velocity = mix(velocity_z1, velocity_z2, j.y);

//    if (speed > 1.0) {
//        cells[my_index].velocity = cells[my_index].velocity * 1.1;
//    }
    //TODO(amatej): is this right?
    vec2 new_target_pos = pos + cells[my_index].velocity;
    if (new_target_pos.x < 0 || new_target_pos.x >= width) {
        cells[my_index].velocity.x = -cells[my_index].velocity.x;
    }
    if (new_target_pos.y < 0 || new_target_pos.y >= height) {
        cells[my_index].velocity.y = -cells[my_index].velocity.y;
    }

}
