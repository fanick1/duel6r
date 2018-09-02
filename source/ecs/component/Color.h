#ifndef _ECS_COLOR_H
#define _ECS_COLOR_H

#include "../kktecs.h"

class ColorComponent : public KktComponent<ColorComponent>{
public:
   float r = 255;
   float g = 255;
   float b = 255;

};

#endif
