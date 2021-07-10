#version 430

layout(local_size_x=256) in;

layout(location = 0) uniform float dt;
layout(std430, binding=0) buffer pblock { vec4 positions[]; };
layout(std430, binding=1) buffer vblock { vec4 velocities[]; };

layout (location = 1, rgba8) uniform image2D destTex;

void main() {
   int index = int(gl_GlobalInvocationID);
   vec4 position = positions[index];
   vec4 velocity = velocities[index];
   position.xy += dt*velocity.xy;
   if (position.x > 0.5) {
       position.x = 0.5;
       velocities[index] = -velocities[index];
   }
   if (position.x < -0.5) {
       position.x = -0.5;
   }
   if (position.y > 0.5) {
       position.y = 0.5;
       velocities[index] = -velocities[index];
   }
   if (position.y < -0.5) {
       position.y = -0.5;
   }
   positions[index] = position;



}
