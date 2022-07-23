#version 330 core

out vec4 FragColor;
in vec3 vertex_color;
in vec3 vertex_normal;
in vec3 vertex_view;

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
uniform mat4 um4v;
uniform mat4 um4n;
uniform Light light[3];
uniform Material material;
uniform int Verpixel;

void main() {
    vec3 N = normalize(vertex_normal);
    vec3 V = -vertex_view;
    vec3 color = vec3(0, 0, 0);
    
    if(lightIdx == 0)
    {
        vec4 View_light = um4v * vec4(light[0].position, 0);
        vec3 Lp = normalize(View_light.xyz + V);
        vec3 Half_vec = normalize(Lp + V);
        
        float NLp = max(dot(N, Lp), 0);
        float HN = pow(max(dot(Half_vec, N), 0), light[0].Shininess);
        
        color = (light[0].Ia * material.Ka + light[0].Id * material.Kd * NLp + light[0].Is * material.Ks * HN);
    }
    else if(lightIdx == 1)
    {
        vec4 View_light = um4v * vec4(light[1].position, 1.0);
        
        vec3 Lp = normalize(View_light.xyz + V);
        vec3 Half_vec = normalize(Lp + V);
        
        float NLp = max(dot(N, Lp), 0);
        float HN = pow(max(dot(Half_vec, N), 0), light[1].Shininess);
        
        float Distance = length(vertex_view.xyz - View_light.xyz);
        float f_att = min(1.0 / (light[1].constantAttenuation +
                                 light[1].linearAttenuation * Distance +
                                 light[1].quadraticAttenuation * Distance * Distance), 1.0);
        
        color = (light[1].Ia * material.Ka + f_att * (light[1].Id * material.Kd * NLp + light[1].Is * material.Ks * HN));
    }
    else if(lightIdx == 2)
    {
        vec4 View_light = um4v * vec4(light[2].position, 1.0);
        vec3 Lp = normalize(View_light.xyz + V);
        vec3 Half_vec = normalize(Lp + V);
        
        float NLp = max(dot(N, Lp), 0);
        float HN = pow(max(dot(Half_vec, N), 0), light[2].Shininess);
        
        float Distance = length(vertex_view.xyz - View_light.xyz);
        float f_att = min(1.0 / (light[2].constantAttenuation +
                                 light[2].linearAttenuation * Distance +
                                 light[2].quadraticAttenuation * Distance * Distance), 1.0);
        
        vec3 spotdirection = vec4(um4v * vec4(light[2].spotDirection, 0)).xyz;
        float vd = dot(normalize(vertex_view.xyz - View_light.xyz), normalize(spotdirection));
        float cutoff = cos((light[2].spotCutoff * 3.1415926 / 180));
        float spotEffect = (vd > cutoff) ? pow(max(vd, 0), light[2].spotExponent) : 0.0;
        
        color = (light[2].Ia * material.Ka + f_att * spotEffect * (light[2].Id * material.Kd * NLp + light[2].Is * material.Ks * HN));
    }
//    FragColor = vec4(color, 1.0f);
    vec4 v_color = vec4 (vertex_color, 1.0f);
    vec4 p_color = vec4 (color, 1.0f);
    if (Verpixel == 0) FragColor = v_color;
    if (Verpixel == 1) FragColor = p_color;
}


