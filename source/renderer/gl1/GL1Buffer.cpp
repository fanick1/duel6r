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

#include <vector>
#include "GL1Buffer.h"
#include "GL1Renderer.h"
#include "../../FaceList.h"

namespace Duel6 {
    namespace {
        const Float32 waveHeight = 0.1f;
    }

    GL1Buffer::GL1Buffer(GL1Renderer &renderer, const FaceList &faceList)
            : renderer(renderer), faceList(faceList) {
    }

    GL1Buffer::~GL1Buffer() {
    }

    void GL1Buffer::update(const FaceList &faceList) {
    }

    void GL1Buffer::render(const Material &material) {
        const auto &faces = faceList.getFaces();
        const Vertex *vertex = faceList.getVertexes().data();

        for (size_t i = 0; i < faces.size(); ++i) {
            size_t v = 4 * i;
            const Face &face = faces[i];

            const Vertex &v1 = vertex[v + 0];
            const Vertex &v2 = vertex[v + 1];
            const Vertex &v3 = vertex[v + 2];
            const Vertex &v4 = vertex[v + 3];

            Float32 currentTexture = face.getCurrentTexture();
            if (v2.getFlag() != Vertex::Flow) {
                    renderer.quad(Vector(v1.x, v1.y, v1.z), Vector(v1.u, v1.v, currentTexture),
                    Vector(v2.x, v2.y, v2.z), Vector(v2.u, v2.v, currentTexture),
                    Vector(v3.x, v3.y, v3.z), Vector(v3.u, v3.v, currentTexture),
                    Vector(v4.x, v4.y, v4.z), Vector(v4.u, v4.v, currentTexture),
                    material);
            } else {
                renderer.quad(getVertexPosition(v1), Vector(v1.u, v1.v, currentTexture),
                    getVertexPosition(v2), Vector(v2.u, v2.v, currentTexture),
                    getVertexPosition(v3), Vector(v3.u, v3.v, currentTexture),
                    getVertexPosition(v4), Vector(v4.u, v4.v, currentTexture),
                    material);
            }

//            vertex += 4;
        }

    }

    Vector GL1Buffer::getVertexPosition(const Duel6::Vertex &vertex) const {
        Float32 y = vertex.y;
        if (vertex.getFlag() == Vertex::Flow) {
            y = y - waveHeight + Math::radianSin(renderer.getGlobalTime() * 2.13f + 1.05f * vertex.x) * waveHeight;
        }
        return Vector(vertex.x, y, vertex.z);
    }
}
