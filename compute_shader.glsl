#version 430
layout(local_size_x=256) in;

struct Agent {
    vec2 position;
    float angle;
    float type;
};

layout(location = 0) uniform float dt;
layout(binding=0) buffer pblock { Agent agents[]; };

layout (location = 6, rgba8) readonly uniform image2D srcTex;
layout (location = 1, rgba8) writeonly uniform image2D destTex;
layout (location = 2) uniform int width;
layout (location = 3) uniform int height;
layout (location = 4) uniform float moveSpeed;
layout (location = 5) uniform float turnSpeed;

float pserandom(vec2 uv) {
    float x = sin(dot(uv.xy, vec2(12.9898,78.233))) * 43758.5453123;
    return x - floor(x);
}

float sense(Agent agent, float sensorAngleOffset, float sensorOffsetDst, int sensorSize, int index) {
    float sensorAngle = agent.angle + sensorAngleOffset;
    vec2 sensorDir = vec2(cos(sensorAngle), sin(sensorAngle));
    vec2 sensorCenter = agent.position + sensorDir * sensorOffsetDst;
    float sum = 0;

    for (int offsetX = -sensorSize; offsetX <= sensorSize; offsetX++) {
        for (int offsetY = -sensorSize; offsetY <= sensorSize; offsetY++) {
            vec2 pos = sensorCenter + vec2(offsetX, offsetY);
            if (pos.x >= 0 && pos.x < width && pos.y >= 0 && pos.y < height) {
                //Use separate textures
                //if (agents[index].type >= 0.0f) {
                    vec3 c = imageLoad(srcTex, ivec2(pos)).xyz;
                    sum += c.x + c.y + c.z;
                //}
            }
        }
    }

    return sum;
}

void main() {
    //ivec2 storePos = ivec2(gl_GlobalInvocationID.xy);

    //These should come from CPU
    float sensorAngleSpacing = 1.0;
    float sensorOffsetDst = 5.0;
    int sensorSize = 2;


    int N = int(gl_NumWorkGroups.x*gl_WorkGroupSize.x);
    int index = int(gl_GlobalInvocationID);

    vec2 center = vec2(float(width)/1.618f, float(height)/1.618f);
    vec4 color = vec4(1, 0, 0, 1);
    Agent agent = agents[index];

    vec2 direction = vec2(cos(agent.angle), sin(agent.angle));
    vec2 newPos = agent.position + direction * moveSpeed * dt;

    float weightForward = sense(agent, 0, sensorOffsetDst, sensorSize, index);
    float weightLeft = sense(agent, sensorAngleSpacing, sensorOffsetDst, sensorSize, index);
    float weightRight = sense(agent, -sensorAngleSpacing, sensorOffsetDst, sensorSize, index);

    float randomSteerStrength = pserandom(agent.position);

    // Guide lights towards center
    vec2 vec_to_center = agent.position - center;
    vec2 vec_move_dir = agent.position - newPos;

    float top = vec_move_dir.x * vec_to_center.x + vec_move_dir.y * vec_to_center.y;
    float bot = length(vec_move_dir) * length(vec_to_center);
    float cos_of_angle = top/bot;

    //TODO(amatej): I think the problem is the fact the arccos is <0,pi> -> we loose info about the other inverval <pi, 2pi> -> we always got just in one direction
    //              we never know the other side is shorter
    float angle = acos(cos_of_angle);

    if (weightForward >= weightLeft && weightForward >= weightRight) {
        // Continue in the same dir
        //agents[index].angle += 0;
        if (angle < 1.54f) {
            agents[index].angle -= randomSteerStrength * turnSpeed * dt;
        } else {
            agents[index].angle += randomSteerStrength * turnSpeed * dt;
        }
    } else if (weightForward < weightLeft && weightForward < weightRight) {
        // Turn randomly
        //agents[index].angle += (randomSteerStrength - 0.5f) * 2.0f *  turnSpeed * dt;

        if (angle < 1.54f) {
            agents[index].angle -= randomSteerStrength * turnSpeed * dt;
        } else {
            agents[index].angle += randomSteerStrength * turnSpeed * dt;
        }

    } else if (weightRight > weightLeft) {
        // Turn Right
        agents[index].angle -= randomSteerStrength * turnSpeed * dt;
    } else if (weightLeft > weightRight) {
        // Turn Left
        agents[index].angle += randomSteerStrength * turnSpeed * dt;
    }

    if (newPos.x < 0.0f || newPos.x >= float(width) || newPos.y < 0.0f || newPos.y >= float(height))
    {
        newPos.x = min(width-0.01, max(0, newPos.x));
        newPos.y = min(height-0.01, max(0, newPos.y));
        agents[index].angle = pserandom(agent.position) * 2.0f * 3.14f;

    }
    //ivec2 diff = ivec2(newPos - agents[index].position);
    //ivec2 inc = ivec2(0,0);

    //if (diff.x > 0)
    //    inc.x = 1;
    //if (diff.y > 0)
    //    inc.x = 1;
    //if (diff.x < 0)
    //    inc.x = -1;
    //if (diff.y < 0)
    //    inc.x = -1;

    //int failsafe = 0;

    //ivec2 i = ivec2(agents[index].position);
    //while(newPos.x > i.x || newPos.y > i.y ) {
    //    imageStore(destTex, ivec2(i), color);
    //    i = i + inc;

    //    if (failsafe > 1000){
    //        break;
    //    }
    //    failsafe++;
    //}

    agents[index].position = newPos;
    imageStore(destTex, ivec2(newPos) + ivec2(0,0), color);
    imageStore(destTex, ivec2(newPos) + ivec2(1,1), color);
    imageStore(destTex, ivec2(newPos) + ivec2(-1,-1), color);
    imageStore(destTex, ivec2(newPos) + ivec2(-1,1), color);
    imageStore(destTex, ivec2(newPos) + ivec2(1,-1), color);
    imageStore(destTex, ivec2(newPos) + ivec2(1,0), color);
    imageStore(destTex, ivec2(newPos) + ivec2(0,1), color);
    imageStore(destTex, ivec2(newPos) + ivec2(0,-1), color);
    imageStore(destTex, ivec2(newPos) + ivec2(-1,0), color);
    imageStore(destTex, ivec2(newPos) + ivec2(2,0), color);
    imageStore(destTex, ivec2(newPos) + ivec2(0,2), color);
    imageStore(destTex, ivec2(newPos) + ivec2(-2,0), color);
    imageStore(destTex, ivec2(newPos) + ivec2(0,-2), color);
}