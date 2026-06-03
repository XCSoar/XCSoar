// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ForwardView/ForwardViewTerrainShader.hpp"

#ifdef ENABLE_OPENGL

#include "ForwardView/ForwardViewGeometry.hpp"
#include "Geo/FAISphere.hpp"
#include "ui/canvas/opengl/Attribute.hpp"
#include "ui/canvas/opengl/Program.hpp"
#include "ui/canvas/opengl/Shaders.hpp"
#include "lib/fmt/RuntimeError.hxx"

#include <cstddef>
#include <glm/gtc/type_ptr.hpp>
#include <memory>

namespace {

static GLProgram *program = nullptr;
static GLint loc_projection = -1;
static GLint loc_modelview = -1;
static GLint loc_scroll = -1;
static GLint loc_ref_alt = -1;
static GLint loc_vertical_ref = -1;
static GLint loc_eye_z = -1;
static GLint loc_sun_dir = -1;
static GLint loc_sun_active = -1;
static GLint loc_earth_radius = -1;
static GLint loc_vert_exag = -1;
static GLint loc_topo_active = -1;
static GLint loc_topo_tex = -1;
static GLint loc_terrain_ramp_weight = -1;

static constexpr GLuint ATTR_MESH = 4;
static constexpr GLuint ATTR_MATERIAL = 5;
static constexpr GLuint ATTR_NORMAL = 6;
static constexpr GLuint ATTR_SHADOW = 7;
static constexpr GLuint ATTR_TOPO_UV = 8;
static constexpr GLuint ATTR_BASE_RGB = 9;

static std::unique_ptr<GLArrayBuffer> vbo;
static unsigned triangle_count = 0;

#define GLSL_VERSION "#version 100\n"
#define GLSL_PRECISION "precision mediump float;\n"

static constexpr char vertex_shader[] =
  GLSL_VERSION
  "precision highp float;\n"
  R"glsl(
    uniform mat4 projection;
    uniform mat4 modelview;
    uniform vec2 scroll;
    uniform highp float ref_alt;
    uniform highp float vertical_ref;
    uniform highp float earth_radius;
    uniform highp float vert_exag;

    attribute vec3 mesh_pos;
    attribute float mesh_material;
    attribute vec3 mesh_normal;
    attribute float mesh_shadow;
    attribute vec2 mesh_topo_uv;
    attribute vec3 mesh_base_rgb;

    varying highp vec3 v_normal;
    varying highp float v_shadow;
    varying highp vec3 v_world;
    varying highp float v_msl;
    varying highp float v_material;
    varying mediump vec2 v_topo_uv;
    varying mediump vec3 v_base_rgb;

    void main() {
      float display_x = mesh_pos.x - scroll.x;
      float display_y = -(mesh_pos.y - scroll.y);
      float msl = mesh_pos.z;
      float z = (msl - ref_alt) +
        (msl - vertical_ref) * vert_exag -
        (display_x * display_x + display_y * display_y) /
        (2. * earth_radius);

      v_world = vec3(display_x, display_y, z);
      v_msl = msl;
      v_material = mesh_material;
      v_normal = mesh_normal;
      v_shadow = mesh_shadow;
      v_topo_uv = mesh_topo_uv;
      v_base_rgb = mesh_base_rgb;
      gl_Position = projection * modelview * vec4(v_world, 1.);
    }
)glsl";

