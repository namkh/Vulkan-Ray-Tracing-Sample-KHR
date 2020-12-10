#version 460

#extension GL_EXT_ray_tracing : enable
#extension GL_GOOGLE_include_directive : enable

#include "Common.glsl"

layout(location=1) rayPayloadInEXT ShadowPayloadData payload;

void main()
{   
    payload.isShadowed = false;
}