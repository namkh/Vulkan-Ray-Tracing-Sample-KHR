#version 460

#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

#include "Common.glsl"

layout (binding=0, set=0) uniform accelerationStructureEXT topLevelAs;

layout (binding=2, set=0) uniform GlobalConstantBuffer
{
    mat4 matViewInv;
    mat4 matProjInv;
    vec3 lightDir;
    float padding0;
} globalConstants;

layout(binding = 3, set = 0) buffer ObjConstantBuffer { ObjectData data[]; } objConstants;
layout(binding = 4, set = 0) buffer MaterialConstantBuffer { MaterialData data[]; } materialConstants;
layout(binding = 5, set = 0, scalar) buffer VertexBuffer { Vertex data[]; } vertexBuffer[];
layout(binding = 6, set = 0) buffer IndexBuffer { uint data[]; } indexBuffer[];
layout(binding = 8, set = 0) uniform sampler2D samplers[];

layout(location=0) rayPayloadInEXT RayPayloadData payload;
layout(location=1) rayPayloadEXT ShadowPayloadData shadowedPayload;

hitAttributeEXT vec2 attribs;

const float min = 0.1f;
const float max = 10000.0f;

const uint vertexSizeOfFloat = 15;

vec3 NormalSampleToWorldSpace(vec3 normalMapSample, vec3 unitNormalW, vec3 tangentW)
{
	vec3 normalT = 2.0f*normalMapSample - 1.0f;

	vec3 N = unitNormalW;
	vec3 T = normalize(tangentW - dot(tangentW, N)*N);
	vec3 B = cross(N, T);

	mat3 TBN = mat3(T, B, N);

	vec3 bumpedNormalW = TBN * normalT;

	return bumpedNormalW;
}

vec3 SchlickFesnel(vec3 F0, float ldh)
{
    float t = pow(1.0f - ldh, 5);
    return F0 + (1.0f - F0) * t;
}

float GGXNormalDistribution(float a2, float ndh)
{
    float d =  (ndh * a2 - ndh) * ndh + 1.0f;
    return a2 / (PI * d * d);
}

float SchlickMaskingTerm(float a2, float ndl, float ndv)
{
	float k = a2 / 2;

	float g_v = ndv / (ndv * (1 - k) + k);
	float g_l = ndl / (ndl * (1 - k) + k);
	return g_v * g_l;
}

