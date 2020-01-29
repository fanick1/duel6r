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

#include "Color.h"
#include "Type.h"

namespace Duel6 {
    class SpriteEffect {
    public:
        class SpriteEffectDefinition {

        private:
            Color additiveColor0;
            Float32 timeout0;

            Color additiveColor1;
            Float32 timeout1;

            Color additiveColor2;
            Float32 timeout2;

        public:
            SpriteEffectDefinition(const Color & additiveColor0 = Color::ZERO, Float32 timeout0 = 0,
                                   const Color & additiveColor1 = Color::ZERO,
                                   Float32 timeout1 = 0,
                                   const Color & additiveColor2 = Color::ZERO,
                                   Float32 timeout2 = 0)
                : additiveColor0(additiveColor0),
                  timeout0(timeout0),
                  additiveColor1(additiveColor1),
                  timeout1(timeout0 + timeout1),
                  additiveColor2(additiveColor2),
                  timeout2(timeout0 + timeout1 + timeout2) {
            }
            SpriteEffectDefinition &operator=(SpriteEffectDefinition & definition) {
                additiveColor0 = definition.additiveColor0;
                timeout0 = definition.timeout0;
                additiveColor1 = definition.additiveColor1;
                timeout1 = definition.timeout1;
                additiveColor2 = definition.additiveColor2;
                timeout2 = definition.timeout2;
                return *this;

            };
             SpriteEffectDefinition (SpriteEffectDefinition & definition):
                additiveColor0(definition.additiveColor0),
                timeout0(definition.timeout0),
                additiveColor1(definition.additiveColor1),
                timeout1(definition.timeout1),
                additiveColor2(definition.additiveColor2),
                timeout2(definition.timeout2){

            };
            const Color& getAdditiveColor0() const
            {
                return additiveColor0;
            }

            const Color& getAdditiveColor1() const
            {
                return additiveColor1;
            }

            const Color& getAdditiveColor2() const
            {
                return additiveColor2;
            }

            const Float32 getTimeout0() const
            {
                return timeout0;
            }

            const Float32 getTimeout1() const
            {
                return timeout1;
            }

            const Float32 getTimeout2() const
            {
                return timeout2;
            }

        };
        public:
        static const SpriteEffectDefinition BLINK_RED;
        static const SpriteEffectDefinition BLINK_GREEN;
        static const SpriteEffectDefinition BLINK_BLUE;
        static const SpriteEffectDefinition BLINK_WHITE;
        static const SpriteEffectDefinition BLINK_YELLOW;
        static const SpriteEffectDefinition BLINK_RED_DOUBLE;
        static const SpriteEffectDefinition BLINK_GREEN_DOUBLE;
        static const SpriteEffectDefinition BLINK_BLUE_DOUBLE;
        static const SpriteEffectDefinition BLINK_WHITE_DOUBLE;
        static const SpriteEffectDefinition BLINK_YELLOW_DOUBLE;
        static const SpriteEffectDefinition NONE;

    private:
        const SpriteEffectDefinition * spriteEffectDefinition;
        const Color * currentColor;
        Float32 timer = 0;
        public:

        SpriteEffect(const SpriteEffectDefinition & spriteEffectDefinition)
            :
              spriteEffectDefinition(&spriteEffectDefinition),
              currentColor(&spriteEffectDefinition.getAdditiveColor0()) {
        }

        SpriteEffect &operator=(const SpriteEffect & effect);

        void update(Float32 elapsedTime) {
            timer += elapsedTime;
            if (timer > spriteEffectDefinition->getTimeout2()) {
                currentColor = &Color::ZERO;
            } else if (timer > spriteEffectDefinition->getTimeout1()) {
                currentColor = &spriteEffectDefinition->getAdditiveColor2();
            } else if (timer > spriteEffectDefinition->getTimeout0()) {
                currentColor = &spriteEffectDefinition->getAdditiveColor1();
            } else if (timer > 0) {
                currentColor = &spriteEffectDefinition->getAdditiveColor0();
            }
        }

        const Color & getAdditiveColor() const {
            return *currentColor;
        }

    };
}
