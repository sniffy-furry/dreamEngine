// render_cube: a hot-swappable render module. Spins a lit-less colored cube with GLES 3.0.
// Everything it exposes to the UI is a property (fov, wireframe, spin, distance).
#include "dream/core/module_api.h"
#include <GLES3/gl3.h>
#include <cmath>
#include <cstring>

namespace {

const DreamEngineHostAPI* g_host = nullptr;
uint32_t p_fov, p_wire, p_spin, p_dist;
uint32_t p_cam_yaw, p_cam_pitch, p_obj_x, p_obj_y, p_obj_z, p_obj_yaw, p_obj_pitch, p_obj_scale;
float g_angle = 0.f;
int g_w = 1, g_h = 1;
GLuint g_prog = 0, g_vao = 0, g_vbo = 0, g_ibo_tri = 0, g_ibo_line = 0, g_grid_vao = 0, g_grid_vbo = 0;
GLint g_loc_mvp = -1;

const char* kVS = R"(#version 300 es
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aCol;
uniform mat4 uMVP;
out vec3 vCol;
void main(){ vCol = aCol; gl_Position = uMVP * vec4(aPos, 1.0); })";
const char* kFS = R"(#version 300 es
precision mediump float;
in vec3 vCol; out vec4 o;
void main(){ o = vec4(vCol, 1.0); })";

// column-major 4x4 helpers
using M4 = float[16];
void ident(M4 m) { std::memset(m, 0, sizeof(M4)); m[0] = m[5] = m[10] = m[15] = 1.f; }
void mul(M4 out, const M4 a, const M4 b) {
    M4 r;
    for (int c = 0; c < 4; ++c)
        for (int rI = 0; rI < 4; ++rI) {
            float v = 0.f;
            for (int k = 0; k < 4; ++k) v += a[k * 4 + rI] * b[c * 4 + k];
            r[c * 4 + rI] = v;
        }
    std::memcpy(out, r, sizeof(M4));
}
void perspective(M4 m, float fov_deg, float aspect, float zn, float zf) {
    const float f = 1.f / std::tan(fov_deg * 0.5f * 3.14159265f / 180.f);
    std::memset(m, 0, sizeof(M4));
    m[0] = f / aspect; m[5] = f;
    m[10] = (zf + zn) / (zn - zf); m[11] = -1.f;
    m[14] = 2.f * zf * zn / (zn - zf);
}
void rot_y(M4 m, float a) { ident(m); m[0] = std::cos(a); m[8] = std::sin(a); m[2] = -std::sin(a); m[10] = std::cos(a); }
void rot_x(M4 m, float a) { ident(m); m[5] = std::cos(a); m[9] = -std::sin(a); m[6] = std::sin(a); m[10] = std::cos(a); }
void translate(M4 m, float x, float y, float z) { ident(m); m[12] = x; m[13] = y; m[14] = z; }

GLuint compile(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512]; glGetShaderInfoLog(s, sizeof log, nullptr, log);
        if (g_host && g_host->log) g_host->log(6, log);
    }
    return s;
}

// ---- engine-side module (properties + UI description) ----
int mod_init(DreamEngineModule*, const DreamEngineHostAPI* host) {
    g_host = host;
    if (!host->props || !host->ui) return 0;
    p_fov = host->props->register_float("render.fov", 70.f, 30.f, 120.f, "Render");
    p_wire = host->props->register_bool("render.wireframe", 0, "Render");
    p_spin = host->props->register_float("render.spin", 1.f, 0.f, 5.f, "Render");
    p_dist = host->props->register_float("render.distance", 4.f, 2.f, 12.f, "Render");
    p_cam_yaw = host->props->register_float("editor.camera.yaw", 0.f, -180.f, 180.f, "Editor Camera");
    p_cam_pitch = host->props->register_float("editor.camera.pitch", 0.f, -80.f, 80.f, "Editor Camera");
    p_obj_x = host->props->register_float("editor.object.x", 0.f, -20.f, 20.f, "Selected Object");
    p_obj_y = host->props->register_float("editor.object.y", 0.f, -20.f, 20.f, "Selected Object");
    p_obj_z = host->props->register_float("editor.object.z", 0.f, -20.f, 20.f, "Selected Object");
    p_obj_yaw = host->props->register_float("editor.object.yaw", 0.f, -360.f, 360.f, "Selected Object");
    p_obj_pitch = host->props->register_float("editor.object.pitch", 0.f, -360.f, 360.f, "Selected Object");
    p_obj_scale = host->props->register_float("editor.object.scale", 1.f, 0.25f, 3.f, "Selected Object");
    const uint32_t panel = host->ui->panel("Render");
    host->ui->query(panel, "render", nullptr, nullptr);   // every render.* property
    return 1;
}
void mod_update(DreamEngineModule*, double dt) {
    g_angle += static_cast<float>(dt) * g_host->props->get(p_spin);
}
void mod_shutdown(DreamEngineModule*) {}

