// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Shaders.hpp"
#include "Program.hpp"
#include "Attribute.hpp"
#include "Globals.hpp"
#include "ui/dim/Point.hpp"
#include "LogFile.hpp"
#include "lib/fmt/RuntimeError.hxx"
#include "util/Exception.hxx"

#include <glm/gtc/type_ptr.hpp>

namespace OpenGL {

GLProgram *solid_shader;
GLint solid_projection, solid_modelview, solid_translate;

GLProgram *texture_shader;
GLint texture_projection, texture_texture, texture_translate;

GLProgram *invert_shader;
GLint invert_projection, invert_texture, invert_translate;

GLProgram *alpha_fix_color_shader = nullptr;
GLint alpha_fix_color_projection = 0, alpha_fix_color_texture = 0,
  alpha_fix_color_translate = 0, alpha_fix_color_color = 0;

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

GLProgram *round_line_shader;
GLint round_line_projection, round_line_translate,
  round_line_softness, round_line_min_coverage, round_line_color;

GLProgram *hillshade_shader;
GLint hillshade_projection, hillshade_translate,
  hillshade_height_tex, hillshade_ramp_tex,
  hillshade_texel_step, hillshade_sun, hillshade_contrast,
  hillshade_height_slope_factor, hillshade_height_div,
  hillshade_q, hillshade_do_shading, hillshade_contour_div,
  hillshade_height_texel;

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
      vec4 texture_sample;
      vec4 color;
      vec4 alpha;
      texture_sample = texture2D(texture, texcoordvar);
      alpha = vec4(vec3(0),texture_sample.a);
      color = vec4(vec3(1) - texture_sample.rgb, 0);
      /* Workaround for the LIMA GPU driver for OpenVario:
       * Assigning the alpha component directly leads to
       * render errors in blending mode
       * Full-vector additions or multiplications however work.
       */
      gl_FragColor = color + alpha;
    }
)glsl";

static constexpr char alpha_fix_color_vertex_shader[] =
  GLSL_VERSION
  R"glsl(
    uniform mat4 projection;
    uniform vec2 translate;
    attribute vec4 position;
    attribute vec2 texcoord;
    varying vec2 texcoordvar;
    void main() {
      gl_Position = position;
      gl_Position.xy += translate;
      gl_Position = projection * gl_Position;
      texcoordvar = texcoord;
    }
)glsl";
static constexpr char alpha_fix_color_fragment_shader[] =
  GLSL_VERSION
  GLSL_PRECISION
  R"glsl(
    uniform sampler2D texture;
    varying vec2 texcoordvar;
    uniform vec4 color;
    void main() {
      gl_FragColor = vec4(color.rgb, texture2D(texture, texcoordvar).a);
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

static constexpr char round_line_vertex_shader[] =
  GLSL_VERSION
  GLSL_PRECISION
  R"glsl(
    uniform mat4 projection;
    uniform vec2 translate;
    attribute vec4 position;
    attribute vec4 texcoord;
    attribute float radius;
    varying highp vec2 vert_pos;
    varying highp vec4 segment;
    varying highp float radiusvar;
    void main() {
      vert_pos = position.xy;
      segment = texcoord;
      radiusvar = radius;
      gl_Position = position;
      gl_Position.xy += translate;
      gl_Position = projection * gl_Position;
    }
)glsl";

static constexpr char round_line_fragment_shader[] =
  GLSL_VERSION
  GLSL_PRECISION
  R"glsl(
    uniform float softness;
    uniform float min_coverage;
    uniform vec4 color;
    varying highp vec2 vert_pos;
    varying highp vec4 segment;
    varying highp float radiusvar;
    void main() {
      highp vec2 a = segment.xy;
      highp vec2 ab = segment.zw - a;
      highp float length2 = dot(ab, ab);
      highp float t = length2 > 0.0
        ? clamp(dot(vert_pos - a, ab) / length2, 0.0, 1.0)
        : 0.0;
      highp float d = distance(vert_pos, a + t * ab);

      /* how much of the pixel is inside, faded over the soft edge;
         the S curve makes a wide soft edge look like a gradient
         instead of a flat band */
      float coverage = smoothstep(0.0, 1.0,
                                  (radiusvar - d) / softness + 0.5);
      if (coverage <= 0.0 || coverage < min_coverage) discard;

      gl_FragColor = vec4(color.rgb, color.a * coverage);
    }
)glsl";

static const char *const hillshade_vertex_shader = texture_vertex_shader;

