////////////////////////////////////////////////////////////////////////////////
//
// shader.h
//
//   CLCL: CAVELib Compatible Library
//
//     Copyright 2015-2019 Shintaro Kawahara(kawahara@jamstec.go.jp).
//     All rights reserved.
//
//  Shader class from a sample program included in ZED SDK:
//     Copyright (c) 2018, STEREOLABS.
//
////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <GL/glew.h>

class Shader {
public:
    Shader(const GLchar* vs, const GLchar* fs);
    ~Shader();

    GLuint getProgramId();

    static const GLint ATTRIB_VERTICES_POS = 0;
    static const GLint ATTRIB_TEXTURE2D_POS = 1;
private:
    bool compile(GLuint &shaderId, GLenum type, const GLchar* src);
    GLuint verterxId_;
    GLuint fragmentId_;
    GLuint programId_;
};