DreamEngineModule g_module = {nullptr, mod_init, mod_update, mod_shutdown};
const DreamEngineModuleDescriptor g_desc = {DREAM_ENGINE_MODULE_ABI_VERSION, 1, "render_cube", "Render Cube"};

// ---- render side (GL context current) ----
void gl_init(DreamRenderModule*, const DreamEngineHostAPI*, int w, int h) {
    g_w = w; g_h = h;
    GLuint vs = compile(GL_VERTEX_SHADER, kVS), fs = compile(GL_FRAGMENT_SHADER, kFS);
    g_prog = glCreateProgram();
    glAttachShader(g_prog, vs); glAttachShader(g_prog, fs);
    glLinkProgram(g_prog);
    glDeleteShader(vs); glDeleteShader(fs);
    g_loc_mvp = glGetUniformLocation(g_prog, "uMVP");

    static const float v[] = {  // pos(3) color(3), 8 corners
        -1,-1,-1, 1,0,0,   1,-1,-1, 0,1,0,   1, 1,-1, 0,0,1,  -1, 1,-1, 1,1,0,
        -1,-1, 1, 1,0,1,   1,-1, 1, 0,1,1,   1, 1, 1, 1,1,1,  -1, 1, 1, 0.3f,0.3f,0.3f};
    static const unsigned short tri[] = {
        0,2,1, 0,3,2,  4,5,6, 4,6,7,  0,4,7, 0,7,3,  1,2,6, 1,6,5,  3,7,6, 3,6,2,  0,1,5, 0,5,4};
    static const unsigned short line[] = {0,1,1,2,2,3,3,0, 4,5,5,6,6,7,7,4, 0,4,1,5,2,6,3,7};
    glGenVertexArrays(1, &g_vao); glBindVertexArray(g_vao);
    glGenBuffers(1, &g_vbo); glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof v, v, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 24, (void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 24, (void*)12);
    glGenBuffers(1, &g_ibo_tri); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ibo_tri);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof tri, tri, GL_STATIC_DRAW);
    glGenBuffers(1, &g_ibo_line); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ibo_line);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof line, line, GL_STATIC_DRAW);
    glBindVertexArray(0);

    // Editor ground grid: 20x20 lines with a slightly brighter central axis.
    float grid[21 * 21 * 2 * 6];
    int gi = 0;
    for (int i = -10; i <= 10; ++i) {
        const float a = static_cast<float>(i);
        const float c = (i == 0) ? 0.65f : 0.22f;
        grid[gi++] = -10.f; grid[gi++] = 0.f; grid[gi++] = a; grid[gi++] = c; grid[gi++] = c; grid[gi++] = c;
        grid[gi++] =  10.f; grid[gi++] = 0.f; grid[gi++] = a; grid[gi++] = c; grid[gi++] = c; grid[gi++] = c;
        grid[gi++] = a; grid[gi++] = 0.f; grid[gi++] = -10.f; grid[gi++] = c; grid[gi++] = c; grid[gi++] = c;
        grid[gi++] = a; grid[gi++] = 0.f; grid[gi++] =  10.f; grid[gi++] = c; grid[gi++] = c; grid[gi++] = c;
    }
    glGenVertexArrays(1, &g_grid_vao); glBindVertexArray(g_grid_vao);
    glGenBuffers(1, &g_grid_vbo); glBindBuffer(GL_ARRAY_BUFFER, g_grid_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(grid), grid, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 24, (void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 24, (void*)12);
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}
void gl_resize(DreamRenderModule*, int w, int h) { g_w = w; g_h = h; }
void gl_draw(DreamRenderModule*, double) {
    glViewport(0, 0, g_w, g_h);
    glClearColor(0.05f, 0.06f, 0.09f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    const auto* P = g_host->props;
    M4 proj, view, ry, rx, model, vp, mvp;
    perspective(proj, P->get(p_fov), static_cast<float>(g_w) / static_cast<float>(g_h > 0 ? g_h : 1), 0.1f, 50.f);
    M4 camY, camX, tr;
    rot_y(camY, -P->get(p_cam_yaw) * 3.14159265f / 180.f);
    rot_x(camX, -P->get(p_cam_pitch) * 3.14159265f / 180.f);
    translate(tr, 0.f, 0.f, -P->get(p_dist));
    mul(view, camX, camY); mul(view, view, tr);
    rot_y(ry, (g_angle + P->get(p_obj_yaw) * 3.14159265f / 180.f));
    rot_x(rx, (g_angle * 0.6f + P->get(p_obj_pitch) * 3.14159265f / 180.f));
    mul(model, rx, ry);
    model[12] = P->get(p_obj_x); model[13] = P->get(p_obj_y); model[14] = P->get(p_obj_z);
    model[0] *= P->get(p_obj_scale); model[1] *= P->get(p_obj_scale); model[2] *= P->get(p_obj_scale);
    model[4] *= P->get(p_obj_scale); model[5] *= P->get(p_obj_scale); model[6] *= P->get(p_obj_scale);
    model[8] *= P->get(p_obj_scale); model[9] *= P->get(p_obj_scale); model[10] *= P->get(p_obj_scale); mul(vp, proj, view); mul(mvp, vp, model);
    glUseProgram(g_prog);
    // Draw the editor grid on the XZ plane.
    M4 gridModel, gridMvp; ident(gridModel); gridModel[13] = -1.35f;
    mul(gridMvp, vp, gridModel);
    glUniformMatrix4fv(g_loc_mvp, 1, GL_FALSE, gridMvp);
    glBindVertexArray(g_grid_vao);
    glDrawArrays(GL_LINES, 0, 84);

    // Draw the selected object preview.
    glUniformMatrix4fv(g_loc_mvp, 1, GL_FALSE, mvp);
    glBindVertexArray(g_vao);
    if (P->get(p_wire) != 0.f) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ibo_line);
        glDrawElements(GL_LINES, 24, GL_UNSIGNED_SHORT, nullptr);
    } else {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ibo_tri);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, nullptr);
    }
    glBindVertexArray(0);
}
void gl_shutdown(DreamRenderModule*) {
    if (g_prog) glDeleteProgram(g_prog);
    if (g_vbo) glDeleteBuffers(1, &g_vbo);
    if (g_ibo_tri) glDeleteBuffers(1, &g_ibo_tri);
    if (g_ibo_line) glDeleteBuffers(1, &g_ibo_line);
    if (g_vao) glDeleteVertexArrays(1, &g_vao);
    if (g_grid_vbo) glDeleteBuffers(1, &g_grid_vbo);
    if (g_grid_vao) glDeleteVertexArrays(1, &g_grid_vao);
    g_prog = g_vbo = g_ibo_tri = g_ibo_line = g_vao = g_grid_vbo = g_grid_vao = 0;
}
DreamRenderModule g_render = {nullptr, gl_init, gl_resize, gl_draw, gl_shutdown};

}  // namespace

extern "C" {
DREAM_MODULE_EXPORT const DreamEngineModuleDescriptor* dream_module_get_descriptor(void) { return &g_desc; }
DREAM_MODULE_EXPORT DreamEngineModule* dream_module_create(const DreamEngineHostAPI*) { return &g_module; }
DREAM_MODULE_EXPORT void dream_module_destroy(DreamEngineModule*) {}
DREAM_MODULE_EXPORT const DreamRenderModule* dream_module_get_render(void) { return &g_render; }
}