static constexpr char fragment_shader[] =
  GLSL_VERSION
  GLSL_PRECISION
  R"glsl(
    uniform vec3 sun_dir;
    uniform float sun_active;
    uniform float topo_active;
    uniform float terrain_ramp_weight;
    uniform sampler2D topo_tex;

    varying highp vec3 v_normal;
    varying highp float v_shadow;
    varying highp float v_msl;
    varying highp float v_material;
    varying mediump vec2 v_topo_uv;
    varying mediump vec3 v_base_rgb;

    const float SHADING_CONTRAST = 165.;
    const float SHADING_AMBIENT = 0.18;

    vec3 land = vec3(0.420, 0.451, 0.333);
    vec3 water = vec3(0.333, 0.627, 1.0);

    float mix_byte(float a, float b, float i) {
      return (a * i + b * (128. - i)) / 128.;
    }

    vec3 apply_shading(vec3 color, float illum) {
      if (illum >= -0.5 && illum <= 0.5)
        return color;

      if (illum < 0.) {
        float i = min(96., -illum);
        return vec3(
          mix_byte(0., color.r, i),
          mix_byte(0., color.g, i),
          mix_byte(96. / 255., color.b, i));
      }

      float i = min(64., illum / 2.);
      return vec3(
        mix_byte(1., color.r, i),
        mix_byte(1., color.g, i),
        mix_byte(16. / 255., color.b, i));
    }

    void main() {
      if (v_material > 1.5) {
        discard;
      }

      vec3 ramp = v_base_rgb;
      vec3 color = v_material > 0.5 ? water : ramp;
      if (topo_active > 0.5 && v_material < 0.5)
        color = mix(land, ramp, terrain_ramp_weight);

      float illum = 0.;
      if (sun_active > 0.5) {
        float dot_s = dot(normalize(v_normal), sun_dir);
        dot_s = clamp(dot_s, -1., 1.);
        float shade = SHADING_AMBIENT + (1. - SHADING_AMBIENT) * dot_s;
        illum = (shade - 0.5) * SHADING_CONTRAST;
        if (v_shadow < -0.5)
          illum = min(illum, v_shadow);
      }

      color = apply_shading(color, illum);

      if (topo_active > 0.5) {
        vec4 topo = texture2D(topo_tex, v_topo_uv);
        if (topo.a > 0.02)
          color = mix(color, topo.rgb, topo.a);
      }

      gl_FragColor = vec4(color, 1.);
    }
)glsl";

static void
CompileAttachShader(GLProgram &prog, GLenum type, const char *code)
{
  GLShader shader(type);
  shader.Source(code);
  shader.Compile();

  if (shader.GetCompileStatus() != GL_TRUE) {
    char log[1000];
    shader.GetInfoLog(log, sizeof(log));
    throw FmtRuntimeError("Forward view terrain shader failed: {}", log);
  }

  prog.AttachShader(shader);
}

static void
LinkProgram(GLProgram &prog)
{
  prog.Link();

  if (prog.GetLinkStatus() != GL_TRUE) {
    char log[1000];
    prog.GetInfoLog(log, sizeof(log));
    throw FmtRuntimeError("Forward view terrain shader link failed: {}", log);
  }
}

static void
DisableSolidShaderAttributes() noexcept
{
  glDisableVertexAttribArray(OpenGL::Attribute::POSITION);
  glDisableVertexAttribArray(OpenGL::Attribute::COLOR);
}

} // namespace

bool
ForwardViewTerrainShader::EnsureInitialised() noexcept
{
  if (program != nullptr)
    return true;

  try {
    program = new GLProgram();
    program->BindAttribLocation(ATTR_MESH, "mesh_pos");
    program->BindAttribLocation(ATTR_MATERIAL, "mesh_material");
    program->BindAttribLocation(ATTR_NORMAL, "mesh_normal");
    program->BindAttribLocation(ATTR_SHADOW, "mesh_shadow");
    program->BindAttribLocation(ATTR_TOPO_UV, "mesh_topo_uv");
    program->BindAttribLocation(ATTR_BASE_RGB, "mesh_base_rgb");
    CompileAttachShader(*program, GL_VERTEX_SHADER, vertex_shader);
    CompileAttachShader(*program, GL_FRAGMENT_SHADER, fragment_shader);
    LinkProgram(*program);

    loc_projection = program->GetUniformLocation("projection");
    loc_modelview = program->GetUniformLocation("modelview");
    loc_scroll = program->GetUniformLocation("scroll");
    loc_ref_alt = program->GetUniformLocation("ref_alt");
    loc_vertical_ref = program->GetUniformLocation("vertical_ref");
    loc_eye_z = program->GetUniformLocation("eye_z");
    loc_sun_dir = program->GetUniformLocation("sun_dir");
    loc_sun_active = program->GetUniformLocation("sun_active");
    loc_earth_radius = program->GetUniformLocation("earth_radius");
    loc_vert_exag = program->GetUniformLocation("vert_exag");
    loc_topo_active = program->GetUniformLocation("topo_active");
    loc_topo_tex = program->GetUniformLocation("topo_tex");
    loc_terrain_ramp_weight =
      program->GetUniformLocation("terrain_ramp_weight");
    return true;
  } catch (...) {
    delete program;
    program = nullptr;
    return false;
  }
}

