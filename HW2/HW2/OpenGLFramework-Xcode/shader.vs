#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNormal;

//hw2
out vec3 vertex_color;
out vec3 vertex_normal;
out vec3 vertex_view;
uniform mat4 mvp;

uniform mat4 um4v;
uniform mat4 um4n;
//hw2
struct Light{
    vec3 position;
    vec3 spotDirection;
    vec3 Ia;
    vec3 Id;
    vec3 Is;
    float Shininess;
    float spotExponent;
    float spotCutoff;
    float constantAttenuation;
    float linearAttenuation;
    float quadraticAttenuation;
};

struct Material
{
    vec3 Ka;
    vec3 Kd;
    vec3 Ks;
    
};
uniform int lightIdx;
uniform Light light[3];
uniform Material material;
uniform int Verpixel;

void main()
{
    vec4 View_vertex = um4n * vec4(aPos.x, aPos.y, aPos.z, 1.0);
    vec4 n_plum = transpose( inverse(um4n)) * vec4(aNormal.x, aNormal.y, aNormal.z, 0.0);
    
    vertex_view = View_vertex.xyz;
    vertex_normal = n_plum.xyz;
    
    vec3 N = normalize(n_plum.xyz);
    vec3 V = -View_vertex.xyz;
    
    if(lightIdx == 0)
    {
        vec4 View_light = um4v * vec4(light[0].position, 0);
        vec3 Lp = normalize(View_light.xyz + V);
        vec3 Half_vec = normalize(Lp + V);
        
        float NLp = max(dot(N, Lp), 0);
        float HN = pow(max(dot(Half_vec, N), 0), light[0].Shininess);
        
        vertex_color = (light[0].Ia * material.Ka + light[0].Id * material.Kd * NLp + light[0].Is * material.Ks * HN);
    }
    else if(lightIdx == 1)
    {
        vec4 View_light = um4v * vec4(light[1].position, 1.0);
        
        vec3 Lp = normalize(View_light.xyz + V);
        vec3 Half_vec = normalize(Lp + V);
        
        float NLp = dot(N, Lp);
        float HN = pow(max(dot(Half_vec, N), 0), light[1].Shininess);
        
        float Distance = length(View_vertex.xyz - View_light.xyz);
        float f_att = min(1.0 / (light[1].constantAttenuation +
                                 light[1].linearAttenuation * Distance +
                                 light[1].quadraticAttenuation * Distance * Distance), 1.0);
        
        vertex_color = (light[1].Ia * material.Ka + f_att * (light[1].Id * material.Kd * NLp + light[1].Is * material.Ks * HN));
    }
    else if(lightIdx == 2)
    {
        vec4 View_light = um4v * vec4(light[2].position, 1.0);
        
        vec3 Lp = normalize(View_light.xyz + V);
        vec3 Half_vec = normalize(Lp + V);
        
        float NLp = max(dot(N, Lp), 0);
        float HN = pow(max(dot(Half_vec, N), 0), light[2].Shininess);
        
        float Distance = length(View_vertex.xyz - View_light.xyz);
        float f_att = min(1.0 / (light[2].constantAttenuation +
                                 light[2].linearAttenuation * Distance +
                                 light[2].quadraticAttenuation * Distance * Distance), 1);
        //??? p.49
        vec3 v = normalize(View_vertex.xyz - View_light.xyz);
        vec3 d = normalize(vec4(um4v * vec4(light[2].spotDirection, 0)).xyz);
        float vd = dot(v, d);
        float cutoff = cos((light[2].spotCutoff * 3.1415926 / 180));
        float spotEffect = (vd > cutoff) ? pow(max(vd, 0), light[2].spotExponent) : 0.0;
        vertex_color = (light[2].Ia * material.Ka + f_att * spotEffect * (light[2].Id * material.Kd * NLp + light[2].Is * material.Ks * HN));
//        vertex_color = light[2].Ia * material.Ka + 1 * 1 * (light[2].Id * material.Kd * NLp + light[2].Is * material.Ks * HN);
    }
    
    gl_Position = mvp * vec4(aPos.x, aPos.y, aPos.z, 1.0);

}
