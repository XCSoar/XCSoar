/*
Copyright_License {

  XCSoar Glide Compute5r - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Shaders.hpp"
#include "Program.hpp"
#include "Globals.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <stdio.h>

namespace OpenGL {
  GLProgram *solid_shader;
  GLint solid_projection, solid_modelview;

  GLProgram *texture_shader;
  GLint texture_projection, texture_texture;

  GLProgram *invert_shader;
  GLint invert_projection, invert_texture;

  GLProgram *alpha_shader;
  GLint alpha_projection, alpha_texture;

  GLProgram *combine_texture_shader;
  GLint combine_texture_projection, combine_texture_texture;
}

#ifdef HAVE_GLES
#define GLSL_VERSION
#define GLSL_PRECISION "precision mediump float;\n"
#else
#define GLSL_VERSION "#version 120\n"
#define GLSL_PRECISION
#endif

static constexpr char solid_vertex_shader[] =
  GLSL_VERSION
  "uniform mat4 projection;"
  "uniform mat4 modelview;"
  "attribute vec4 translate;"
  "attribute vec4 position;"
  "attribute vec4 color;"
  "varying vec4 colorvar;"
  "void main() {"
  "  gl_Position = projection * (modelview * position + translate);"
  "  colorvar = color;"
  "}";

static constexpr char solid_fragment_shader[] =
  GLSL_VERSION
  GLSL_PRECISION
  "varying vec4 colorvar;"
  "void main() {"
  "  gl_FragColor = colorvar;"
  "}";

static constexpr char texture_vertex_shader[] =
  GLSL_VERSION
  "uniform mat4 projection;"
  "attribute vec4 translate;"
  "attribute vec4 position;"
  "attribute vec2 texcoord;"
  "varying vec2 texcoordvar;"
  "attribute vec4 color;"
  "varying vec4 colorvar;"
  "void main() {"
  "  gl_Position = projection * (position + translate);"
  "  texcoordvar = texcoord;"
  "  colorvar = color;"
  "}";

static constexpr char texture_fragment_shader[] =
  GLSL_VERSION
  GLSL_PRECISION
  "uniform sampler2D texture;"
  "varying vec2 texcoordvar;"
  "void main() {"
  "  gl_FragColor = texture2D(texture, texcoordvar);"
  "}";

static const char *const invert_vertex_shader = texture_vertex_shader;
static constexpr char invert_fragment_shader[] =
  GLSL_VERSION
  GLSL_PRECISION
  "uniform sampler2D texture;"
  "varying vec2 texcoordvar;"
  "void main() {"
  "  vec4 color = texture2D(texture, texcoordvar);"
  "  gl_FragColor = vec4(vec3(1) - color.rgb, color.a);"
  "}";

static const char *const alpha_vertex_shader = texture_vertex_shader;
static constexpr char alpha_fragment_shader[] =
  GLSL_VERSION
  GLSL_PRECISION
  "uniform sampler2D texture;"
  "varying vec4 colorvar;"
  "varying vec2 texcoordvar;"
  "void main() {"
  "  gl_FragColor = vec4(colorvar.rgb, texture2D(texture, texcoordvar).a);"
  "}";

static const char *const combine_texture_vertex_shader = texture_vertex_shader;
static constexpr char combine_texture_fragment_shader[] =
  GLSL_VERSION
  GLSL_PRECISION
  "uniform sampler2D texture;"
  "varying vec4 colorvar;"
  "varying vec2 texcoordvar;"
  "void main() {"
  "  gl_FragColor = colorvar * texture2D(texture, texcoordvar);"
  "}";

static void
CompileAttachShader(GLProgram &program, GLenum type, const char *code)
{
  GLShader shader(type);
  shader.Source(code);
  shader.Compile();

  if (shader.GetCompileStatus() != GL_TRUE) {
    char log[4096];
    shader.GetInfoLog(log, sizeof(log));
    fprintf(stderr, "Shader compiler failed: %s\n", log);
  }

  program.AttachShader(shader);
}

static GLProgram *
CompileProgram(const char *vertex_shader, const char *fragment_shader)
{
  GLProgram *program = new GLProgram();
  CompileAttachShader(*program, GL_VERTEX_SHADER, vertex_shader);
  CompileAttachShader(*program, GL_FRAGMENT_SHADER, fragment_shader);
  program->Link();

  if (program->GetLinkStatus() != GL_TRUE) {
    char log[4096];
    program->GetInfoLog(log, sizeof(log));
    fprintf(stderr, "Shader linker failed: %s\n", log);
  }

  return program;
}

static void
LinkProgram(GLProgram &program)
{
  program.Link();

  if (program.GetLinkStatus() != GL_TRUE) {
    char log[4096];
    program.GetInfoLog(log, sizeof(log));
    fprintf(stderr, "Shader linker failed: %s\n", log);
  }
}

void
OpenGL::InitShaders()
{
  DeinitShaders();

  solid_shader = CompileProgram(solid_vertex_shader, solid_fragment_shader);
  solid_shader->BindAttribLocation(Attribute::TRANSLATE, "translate");
  solid_shader->BindAttribLocation(Attribute::POSITION, "position");
  solid_shader->BindAttribLocation(Attribute::COLOR, "color");
  LinkProgram(*solid_shader);

  solid_projection = solid_shader->GetUniformLocation("projection");
  solid_modelview = solid_shader->GetUniformLocation("modelview");

  solid_shader->Use();
  glUniformMatrix4fv(solid_modelview, 1, GL_FALSE,
                     glm::value_ptr(glm::mat4()));

  texture_shader = CompileProgram(texture_vertex_shader, texture_fragment_shader);
  texture_shader->BindAttribLocation(Attribute::TRANSLATE, "translate");
  texture_shader->BindAttribLocation(Attribute::POSITION, "position");
  texture_shader->BindAttribLocation(Attribute::TEXCOORD, "texcoord");
  LinkProgram(*texture_shader);

  texture_projection = texture_shader->GetUniformLocation("projection");
  texture_texture = texture_shader->GetUniformLocation("texture");

  texture_shader->Use();
  glUniform1i(texture_texture, 0);

  invert_shader = CompileProgram(invert_vertex_shader, invert_fragment_shader);
  invert_shader->BindAttribLocation(Attribute::TRANSLATE, "translate");
  invert_shader->BindAttribLocation(Attribute::POSITION, "position");
  invert_shader->BindAttribLocation(Attribute::TEXCOORD, "texcoord");
  LinkProgram(*invert_shader);

  invert_projection = invert_shader->GetUniformLocation("projection");
  invert_texture = invert_shader->GetUniformLocation("texture");

  invert_shader->Use();
  glUniform1i(invert_texture, 0);

  alpha_shader = CompileProgram(alpha_vertex_shader, alpha_fragment_shader);
  alpha_shader->BindAttribLocation(Attribute::TRANSLATE, "translate");
  alpha_shader->BindAttribLocation(Attribute::POSITION, "position");
  alpha_shader->BindAttribLocation(Attribute::TEXCOORD, "texcoord");
  alpha_shader->BindAttribLocation(Attribute::COLOR, "color");
  LinkProgram(*alpha_shader);

  alpha_projection = alpha_shader->GetUniformLocation("projection");
  alpha_texture = alpha_shader->GetUniformLocation("texture");

  alpha_shader->Use();
  glUniform1i(alpha_texture, 0);

  combine_texture_shader = CompileProgram(combine_texture_vertex_shader,
                                          combine_texture_fragment_shader);
  combine_texture_shader->BindAttribLocation(Attribute::TRANSLATE, "translate");
  combine_texture_shader->BindAttribLocation(Attribute::POSITION, "position");
  combine_texture_shader->BindAttribLocation(Attribute::TEXCOORD, "texcoord");
  combine_texture_shader->BindAttribLocation(Attribute::COLOR, "color");
  LinkProgram(*combine_texture_shader);

  combine_texture_projection =
    combine_texture_shader->GetUniformLocation("projection");
  combine_texture_texture =
    combine_texture_shader->GetUniformLocation("texture");

  combine_texture_shader->Use();
  glUniform1i(combine_texture_texture, 0);

  glVertexAttrib4f(Attribute::TRANSLATE, 0, 0, 0, 0);
}

void
OpenGL::DeinitShaders()
{
  delete solid_shader;
  solid_shader = nullptr;
}

void
OpenGL::UpdateShaderProjectionMatrix()
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
}
