/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
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

#ifndef XCSOAR_SCREEN_OPENGL_PROGRAM_HPP
#define XCSOAR_SCREEN_OPENGL_PROGRAM_HPP

#include "System.hpp"
#include "Compiler.h"

/**
 * This class represents an OpenGL 3.0 / ES2.0 shader.
 */
class GLShader {
  const GLuint id;

public:
  explicit GLShader(GLenum type):id(glCreateShader(type)) {}

  ~GLShader() {
    glDeleteShader(id);
  }

  GLuint GetId() const {
    return id;
  }

  void Source(const char *_source) {
    const GLchar *source = (const GLchar *)_source;
    glShaderSource(id, 1, &source, nullptr);
  }

  void Compile() {
    glCompileShader(id);
  }

  gcc_pure
  GLint GetCompileStatus() const {
    GLint status;
    glGetShaderiv(id, GL_COMPILE_STATUS, &status);
    return status;
  }

  gcc_pure
  GLint GetInfoLogLength() const {
    GLint length;
    glGetShaderiv(id, GL_INFO_LOG_LENGTH, &length);
    return length;
  }

  void GetInfoLog(char *buffer, GLsizei max_size) {
    glGetShaderInfoLog(id, max_size, nullptr, (GLchar *)buffer);
  }
};

/**
 * This class represents an OpenGL 3.0 / ES2.0 program.
 */
class GLProgram {
  const GLuint id;

public:
  GLProgram():id(glCreateProgram()) {}

  ~GLProgram() {
    glDeleteProgram(id);
  }

  GLuint GetId() const {
    return id;
  }

  void AttachShader(const GLShader &shader) {
    glAttachShader(id, shader.GetId());
  }

  void Link() {
    glLinkProgram(id);
  }

  gcc_pure
  GLint GetLinkStatus() const {
    GLint status;
    glGetProgramiv(id, GL_LINK_STATUS, &status);
    return status;
  }

  gcc_pure
  GLint GetInfoLogLength() const {
    GLint length;
    glGetProgramiv(id, GL_INFO_LOG_LENGTH, &length);
    return length;
  }

  void GetInfoLog(char *buffer, GLsizei max_size) {
    glGetProgramInfoLog(id, max_size, nullptr, (GLchar *)buffer);
  }

  void Validate() {
    glValidateProgram(id);
  }

  void Use() {
    glUseProgram(id);
  }

  gcc_pure
  GLint GetUniformLocation(const char *name) const {
    return glGetUniformLocation(id, (const GLchar *)name);
  }

  gcc_pure
  GLint GetAttribLocation(const char *name) const {
    return glGetAttribLocation(id, (const GLchar *)name);
  }

  void BindAttribLocation(GLuint index, const char *name) {
    glBindAttribLocation(id, index, (const GLchar *)name);
  }
};

#endif
