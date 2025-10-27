$input v_color0, v_texcoord0, v_normal

#include <bgfx_shader.sh>
void main() {
     float diffuse_fac = max(0.0, dot(v_normal, normalize(vec3(0.25, 0.5, 0.75))));
     vec3 ambient = vec3(0.1, 0.1, 0.1);
     gl_FragColor = vec4(ambient + (v_color0.xyz * diffuse_fac), 1.0);
}