// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Shaders.hpp"
#include "Program.hpp"
#include "Attribute.hpp"
#include "Globals.hpp"
#include "ui/dim/Point.hpp"
#include "lib/fmt/RuntimeError.hxx"

#include <glm/gtc/type_ptr.hpp>

namespace OpenGL {

GLProgram *solid_shader;
GLint solid_projection, solid_modelview, solid_translate;

GLProgram *texture_shader;
GLint texture_projection, texture_texture, texture_translate;

GLProgram *invert_shader;
GLint invert_projection, invert_texture, invert_translate;

GLProgram *alpha_shader;
GLint alpha_projection, alpha_texture, alpha_translate;

GLProgram *combine_texture_shader;
GLint combine_texture_projection, combine_texture_texture,
  combine_texture_translate;

GLProgram *dashed_shader;
GLint dashed_projection, dashed_translate,
  dashed_resolution, dashed_start, dashed_period, dashed_ratio;

GLProgram *circle_outline_shader;
GLint circle_outline_projection, circle_outline_translate,
  circle_outline_center, circle_outline_radius1, circle_outline_radius2,
  circle_outline_color;

GLProgram *filled_circle_shader;
GLint filled_circle_projection, filled_circle_translate,
  filled_circle_center, filled_circle_radius1, filled_circle_radius2,
  filled_circle_color1, filled_circle_color2;

} // namespace OpenGL

#define GLSL_VERSION "#version 100\n"
#define GLSL_PRECISION "precision mediump float;\n"

static constexpr char solid_vertex_shader[] =
  GLSL_VERSION
  R"glsl(
    uniform mat4 projection;
    uniform mat4 modelview;
    uniform vec2 translate;
    attribute vec4 position;
    attribute vec4 color;
    varying vec4 colorvar;
    void main() {
      gl_Position = modelview * position;
      gl_Position.xy += translate;
      gl_Position = projection * gl_Position;
      colorvar = color;
    }
)glsl";

static constexpr char solid_fragment_shader[] =
  GLSL_VERSION
  GLSL_PRECISION
  R"glsl(
    varying vec4 colorvar;
    void main() {
      gl_FragColor = colorvar;
    }
)glsl";

static constexpr char texture_vertex_shader[] =
  GLSL_VERSION
  R"glsl(
    uniform mat4 projection;
    uniform vec2 translate;
    attribute vec4 position;
    attribute vec2 texcoord;
    varying vec2 texcoordvar;
    attribute vec4 color;
    varying vec4 colorvar;
    void main() {
      gl_Position = position;
      gl_Position.xy += translate;
      gl_Position = projection * gl_Position;
      texcoordvar = texcoord;
      colorvar = color;
    }
)glsl";

static constexpr char texture_fragment_shader[] =
  GLSL_VERSION
  GLSL_PRECISION
  R"glsl(
    uniform sampler2D texture;
    varying vec2 texcoordvar;
    void main() {
      gl_FragColor = texture2D(texture, texcoordvar);
    }
)glsl";

static const char *const invert_vertex_shader = texture_vertex_shader;
static constexpr char invert_fragment_shader[] =
  GLSL_VERSION
  GLSL_PRECISION
  R"glsl(
    uniform sampler2D texture;
    varying vec2 texcoordvar;
    void main() {
      vec4 color = texture2D(texture, texcoordvar);
      gl_FragColor = vec4(vec3(1) - color.rgb, color.a);
    }
)glsl";

static const char *const alpha_vertex_shader = texture_vertex_shader;
static constexpr char alpha_fragment_shader[] =
  GLSL_VERSION
  GLSL_PRECISION
  R"glsl(
    uniform sampler2D texture;
    varying vec4 colorvar;
    varying vec2 texcoordvar;
    void main() {
      gl_FragColor = vec4(colorvar.rgb, texture2D(texture, texcoordvar).a);
    }
)glsl";

static const char *const combine_texture_vertex_shader = texture_vertex_shader;
static constexpr char combine_texture_fragment_shader[] =
  GLSL_VERSION
  GLSL_PRECISION
  R"glsl(
    uniform sampler2D texture;
    varying vec4 colorvar;
    varying vec2 texcoordvar;
    void main() {
      gl_FragColor = colorvar * texture2D(texture, texcoordvar);
    }
)glsl";

