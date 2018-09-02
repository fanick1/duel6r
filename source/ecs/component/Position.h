#ifndef _ECS_POSITION_H
#define _ECS_POSITION_H

#include "../kktecs.h"
class PositionComponent : public KktComponent<PositionComponent>{
public:
    float x;
    float y;
    float velocityX;
    float velocityY;
    float accX;
    float accY;

};
#endif