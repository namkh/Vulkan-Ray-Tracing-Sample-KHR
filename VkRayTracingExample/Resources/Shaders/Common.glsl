const uint SURFACE_TYPE_DEFAULT = 0;
const uint SURFACE_TYPE_TRANSPARENT = 1;
const uint SURFACE_TYPE_TRANSPARENT_REFRACT = 2;

const float IOR_AIR = 1.0003;
const float IOR_GLASS = 1.52;
const float IOR_STEEL = 2.50;

const float PI = 3.14159265;

const vec3 METAL_COLOR = vec3(0.56f, 0.57f, 0.58f);

struct RayPayloadData
{
    vec3 hitColor;
    uint traceDepth;
    vec3 refectColor;
    float indexOfRefraction;
};

struct ShadowPayloadData
{
    bool isShadowed;
    uint randSeed;
};

struct Vertex
{
    vec4 position;
    vec4 normal;
    vec4 tangent;
    vec4 color;
    vec4 uv;
};

struct ObjectData
{
    mat4 worldMat;

		int geometryID;
		int materialID;
		float padding0;
		float padding1;
};

struct MaterialData
{
    vec4 color;

		uint materialTypeIndex;
		float indexOfRefraction;
		float metallic;
		float roughness;

		int diffuseTexIndex;
		int normalTexIndex;
		int roughnessTexIndex;
		int metallicTexIndex;

		int ambientOcclusionTexIndex;
		float uvScale;
		float padding0;
		float padding1;
};

uint tea(uint val0, uint val1)
{
  uint v0 = val0;
  uint v1 = val1;
  uint s0 = 0;

  for(uint n = 0; n < 16; n++)
  {
    s0 += 0x9e3779b9;
    v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + s0) ^ ((v1 >> 5) + 0xc8013ea4);
    v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + s0) ^ ((v0 >> 5) + 0x7e95761e);
  }

  return v0;
}

uint lcg(inout uint prev)
{
  uint LCG_A = 1664525u;
  uint LCG_C = 1013904223u;
  prev       = (LCG_A * prev + LCG_C);
  return prev & 0x00FFFFFF;
}

float rnd(inout uint prev)
{
  return (float(lcg(prev)) / float(0x01000000));
}

//from substance shader api
//https://docs.substance3d.com/spdoc/lib-pbr-shader-api-188976077.html
//////////////////////////////////////////////////////////////////////////////////////////////////////////
const float M_PI = 3.14159265;
const float M_2PI = 2.0 * M_PI;
const float M_INV_PI = 0.31830988;
const float M_GOLDEN_RATIO = 1.618034;

const float maxLod = 1.0f;
const int nbSamples = 16;
const float environment_rotation = 0.0f;
const float environment_exposure = 1.0f;
const float horizonFade = 1.3f;

struct LocalVectors 
{
  vec3 vertexNormal;
  vec3 tangent, bitangent, normal, eye;
};

float fibonacci1D(int i)
{
  return fract((float(i) + 1.0) * M_GOLDEN_RATIO);
}

vec2 fibonacci2D(int i, int nbSamples)
{
  return vec2(
    (float(i)+0.5) / float(nbSamples),
    fibonacci1D(i)
  );
}

vec3 envIrradiance(vec3 dir)
{
  float rot = environment_rotation * M_2PI;
  float crot = cos(rot);
  float srot = sin(rot);
  vec4 shDir = vec4(dir.xzy, 1.0);
  shDir = vec4(
    shDir.x * crot - shDir.y * srot,
    shDir.x * srot + shDir.y * crot,
    shDir.z,
    1.0);
  return max(vec3(0.0), vec3(
      dot(shDir, shDir),
      dot(shDir, shDir),
      dot(shDir, shDir)
    )) * environment_exposure;
}

float normal_distrib(
  float ndh,
  float Roughness)
{
  // use GGX / Trowbridge-Reitz, same as Disney and Unreal 4
  // cf http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf p3
  float alpha = Roughness * Roughness;
  float tmp = alpha / max(1e-8,(ndh*ndh*(alpha*alpha-1.0)+1.0));
  return tmp * tmp * M_INV_PI;
}

