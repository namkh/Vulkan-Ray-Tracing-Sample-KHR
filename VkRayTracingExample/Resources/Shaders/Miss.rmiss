#version 460

#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "Common.glsl"

layout(binding = 7, set = 0) uniform samplerCube cubemapTexture;

layout(location=0) rayPayloadInEXT RayPayloadData payload;

void main()
{   


    //float m = 2.0f * sqrt(pow(dir.x, 2.0f) + pow(dir.y, 2.0f) + pow(dir.z + 1.0f, 2.0f));
    //vec2 uv = vec2(dir.x, -dir.y) / m + 0.5f;
    //payload.hitColor = texture(skyTexture, uv).xyz;

    vec3 dir = vec3(gl_WorldRayDirectionEXT.x, gl_WorldRayDirectionEXT.y, gl_WorldRayDirectionEXT.z);
    payload.hitColor =  texture(cubemapTexture, dir).xyz;
}