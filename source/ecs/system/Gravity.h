#ifndef _ECS_GRAVITY_H
#define _ECS_GRAVITY_H
#include "../kktecs.h"
class GravitySystem : public KktSystem<GravitySystem>{
public:
    constexpr static const float GRAVITATIONAL_ACCELERATION = -11.0f; //copied from Player.cpp
    void update(float timeDelta, KktFckr & fckr) override;
};

#endif