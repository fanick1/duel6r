
#include "../kktecs.h"
#include "Gravity.h"
#include "../component/Position.h"

void GravitySystem::update(float timeDelta, KktFckr & fckr) {
    for (auto entity : fckr.getEntitiesWithComponents<PositionComponent>()) {
        auto [p] = fckr.get<PositionComponent>(entity);
        p->accY = GRAVITATIONAL_ACCELERATION * timeDelta;

//
//        if (p->accX > 0) {
//            p->accX -= l * timeDelta; // e.g. friction
//            if (p->accX < 0.0001) {
//                p->accX = 0;
//            }
//        }
//        if (p->accX < -0) {
//            p->accX += l * timeDelta; // e.g. friction
//            if (p->accX > -0.001) {
//                p->accX = 0;
//            }
//        }

        p->velocityY += p->accY;
        p->velocityX += p->accX;
        constexpr float l = 4;
        constexpr float l2 = 2;
        if (p->velocityX < 0.0f) {
            p->velocityX = std::min(p->velocityX + l2 * timeDelta, 0.0f);
        }
        if (p->velocityX > 0.0f) {
            p->velocityX = std::max(p->velocityX - l2 * timeDelta, 0.0f);
        }
//        if (p->velocityY <= -l) {
//            p->velocityY = -l;
//        }
//        if (p->velocityY >= l) {
//            p->velocityY = l;
//        }
//        if (p->velocityX <= -l) {
//            p->velocityX = -l;
//        }
//        if (p->velocityX >= l) {
//            p->velocityX = l;
//        }

        p->y += p->velocityY * timeDelta;
        p->x += p->velocityX * timeDelta;
        p->accX = 0;
    }
}

