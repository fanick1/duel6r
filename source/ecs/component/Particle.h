#ifndef _ECS_PARTICLE_H
#define _ECS_PARTICLE_H

#include "../kktecs.h"
class ParticleComponent : public KktComponent<ParticleComponent>{
public:
    typedef enum class Type {
        SPLASH,
        SNOW,
        RAIN
    };

    typedef struct Splash {

    };
    typedef struct Snow {

    };
    typedef struct Rain {
        float prevX;
        float prevY;
    };

    typedef union Data {
        Splash splash;
        Snow snow;
        Rain rain;
    };
public:
    float remainingLife = 5.0; //sec
    float size = 20.0;
    Type type = Type::SPLASH;
    Data data;
};
#endif