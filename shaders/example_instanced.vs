$input a_position, a_color0, a_normal, i_data0, i_data1, i_data2, i_data3
$output v_color0, v_texcoord0, v_normal

#include <bgfx_shader.sh>
void main() {
    mat4 model = mtxFromCols(i_data0, i_data1, i_data2, i_data3);

    mat4 projViewWorld = mul(mul(u_proj, u_view), model);
    vec4 v_pos = mul(projViewWorld, vec4(a_position, 1.0));
    gl_Position = v_pos;
    v_color0 = a_color0;
    v_normal = normalize(mat3(model) * a_normal);
}