static constexpr char dashed_vertex_shader[] =
  GLSL_VERSION
  R"glsl(
    uniform mat4 projection;
    uniform vec2 translate;
    attribute vec4 position;
    attribute vec4 color;
    varying vec4 colorvar;
    varying vec2 vert_pos;
    void main() {
      gl_Position = position;
      gl_Position.xy += translate;
      gl_Position = projection * gl_Position;
      vert_pos = gl_Position.xy;
      colorvar = color;
    }
)glsl";

/* this fragment shader is a big kludge; with GLES 3.0, we could have
   "flat" parameters and we wouldn't need the "start" parameter, but
   since the Lima GPU driver (Cubieboard 2, OpenVario) doesn't
   support GLES 3.0, we're stuck with GLES 2.0 */
static constexpr char dashed_fragment_shader[] =
  GLSL_VERSION
  GLSL_PRECISION
  R"glsl(
    uniform vec2 start;
    uniform vec2 resolution;
    uniform float period;
    uniform float ratio;
    varying vec2 vert_pos;
    varying vec4 colorvar;
    void main() {
      highp vec2 delta = vert_pos - start;
      highp float dist = length(delta * resolution / 2.0);
      if (fract(dist / period) > ratio)
        discard;

      gl_FragColor = colorvar;
    }
)glsl";

/* using "highp" for "vert_pos" because some (Adreno) GPUs have severe
   rendering (rounding?) errors with "mediump" */
static constexpr char circle_vertex_shader[] =
  GLSL_VERSION
  GLSL_PRECISION
  R"glsl(
    uniform mat4 projection;
    uniform vec2 translate;
    attribute vec4 position;
    varying highp vec2 vert_pos;
    void main() {
      vert_pos = position.xy;
      gl_Position = position;
      gl_Position.xy += translate;
      gl_Position = projection * gl_Position;
    }
)glsl";

static constexpr char circle_outline_fragment_shader[] =
  GLSL_VERSION
  GLSL_PRECISION
  R"glsl(
    uniform vec2 center;
    uniform float radius1;
    uniform float radius2;
    uniform vec4 color;
    varying highp vec2 vert_pos;
    void main() {
      float distance = distance(center, vert_pos);
      if (distance < radius1 || distance > radius2) discard;
      gl_FragColor = color;
    }
)glsl";

static constexpr char filled_circle_fragment_shader[] =
  GLSL_VERSION
  GLSL_PRECISION
  R"glsl(
    uniform vec2 center;
    uniform float radius1;
    uniform float radius2;
    uniform vec4 color1;
    uniform vec4 color2;
    varying highp vec2 vert_pos;
    void main() {
      float distance = distance(center, vert_pos);
      if (distance > radius2) discard;

      if (distance < radius1)
        gl_FragColor = color1;
      else
        gl_FragColor = color2;
    }
)glsl";

static void
CompileAttachShader(GLProgram &program, GLenum type, const char *code)
{
  GLShader shader(type);
  shader.Source(code);
  shader.Compile();

  if (shader.GetCompileStatus() != GL_TRUE) {
    char log[1000];
    shader.GetInfoLog(log, sizeof(log));
    throw FmtRuntimeError("Shader compiler failed: {}", log);
  }

  program.AttachShader(shader);
}

static GLProgram *
CompileProgram(const char *vertex_shader, const char *fragment_shader)
{
  GLProgram *program = new GLProgram();
  CompileAttachShader(*program, GL_VERTEX_SHADER, vertex_shader);
  CompileAttachShader(*program, GL_FRAGMENT_SHADER, fragment_shader);
  return program;
}

static void
LinkProgram(GLProgram &program)
{
  program.Link();

  if (program.GetLinkStatus() != GL_TRUE) {
    char log[1000];
    program.GetInfoLog(log, sizeof(log));
    throw FmtRuntimeError("Shader linker failed: {}", log);
  }
}

