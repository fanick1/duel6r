#ifndef _ECS_ParticleSystem_H
#define _ECS_ParticleSystem_H

#include "../kktecs.h"

class ParticleSystem : public KktSystem<ParticleSystem>{
public:
   // static const float GRAVITATIONAL_ACCELERATION = -11.0f; //copied from Player.cpp
    void update(float timeDelta, KktFckr & fckr) override;

};
#endif