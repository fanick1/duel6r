
#include "../kktecs.h"
#include "../component/Position.h"
#include "../component/Particle.h"
#include "Particle.h"
#include <iostream>

    void ParticleSystem::update(float timeDelta, KktFckr & fckr) {
        PositionComponent * pos;
        ParticleComponent * particle;
        for( auto & entity : fckr.getEntitiesWithComponents<PositionComponent, ParticleComponent>()){


// also works:     fckr.get(entity, p, c);
// also works:     auto [p1, c1] = fckr.get(entity, PositionComponent::T(), ColorComponent::T());
// also works:     auto [p,c] = fckr.get<PositionComponent, ColorComponent>(entity);
           fckr.get(*entity, &pos, &particle);
           //p->y -= GRAVITATIONAL_ACCELERATION * timeDelta;
            //particle->size -= timeDelta * 0.01;
            particle->remainingLife -= timeDelta;
            if(particle->remainingLife <= 0) {
                entity->destroy();
               // break;
            }
//            switch(particle->type){
//            case ParticleComponent::Type::RAIN:{
//                auto & data = particle->data.rain;
////                data.prevX = pos->x;
////                data.prevY = pos->y;
//
//                break;
//            }
//            case ParticleComponent::Type::SPLASH:{
//                pos->velocityX *= 0.99;
//                break;
//            }
//            case ParticleComponent::Type::SNOW:{
//                break;
//            }
//
//            }

//           pos->x += particle->x * timeDelta;
//           pos->y += particle->y * timeDelta;
        }

    }