void
OpenGL::InitShaders()
{
  DeinitShaders();

  solid_shader = CompileProgram(solid_vertex_shader, solid_fragment_shader);
  solid_shader->BindAttribLocation(Attribute::POSITION, "position");
  solid_shader->BindAttribLocation(Attribute::COLOR, "color");
  LinkProgram(*solid_shader);

  solid_projection = solid_shader->GetUniformLocation("projection");
  solid_modelview = solid_shader->GetUniformLocation("modelview");
  solid_translate = solid_shader->GetUniformLocation("translate");

  solid_shader->Use();
  glUniformMatrix4fv(solid_modelview, 1, GL_FALSE,
                     glm::value_ptr(glm::mat4(1)));

  texture_shader = CompileProgram(texture_vertex_shader, texture_fragment_shader);
  texture_shader->BindAttribLocation(Attribute::POSITION, "position");
  texture_shader->BindAttribLocation(Attribute::TEXCOORD, "texcoord");
  LinkProgram(*texture_shader);

  texture_projection = texture_shader->GetUniformLocation("projection");
  texture_texture = texture_shader->GetUniformLocation("texture");
  texture_translate = texture_shader->GetUniformLocation("translate");

  texture_shader->Use();
  glUniform1i(texture_texture, 0);

  invert_shader = CompileProgram(invert_vertex_shader, invert_fragment_shader);
  invert_shader->BindAttribLocation(Attribute::POSITION, "position");
  invert_shader->BindAttribLocation(Attribute::TEXCOORD, "texcoord");
  LinkProgram(*invert_shader);

  invert_projection = invert_shader->GetUniformLocation("projection");
  invert_texture = invert_shader->GetUniformLocation("texture");
  invert_translate = invert_shader->GetUniformLocation("translate");

  invert_shader->Use();
  glUniform1i(invert_texture, 0);

  alpha_shader = CompileProgram(alpha_vertex_shader, alpha_fragment_shader);
  alpha_shader->BindAttribLocation(Attribute::POSITION, "position");
  alpha_shader->BindAttribLocation(Attribute::TEXCOORD, "texcoord");
  alpha_shader->BindAttribLocation(Attribute::COLOR, "color");
  LinkProgram(*alpha_shader);

  alpha_projection = alpha_shader->GetUniformLocation("projection");
  alpha_texture = alpha_shader->GetUniformLocation("texture");
  alpha_translate = alpha_shader->GetUniformLocation("translate");

  alpha_shader->Use();
  glUniform1i(alpha_texture, 0);

  combine_texture_shader = CompileProgram(combine_texture_vertex_shader,
                                          combine_texture_fragment_shader);
  combine_texture_shader->BindAttribLocation(Attribute::POSITION, "position");
  combine_texture_shader->BindAttribLocation(Attribute::TEXCOORD, "texcoord");
  combine_texture_shader->BindAttribLocation(Attribute::COLOR, "color");
  LinkProgram(*combine_texture_shader);

  combine_texture_projection =
    combine_texture_shader->GetUniformLocation("projection");
  combine_texture_texture =
    combine_texture_shader->GetUniformLocation("texture");
  combine_texture_translate =
    combine_texture_shader->GetUniformLocation("translate");

  combine_texture_shader->Use();
  glUniform1i(combine_texture_texture, 0);

  dashed_shader = CompileProgram(dashed_vertex_shader, dashed_fragment_shader);
  dashed_shader->BindAttribLocation(Attribute::POSITION, "position");
  dashed_shader->BindAttribLocation(Attribute::COLOR, "color");
  LinkProgram(*dashed_shader);

  dashed_projection = dashed_shader->GetUniformLocation("projection");
  dashed_translate = dashed_shader->GetUniformLocation("translate");
  dashed_resolution = dashed_shader->GetUniformLocation("resolution");
  dashed_start = dashed_shader->GetUniformLocation("start");
  dashed_period = dashed_shader->GetUniformLocation("period");
  dashed_ratio = dashed_shader->GetUniformLocation("ratio");

  circle_outline_shader = CompileProgram(circle_vertex_shader, circle_outline_fragment_shader);
  circle_outline_shader->BindAttribLocation(Attribute::POSITION, "position");
  LinkProgram(*circle_outline_shader);

  circle_outline_projection = circle_outline_shader->GetUniformLocation("projection");
  circle_outline_translate = circle_outline_shader->GetUniformLocation("translate");
  circle_outline_center = circle_outline_shader->GetUniformLocation("center");
  circle_outline_radius1 = circle_outline_shader->GetUniformLocation("radius1");
  circle_outline_radius2 = circle_outline_shader->GetUniformLocation("radius2");
  circle_outline_color = circle_outline_shader->GetUniformLocation("color");

  filled_circle_shader = CompileProgram(circle_vertex_shader, filled_circle_fragment_shader);
  filled_circle_shader->BindAttribLocation(Attribute::POSITION, "position");
  LinkProgram(*filled_circle_shader);

  filled_circle_projection = filled_circle_shader->GetUniformLocation("projection");
  filled_circle_translate = filled_circle_shader->GetUniformLocation("translate");
  filled_circle_center = filled_circle_shader->GetUniformLocation("center");
  filled_circle_radius1 = filled_circle_shader->GetUniformLocation("radius1");
  filled_circle_radius2 = filled_circle_shader->GetUniformLocation("radius2");
  filled_circle_color1 = filled_circle_shader->GetUniformLocation("color1");
  filled_circle_color2 = filled_circle_shader->GetUniformLocation("color2");
}

