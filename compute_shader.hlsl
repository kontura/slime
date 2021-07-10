#version 430
layout(local_size_x=256) in;

layout(location = 0) uniform float dt;
layout(binding=0) buffer pblock { vec4 positions[]; };
layout(binding=1) buffer vblock { vec4 velocities[]; };

layout (location = 1, rgba8) uniform image2D destTex;

void main() {
     //ivec2 storePos = ivec2(gl_GlobalInvocationID.xy);
     //float localCoef = length(vec2(ivec2(gl_LocalInvocationID.xy)-8)/8.0);
     //float globalCoef = sin(float(gl_WorkGroupID.x+gl_WorkGroupID.y)*0.1 + roll)*0.5;

   int N = int(gl_NumWorkGroups.x*gl_WorkGroupSize.x);
   int index = int(gl_GlobalInvocationID);

   vec3 position = positions[index].xyz;
   vec3 velocity = velocities[index].xyz;
   vec3 acceleration = vec3(0,0,0);
   for(int i = 0;i<N;++i) {
       vec3 other = positions[i].xyz;
       vec3 diff = position - other;
       float invdist = 1.0/(length(diff)+0.001);
       acceleration -= diff*0.00001*invdist*invdist*invdist;
   }
   velocities[index] = vec4(velocity+dt*acceleration,0);
   imageStore(destTex, ivec2(int(position.x*1920*2), int(position.y*1080*2)), vec4(0.0, 1.0, 0.0, 1.0));
}