static constexpr char hillshade_fragment_shader[] =
  GLSL_VERSION
  R"glsl(
    /* Always mediump: Mali-400 has no fragment highp, and forming
       int16 as 0..65535 is not representable (mediump is ±16384). */
    precision mediump float;
    uniform sampler2D height_tex;
    uniform sampler2D ramp_tex;
    uniform vec2 texel_step;
    uniform vec3 sun;
    uniform float contrast;
    uniform float height_slope_factor;
    uniform float height_div;
    uniform float q;
    uniform float do_shading;
    uniform float contour_div;
    uniform vec2 height_texel;
    varying vec2 texcoordvar;

    vec2 unpack_la(vec4 t) {
      return vec2(floor(t.r * 255.0 + 0.5),
                  floor(t.a * 255.0 + 0.5));
    }

    /* TerrainHeight::IsSpecial(): value <= -30000.
       -32768 → hi=128; -30000 → hi=138 lo=208.  Do not reconstruct
       those heights (they do not fit in mediump). */
    bool is_special(vec2 b) {
      return b.y >= 128.0 &&
             (b.y < 138.0 || (b.y == 138.0 && b.x <= 208.0));
    }

    float height(vec2 b) {
      float h = (b.y >= 128.0)
        ? ((b.y - 256.0) * 256.0 + b.x)
        : (b.y * 256.0 + b.x);
      return clamp(h, -16383.0, 16383.0);
    }

    vec4 ramp_lookup(float h_idx, float sindex) {
      return texture2D(ramp_tex, vec2((h_idx + 0.5) / 256.0,
                                      (sindex + 64.5) / 128.0));
    }

    float contour_interval(float h) {
      if (h <= 0.0)
        return 0.0;
      return min(254.0, floor(h / contour_div));
    }

    void main() {
      vec2 b = unpack_la(texture2D(height_tex, texcoordvar));
      if (is_special(b)) {
        gl_FragColor = ramp_lookup(255.0, 0.0);
        return;
      }

      float h = height(b);
      float h_idx = min(254.0, max(0.0, floor(h / height_div)));
      float sindex = 0.0;

      if (do_shading > 0.5) {
        vec2 b_above = unpack_la(texture2D(height_tex,
            texcoordvar - vec2(0.0, texel_step.y)));
        vec2 b_below = unpack_la(texture2D(height_tex,
            texcoordvar + vec2(0.0, texel_step.y)));
        vec2 b_left = unpack_la(texture2D(height_tex,
            texcoordvar - vec2(texel_step.x, 0.0)));
        vec2 b_right = unpack_la(texture2D(height_tex,
            texcoordvar + vec2(texel_step.x, 0.0)));

        if (!is_special(b_above) && !is_special(b_below) &&
            !is_special(b_left) && !is_special(b_right)) {
          /* Same n as GenerateSlopeImage, divided by p20*p31 so
             mediump cannot overflow (dd0 = p22*p31).  sval/sindex
             use trunc-toward-zero, matching the CPU ints. */
          float p32 = clamp(height(b_above) - height(b_below),
                            -512.0, 512.0);
          float p22 = clamp(height(b_right) - height(b_left),
                            -512.0, 512.0);
          float p20 = max(2.0 * q, 1.0);
          float p31 = max(2.0 * q, 1.0);
          float n0 = p22 / p20;
          float n1 = p32 / p31;
          float n2 = height_slope_factor;
          float mag = sqrt(n0 * n0 + n1 * n1 + n2 * n2);
          float num = n2 * sun.z + n0 * sun.x + n1 * sun.y;
          float sval = mag > 0.0 ? float(int(num / mag)) : 0.0;
          sindex = float(int((sval - sun.z) * contrast / 128.0));
          sindex = clamp(sindex, -63.0, 63.0);
        }
      }

      if (contour_div > 0.5) {
        vec2 b_up = unpack_la(texture2D(height_tex,
            texcoordvar - vec2(0.0, height_texel.y)));
        vec2 b_lf = unpack_la(texture2D(height_tex,
            texcoordvar - vec2(height_texel.x, 0.0)));
        float h_up = is_special(b_up) ? 0.0 : height(b_up);
        float h_lf = is_special(b_lf) ? 0.0 : height(b_lf);
        float ci = contour_interval(h);
        if (ci != contour_interval(h_up) ||
            ci != contour_interval(h_lf))
          sindex = -64.0;
      }

      gl_FragColor = ramp_lookup(h_idx, sindex);
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

  alpha_fix_color_shader = CompileProgram(alpha_fix_color_vertex_shader,
    alpha_fix_color_fragment_shader);
  alpha_fix_color_shader->BindAttribLocation(Attribute::POSITION, "position");
  alpha_fix_color_shader->BindAttribLocation(Attribute::TEXCOORD, "texcoord");
  LinkProgram(*alpha_fix_color_shader);

  alpha_fix_color_projection = alpha_fix_color_shader->GetUniformLocation("projection");
  alpha_fix_color_texture = alpha_fix_color_shader->GetUniformLocation("texture");
  alpha_fix_color_translate = alpha_fix_color_shader->GetUniformLocation("translate");
  alpha_fix_color_color = alpha_fix_color_shader->GetUniformLocation("color");

  alpha_fix_color_shader->Use();
  glUniform1i(alpha_fix_color_texture, 0);

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

  round_line_shader = CompileProgram(round_line_vertex_shader,
                                     round_line_fragment_shader);
  round_line_shader->BindAttribLocation(Attribute::POSITION, "position");
  round_line_shader->BindAttribLocation(Attribute::TEXCOORD, "texcoord");
  round_line_shader->BindAttribLocation(Attribute::RADIUS, "radius");
  LinkProgram(*round_line_shader);

  round_line_projection = round_line_shader->GetUniformLocation("projection");
  round_line_translate = round_line_shader->GetUniformLocation("translate");
  round_line_softness = round_line_shader->GetUniformLocation("softness");
  round_line_min_coverage =
    round_line_shader->GetUniformLocation("min_coverage");
  round_line_color = round_line_shader->GetUniformLocation("color");

  try {
    hillshade_shader = CompileProgram(hillshade_vertex_shader,
                                      hillshade_fragment_shader);
    hillshade_shader->BindAttribLocation(Attribute::POSITION, "position");
    hillshade_shader->BindAttribLocation(Attribute::TEXCOORD, "texcoord");
    LinkProgram(*hillshade_shader);

    hillshade_projection = hillshade_shader->GetUniformLocation("projection");
    hillshade_translate = hillshade_shader->GetUniformLocation("translate");
    hillshade_height_tex = hillshade_shader->GetUniformLocation("height_tex");
    hillshade_ramp_tex = hillshade_shader->GetUniformLocation("ramp_tex");
    hillshade_texel_step = hillshade_shader->GetUniformLocation("texel_step");
    hillshade_sun = hillshade_shader->GetUniformLocation("sun");
    hillshade_contrast = hillshade_shader->GetUniformLocation("contrast");
    hillshade_height_slope_factor =
      hillshade_shader->GetUniformLocation("height_slope_factor");
    hillshade_height_div = hillshade_shader->GetUniformLocation("height_div");
    hillshade_q = hillshade_shader->GetUniformLocation("q");
    hillshade_do_shading = hillshade_shader->GetUniformLocation("do_shading");
    hillshade_contour_div = hillshade_shader->GetUniformLocation("contour_div");
    hillshade_height_texel = hillshade_shader->GetUniformLocation("height_texel");

    hillshade_shader->Use();
    glUniform1i(hillshade_height_tex, 0);
    glUniform1i(hillshade_ramp_tex, 1);
  } catch (...) {
    delete hillshade_shader;
    hillshade_shader = nullptr;
    LogFmt("OpenGL: hillshade shader failed ({}); using CPU",
           GetFullMessage(std::current_exception()));
  }
}

