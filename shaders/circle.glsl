out vec4 FragColor;

uniform vec2 u_resolution;
uniform vec3 u_pos;
uniform vec4 u_color;

void main() 
{
	vec2 uv = gl_FragCoord.xy / u_resolution.y;
    vec3 pos = 2.0 * u_pos / u_resolution.y;
    //vec3 pos = vec3(2.0 * u_pos.xy / u_resolution.y, u_pos.z);
    
    float m = smoothstep(pos.z, length(pos.xy - uv), 0.01);
    //float n = 0.2 * smoothstep(pos.z * pos.z, length(pos.xy - uv), 0.2);

    FragColor = vec4(u_color.xyz, m);
}
