/*
* Copyright (c) 2006, Ondrej Danek (www.ondrej-danek.net)
* All rights reserved.
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of Ondrej Danek nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
* GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
* OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef DUEL6_RENDERER_GL4_GL4PROGRAM_H
#define DUEL6_RENDERER_GL4_GL4PROGRAM_H

#include "../../Type.h"
#include "../../math/Matrix.h"
#include "GL4Types.h"
#include "GL4Shader.h"

namespace Duel6 {
    class GL4Program {
    private:
        const GL4Shader &vertexShader;
        const GL4Shader &fragmentShader;
        GLuint id;
        static GLuint lastBound;

    public:
        GL4Program(const GL4Shader &vertexShader, const GL4Shader &fragmentShader);
        ~GL4Program();

        void bind() const;
        GLint uniformLocation(const GLchar *name);

        void setUniform(const GLchar *name, Int32 value);
        void setUniform(const GLchar *name, Float32 value);
        void setUniform(const GLchar *name, Float32 value[4]);
        void setUniform(const GLchar *name, const Matrix &value);

        void setUniform(const GLint, Int32 value) const;
        void setUniform(const GLint, Float32 value) const;
        void setUniform(const GLint, Float32 value[4]) const;
        void setUniform(const GLint, const Matrix &value) const;
        GLuint getId() const;

        const GL4Shader &getVertexShader() const;
        const GL4Shader &getFragmentShader() const;
    };

    class GL4Uniform {
    private:
        const GLuint program;
        const GLint location;
    public:
        GL4Uniform(const GLuint program, const GLchar *name)
                : program(program),
                  location(glGetUniformLocation(program, name)) {
        }

        GL4Uniform & operator = (Int32 value){
            glProgramUniform1i(program, location, value);
            return *this;
        }

        GL4Uniform & operator = (Float32 value){
            glProgramUniform1f(program, location, value);
            return *this;
        }

        GL4Uniform & operator = (Float32 value[4]){
            glProgramUniform4fv(program, location, 1, value);
            return *this;
        }

        GL4Uniform & operator = (const Matrix &value){
            glProgramUniformMatrix4fv(program, location, 1, GL_FALSE, value.getStorage());
            return *this;
        }
    };

    class GL4ProgramWrapper {
    private:
        const GL4Program & program;
    public:
        GL4ProgramWrapper(const GL4Program &program) : program(program) {}
        virtual ~GL4ProgramWrapper(){}
        void bind() {
            program.bind();
        }

    };
    class GL4ColorProgramWrapper : public GL4ProgramWrapper {
    public:
        GL4ColorProgramWrapper(const GL4Program &program)
            : GL4ProgramWrapper(program),
              color(program.getId(), "color"),
              mvp(program.getId(), "mvp") {
        }

        GL4Uniform color;
        GL4Uniform mvp;
    };

    class GL4MaterialProgramWrapper : public GL4ProgramWrapper {
    public:
        GL4MaterialProgramWrapper(const GL4Program &program) : GL4ProgramWrapper(program),
           alphaTest(program.getId(), "alphaTest"),
           mvp(program.getId(), "mvp"),
           globalTime(program.getId(), "globalTime"),
           modulateColor(program.getId(), "modulateColor") {
        }

        GL4Uniform alphaTest;
        GL4Uniform mvp;
        GL4Uniform globalTime;
        GL4Uniform modulateColor;
    };
}

#endif