void
OpenGL::DeinitShaders() noexcept
{
  delete round_line_shader;
  round_line_shader = nullptr;
  delete hillshade_shader;
  hillshade_shader = nullptr;
  delete filled_circle_shader;
  filled_circle_shader = nullptr;
  delete circle_outline_shader;
  circle_outline_shader = nullptr;
  delete dashed_shader;
  dashed_shader = nullptr;
  delete combine_texture_shader;
  combine_texture_shader = nullptr;
  delete alpha_fix_color_shader;
  alpha_fix_color_shader = nullptr;
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
  alpha_fix_color_shader->Use();
  glUniformMatrix4fv(alpha_fix_color_projection, 1, GL_FALSE,
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

  round_line_shader->Use();
  glUniformMatrix4fv(round_line_projection, 1, GL_FALSE,
                     glm::value_ptr(projection_matrix));

  if (hillshade_shader != nullptr) {
    hillshade_shader->Use();
    glUniformMatrix4fv(hillshade_projection, 1, GL_FALSE,
                       glm::value_ptr(projection_matrix));
  }
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

  alpha_fix_color_shader->Use();
  glUniform2f(alpha_fix_color_translate, t.x, t.y);

  combine_texture_shader->Use();
  glUniform2f(combine_texture_translate, t.x, t.y);

  dashed_shader->Use();
  glUniform2f(dashed_translate, t.x, t.y);

  circle_outline_shader->Use();
  glUniform2f(circle_outline_translate, t.x, t.y);

  filled_circle_shader->Use();
  glUniform2f(filled_circle_translate, t.x, t.y);

  round_line_shader->Use();
  glUniform2f(round_line_translate, t.x, t.y);

  if (hillshade_shader != nullptr) {
    hillshade_shader->Use();
    glUniform2f(hillshade_translate, t.x, t.y);
  }
}
