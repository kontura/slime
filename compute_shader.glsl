#version 430
layout(local_size_x=8, local_size_y=8) in;

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

layout (location = 7) uniform float moveSpeedType2;
layout (location = 8) uniform float turnSpeedType2;

layout (location = 9) uniform float sensorAngleSpacing;
layout (location = 10) uniform float sensorOffsetDst;
layout (location = 11) uniform int sensorSize;
layout (location = 12) uniform int agentsCount;

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

    int N = int(gl_NumWorkGroups.x*gl_WorkGroupSize.x);
    //int index = int(gl_GlobalInvocationID);
    int index = int(gl_GlobalInvocationID.x);

    vec2 center = vec2(float(width)/1.618f, float(height)/1.618f);
    vec4 color = vec4(0, 0, 1, 1);
    Agent agent = agents[index];

    float agent_move_speed = moveSpeed;
    float agent_turn_speed = turnSpeed;
    //if (agent.type > 1) {
    //    agent_move_speed = moveSpeedType2;
    //    agent_turn_speed = turnSpeedType2;
    //    color = vec4(0,0,1,1);
    //}

    vec2 direction = vec2(cos(agent.angle), sin(agent.angle));
    vec2 newPos = agent.position + direction * agent_move_speed * dt;

    float weightForward = sense(agent, 0, sensorOffsetDst, sensorSize, index);
    float weightLeft = sense(agent, sensorAngleSpacing, sensorOffsetDst, sensorSize, index);
    float weightRight = sense(agent, -sensorAngleSpacing, sensorOffsetDst, sensorSize, index);

    float randomSteerStrength = pserandom(agent.position * index);

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
            agents[index].angle -= randomSteerStrength * agent_turn_speed * dt;
        } else {
            agents[index].angle += randomSteerStrength * agent_turn_speed * dt;
        }
    } else if (weightForward < weightLeft && weightForward < weightRight) {
        // Turn randomly
        //agents[index].angle += (randomSteerStrength - 0.5f) * 2.0f *  agent_turn_speed * dt;

        if (angle < 1.54f) {
            agents[index].angle -= randomSteerStrength * agent_turn_speed * dt;
        } else {
            agents[index].angle += randomSteerStrength * agent_turn_speed * dt;
        }

    } else if (weightRight > weightLeft) {
        // Turn Right
        agents[index].angle -= randomSteerStrength * agent_turn_speed * dt;
    } else if (weightLeft > weightRight) {
        // Turn Left
        agents[index].angle += randomSteerStrength * agent_turn_speed * dt;
    }

    if (newPos.x < 0.0f || newPos.x > float(width) || newPos.y < 0.0f || newPos.y > float(height))
    {
        newPos.x = min(width-0.01, max(0, newPos.x));
        newPos.y = min(height-0.01, max(0, newPos.y));
        agents[index].angle = pserandom(agent.position) * 2.0f * 3.14f;

    }

    agents[index].position = newPos;
    imageStore(destTex, ivec2(newPos), color);
}