void
OpenGL::DeinitShaders() noexcept
{
  delete filled_circle_shader;
  filled_circle_shader = nullptr;
  delete circle_outline_shader;
  circle_outline_shader = nullptr;
  delete dashed_shader;
  dashed_shader = nullptr;
  delete combine_texture_shader;
  combine_texture_shader = nullptr;
  delete alpha_shader;
  alpha_shader = nullptr;
  delete invert_shader;
  invert_shader = nullptr;
  delete texture_shader;
  texture_shader = nullptr;
  delete solid_shader;
  solid_shader = nullptr;
}

void
OpenGL::UpdateShaderProjectionMatrix() noexcept
{
  alpha_shader->Use();
  glUniformMatrix4fv(alpha_projection, 1, GL_FALSE,
                     glm::value_ptr(projection_matrix));

  invert_shader->Use();
  glUniformMatrix4fv(invert_projection, 1, GL_FALSE,
                     glm::value_ptr(projection_matrix));

  texture_shader->Use();
  glUniformMatrix4fv(texture_projection, 1, GL_FALSE,
                     glm::value_ptr(projection_matrix));

  solid_shader->Use();
  glUniformMatrix4fv(solid_projection, 1, GL_FALSE,
                     glm::value_ptr(projection_matrix));

  combine_texture_shader->Use();
  glUniformMatrix4fv(combine_texture_projection, 1, GL_FALSE,
                     glm::value_ptr(projection_matrix));

  dashed_shader->Use();
  glUniformMatrix4fv(dashed_projection, 1, GL_FALSE,
                     glm::value_ptr(projection_matrix));
  glUniform2f(dashed_resolution, viewport_size.x, viewport_size.y);

  circle_outline_shader->Use();
  glUniformMatrix4fv(circle_outline_projection, 1, GL_FALSE,
                     glm::value_ptr(projection_matrix));

  filled_circle_shader->Use();
  glUniformMatrix4fv(filled_circle_projection, 1, GL_FALSE,
                     glm::value_ptr(projection_matrix));
}

void
OpenGL::UpdateShaderTranslate() noexcept
{
  const FloatPoint2D t(translate);

  solid_shader->Use();
  glUniform2f(solid_translate, t.x, t.y);

  texture_shader->Use();
  glUniform2f(texture_translate, t.x, t.y);

  invert_shader->Use();
  glUniform2f(invert_translate, t.x, t.y);

  alpha_shader->Use();
  glUniform2f(alpha_translate, t.x, t.y);

  combine_texture_shader->Use();
  glUniform2f(combine_texture_translate, t.x, t.y);

  dashed_shader->Use();
  glUniform2f(dashed_translate, t.x, t.y);

  circle_outline_shader->Use();
  glUniform2f(circle_outline_translate, t.x, t.y);

  filled_circle_shader->Use();
  glUniform2f(filled_circle_translate, t.x, t.y);
}
