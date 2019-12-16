#include "SpriteEffect.h"
namespace Duel6 {
    const SpriteEffect::SpriteEffectDefinition SpriteEffect::BLINK_RED(Color::RED, 0.1f, Color::ZERO, 0.1f);
    const SpriteEffect::SpriteEffectDefinition SpriteEffect::BLINK_GREEN(Color::GREEN, 0.1f);
    const SpriteEffect::SpriteEffectDefinition SpriteEffect::BLINK_BLUE(Color::BLUE, 0.1f);
    const SpriteEffect::SpriteEffectDefinition SpriteEffect::BLINK_WHITE(Color::WHITE, 0.1f);
    const SpriteEffect::SpriteEffectDefinition SpriteEffect::BLINK_YELLOW(Color::YELLOW, 0.1f);
    const SpriteEffect::SpriteEffectDefinition SpriteEffect::BLINK_RED_DOUBLE(Color::RED, 0.2f, Color::ZERO, 0.1f, Color::RED, 0.2f);
    const SpriteEffect::SpriteEffectDefinition SpriteEffect::BLINK_GREEN_DOUBLE(Color::GREEN, 0.1f, Color::ZERO, 0.1f, Color::GREEN, 0.1f);
    const SpriteEffect::SpriteEffectDefinition SpriteEffect::BLINK_BLUE_DOUBLE(Color::BLUE, 0.1f, Color::ZERO, 0.1f, Color::BLUE, 0.1f);
    const SpriteEffect::SpriteEffectDefinition SpriteEffect::BLINK_WHITE_DOUBLE(Color::WHITE, 0.1f, Color::ZERO, 0.1f, Color::WHITE, 0.1f);
    const SpriteEffect::SpriteEffectDefinition SpriteEffect::BLINK_YELLOW_DOUBLE(Color::YELLOW, 0.2f, Color::ZERO, 0.2f, Color::YELLOW, 0.2f);
    const SpriteEffect::SpriteEffectDefinition SpriteEffect::NONE = {};
    SpriteEffect &SpriteEffect::operator=(const SpriteEffect & effect) {
               if (timer > spriteEffectDefinition->getTimeout2() || spriteEffectDefinition != effect.spriteEffectDefinition) {
                   spriteEffectDefinition = effect.spriteEffectDefinition;
                   timer = 0;
                   currentColor = &spriteEffectDefinition->getAdditiveColor0();
               }
               return *this;
           }
}
