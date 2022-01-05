layout (location = 0) in vec2 vertCoord;

uniform vec2 u_resolution;

void main()
{
    vec2 pos = vertCoord;// * 2.0 / u_resolution.y;
    //pos.x *= (u_resolution.y / u_resolution.x);
    gl_Position = vec4(pos, 0.0, 1.0);
}