vec3 fresnel(
  float vdh,
  vec3 F0)
{
  // Schlick with Spherical Gaussian approximation
  // cf http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf p3
  float sphg = exp2((-5.55473*vdh - 6.98316) * vdh);
  return F0 + (vec3(1.0) - F0) * sphg;
}

float G1(
  float ndw, // w is either Ln or Vn
  float k)
{
  // One generic factor of the geometry function divided by ndw
  // NB : We should have k > 0
  return 1.0 / ( ndw*(1.0-k) +  k );
}

float visibility(
  float ndl,
  float ndv,
  float Roughness)
{
  // Schlick with Smith-like choice of k
  // cf http://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf p3
  // visibility is a Cook-Torrance geometry function divided by (n.l)*(n.v)
  float k = max(Roughness * Roughness * 0.5, 1e-5);
  return G1(ndl,k)*G1(ndv,k);
}

vec3 cook_torrance_contrib(
  float vdh,
  float ndh,
  float ndl,
  float ndv,
  vec3 Ks,
  float Roughness)
{
  // This is the contribution when using importance sampling with the GGX based
  // sample distribution. This means ct_contrib = ct_brdf / ggx_probability
  return fresnel(vdh,Ks) * (visibility(ndl,ndv,Roughness) * vdh * ndl / ndh );
}

vec3 importanceSampleGGX(vec2 Xi, vec3 T, vec3 B, vec3 N, float roughness)
{
  float a = roughness*roughness;
  float cosT = sqrt((1.0-Xi.y)/(1.0+(a*a-1.0)*Xi.y));
  float sinT = sqrt(1.0-cosT*cosT);
  float phi = 2.0*M_PI*Xi.x;
  return
    T * (sinT*cos(phi)) +
    B * (sinT*sin(phi)) +
    N *  cosT;
}

float probabilityGGX(float ndh, float vdh, float Roughness)
{
  return normal_distrib(ndh, Roughness) * ndh / (4.0*vdh);
}

float distortion(vec3 Wn)
{
  // Computes the inverse of the solid angle of the (differential) pixel in
  // the cube map pointed at by Wn
  float sinT = sqrt(1.0-Wn.y*Wn.y);
  return sinT;
}

float computeLOD(vec3 Ln, float p)
{
  return max(0.0, (maxLod-1.5) - 0.5 * log2(float(nbSamples) * p * distortion(Ln)));
}

float horizonFading(float ndl, float horizonFade)
{
  float horiz = clamp(1.0 + horizonFade * ndl, 0.0, 1.0);
  return horiz * horiz;
}

vec3 pbrComputeDiffuse(vec3 normal, vec3 diffColor)
{
  return envIrradiance(normal) * diffColor;
}

vec3 pbrComputeSpecular(LocalVectors vectors, vec3 specColor, float roughness)
{
  vec3 radiance = vec3(0.0);
  float ndv = dot(vectors.eye, vectors.normal);

  for(int i=0; i<nbSamples; ++i)
  {
    vec2 Xi = fibonacci2D(i, nbSamples);
    vec3 Hn = importanceSampleGGX(
      Xi, vectors.tangent, vectors.bitangent, vectors.normal, roughness);
    vec3 Ln = -reflect(vectors.eye,Hn);

    float fade = horizonFading(dot(vectors.vertexNormal, Ln), horizonFade);

    float ndl = dot(vectors.normal, Ln);
    ndl = max( 1e-8, ndl );
    float vdh = max(1e-8, dot(vectors.eye, Hn));
    float ndh = max(1e-8, dot(vectors.normal, Hn));
    float lodS = roughness < 0.01 ? 0.0 : computeLOD(Ln, probabilityGGX(ndh, vdh, roughness));
    radiance += fade * cook_torrance_contrib(vdh, ndh, ndl, ndv, specColor, roughness);
  }
  // Remove occlusions on shiny reflections
  radiance /= float(nbSamples);

  return radiance;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////