void main()
{ 
    ObjectData objData = objConstants.data[gl_InstanceID];
    uint geometryID = objData.geometryID;
    uint materialID = objData.materialID;
    MaterialData materialData = materialConstants.data[materialID];

    mat4 worldMat = objData.worldMat;
    const ivec3 indices = ivec3(indexBuffer[nonuniformEXT(geometryID)].data[3 * gl_PrimitiveID + 0], 
                                indexBuffer[nonuniformEXT(geometryID)].data[3 * gl_PrimitiveID + 1], 
                                indexBuffer[nonuniformEXT(geometryID)].data[3 * gl_PrimitiveID + 2]);
    
    Vertex vert0 = vertexBuffer[nonuniformEXT(geometryID)].data[indices.x];
    Vertex vert1 = vertexBuffer[nonuniformEXT(geometryID)].data[indices.y];
    Vertex vert2 = vertexBuffer[nonuniformEXT(geometryID)].data[indices.z];

    const Vertex testVert = vertexBuffer[nonuniformEXT(geometryID)].data[3];

    const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);

    vec3 worldPos = gl_WorldRayOriginEXT + gl_WorldRayDirectionEXT * gl_HitTEXT;
    vec3 vertexNormalW = normalize(mat3(worldMat) * normalize(vert0.normal.xyz * barycentricCoords.x + vert1.normal.xyz * barycentricCoords.y + vert2.normal.xyz * barycentricCoords.z));
    vec3 vertexTangentW = normalize(mat3(worldMat) * normalize(vert0.tangent.xyz * barycentricCoords.x + vert1.tangent.xyz * barycentricCoords.y + vert2.tangent.xyz * barycentricCoords.z)).xyz;
    vec2 uv = (vert0.uv.xy * barycentricCoords.x + vert1.uv.xy * barycentricCoords.y + vert2.uv.xy * barycentricCoords.z) * materialData.uvScale;

    vec3 normalSample = texture(samplers[materialData.normalTexIndex], uv).xyz;
    vec4 diffuse = texture(samplers[materialData.diffuseTexIndex], uv);
    float roughness = texture(samplers[materialData.roughnessTexIndex], uv).x;
    float metallic = texture(samplers[materialData.metallicTexIndex], uv).x;

    vec3 normal = normalize(NormalSampleToWorldSpace(normalSample, vertexNormalW, vertexTangentW));
    vec3 negWorldRayDirection = normalize(-gl_WorldRayDirectionEXT);
    vec3 negLightDir = normalize(-globalConstants.lightDir);

    float ndv = clamp(dot(normal, negWorldRayDirection), 0.0001f, 1.0f);;
    float ndl = clamp(dot(normal, negLightDir), 0.0001f, 1.0f);
    
    vec3 resColor = vec3(0.0f);
    vec3 diffuseColor = vec3(0.0f);
    vec3 specularColor = vec3(0.0f);
    vec3 F = vec3(0.0f);
    float D = 0.0f;
    float G = 0.0f;

    //reflection
    payload.hitColor = vec3(1.0f);
    vec3 reflectColor = vec3(0.0f);
        
    if(payload.traceDepth < 3)
    {
        vec3 rayDir = reflect(gl_WorldRayDirectionEXT, normal);
        payload.traceDepth++;
        traceRayEXT(topLevelAs, gl_RayFlagsCullBackFacingTrianglesEXT, 0xFF, 0, 0, 0, worldPos.xyz, min, rayDir, max, 0);
        payload.traceDepth--;
        reflectColor = payload.hitColor;
    }    
      
    if(ndv * ndl != 0.0f)
    {
        float reflectiveIndex = pow((IOR_AIR - materialData.indexOfRefraction) / (IOR_AIR + materialData.indexOfRefraction), 2.0f);
        vec3 halfVec = normalize(negWorldRayDirection + negLightDir);
        
        float a2 = pow(roughness, 2);
            
        float ndh = clamp(dot(normal, halfVec), 0.0001f, 1.0f);
        float ldh = dot(negLightDir, halfVec);

        F = SchlickFesnel(vec3(reflectiveIndex), ldh);
        D = GGXNormalDistribution(a2, ndh);
        G = SchlickMaskingTerm(a2, ndl, ndv);
            
        specularColor = (D * G * F) / (4.0f * ldh);
    }

    const float ks = mix(0.0f, 1.0f - F.x, metallic);
    const float kd = 1.0 - ks;

    specularColor = specularColor * diffuse.xyz * ks;
    
    diffuseColor = ndl * materialData.color.xyz * diffuse.xyz * kd;
    if(materialData.ambientOcclusionTexIndex > 0)
    {
        vec3 ao = texture(samplers[materialData.ambientOcclusionTexIndex], uv).xyz;
        diffuseColor = diffuseColor * ao;
    }
    
    reflectColor = ks * diffuse.xyz * reflectColor;
    resColor = diffuseColor + specularColor + reflectColor;
        
    //shadow
    shadowedPayload.isShadowed = true;
    uint flags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsSkipClosestHitShaderEXT | gl_RayFlagsCullBackFacingTrianglesEXT;

    traceRayEXT(topLevelAs, flags, 0xFF, 1, 0, 1, worldPos.xyz, min, -globalConstants.lightDir, max, 1);
    if(shadowedPayload.isShadowed)
    {
        resColor = resColor * 0.7f;
    }

    if(payload.traceDepth < 3)
    {
        payload.traceDepth++;
        traceRayEXT(topLevelAs, gl_RayFlagsCullBackFacingTrianglesEXT, 0xFF, 0, 0, 0, worldPos.xyz, min, gl_WorldRayDirectionEXT, max, 0);
        payload.traceDepth--;
    }
    vec3 transparentColor = payload.hitColor;

    payload.hitColor = mix(transparentColor, resColor, materialData.color.w * diffuse.w);
}