void
ForwardViewTerrainShader::UploadMesh(const MeshVertex *vertices,
                                     unsigned n_triangles) noexcept
{
  triangle_count = 0;
  if (n_triangles == 0 || vertices == nullptr)
    return;

  if (vbo == nullptr)
    vbo = std::make_unique<GLArrayBuffer>();

  const GLsizeiptr bytes =
    GLsizeiptr(sizeof(MeshVertex) * n_triangles * 3);
  vbo->Load(bytes, vertices);
  triangle_count = n_triangles;
}

void
ForwardViewTerrainShader::Draw(const DrawParams &params) noexcept
{
  if (program == nullptr || vbo == nullptr || triangle_count == 0)
    return;

  program->Use();
  DisableSolidShaderAttributes();

  glUniformMatrix4fv(loc_projection, 1, GL_FALSE,
                     glm::value_ptr(params.projection));
  glUniformMatrix4fv(loc_modelview, 1, GL_FALSE,
                     glm::value_ptr(params.modelview));
  glUniform2f(loc_scroll, params.scroll.x, params.scroll.y);
  glUniform1f(loc_ref_alt, params.ref_alt);
  glUniform1f(loc_vertical_ref, params.vertical_ref);
  glUniform1f(loc_eye_z, params.eye_z);
  glUniform3f(loc_sun_dir, params.sun_dir.x,
              params.sun_dir.y, params.sun_dir.z);
  glUniform1f(loc_sun_active, params.sun_active ? 1.f : 0.f);
  glUniform1f(loc_vert_exag,
              float(ForwardViewGeometry::VERTICAL_EXAGGERATION - 1.));
  glUniform1f(loc_topo_active, params.topo_active ? 1.f : 0.f);
  glUniform1f(loc_terrain_ramp_weight, params.terrain_ramp_weight);
  glUniform1i(loc_topo_tex, 0);

  if (params.topo_active)
    glActiveTexture(GL_TEXTURE0);

  glUniform1f(loc_earth_radius, float(FAISphere::REARTH));

  vbo->Bind();
  glEnableVertexAttribArray(ATTR_MESH);
  glEnableVertexAttribArray(ATTR_MATERIAL);
  glEnableVertexAttribArray(ATTR_NORMAL);
  glEnableVertexAttribArray(ATTR_SHADOW);
  glEnableVertexAttribArray(ATTR_TOPO_UV);
  glEnableVertexAttribArray(ATTR_BASE_RGB);

  using V = MeshVertex;
  glVertexAttribPointer(ATTR_MESH, 3, GL_FLOAT, GL_FALSE, sizeof(V),
                        reinterpret_cast<const GLvoid *>(0));
  glVertexAttribPointer(ATTR_MATERIAL, 1, GL_FLOAT, GL_FALSE, sizeof(V),
                        reinterpret_cast<const GLvoid *>(offsetof(V, material)));
  glVertexAttribPointer(ATTR_BASE_RGB, 3, GL_FLOAT, GL_FALSE, sizeof(V),
                        reinterpret_cast<const GLvoid *>(offsetof(V, base_r)));
  glVertexAttribPointer(ATTR_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(V),
                        reinterpret_cast<const GLvoid *>(offsetof(V, nx)));
  glVertexAttribPointer(ATTR_SHADOW, 1, GL_FLOAT, GL_FALSE, sizeof(V),
                        reinterpret_cast<const GLvoid *>(offsetof(V, shadow_cap)));
  glVertexAttribPointer(ATTR_TOPO_UV, 2, GL_FLOAT, GL_FALSE, sizeof(V),
                        reinterpret_cast<const GLvoid *>(offsetof(V, topo_u)));

  glDrawArrays(GL_TRIANGLES, 0, GLsizei(triangle_count * 3));

  glDisableVertexAttribArray(ATTR_BASE_RGB);
  glDisableVertexAttribArray(ATTR_TOPO_UV);
  glDisableVertexAttribArray(ATTR_SHADOW);
  glDisableVertexAttribArray(ATTR_NORMAL);
  glDisableVertexAttribArray(ATTR_MATERIAL);
  glDisableVertexAttribArray(ATTR_MESH);
  GLArrayBuffer::Unbind();
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  if (OpenGL::solid_shader != nullptr)
    OpenGL::solid_shader->Use();
}

#endif
