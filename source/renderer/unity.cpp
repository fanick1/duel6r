#ifdef D6_RENDERER_GL4
    #include "gl4/unity.cpp"
#elif D6_RENDERER_GL1
    #include "gl1/unity.cpp"
#elif D6_RENDERER_HEADLESS
    #include "headless/unity.cpp"
#endif

#include "RendererBase.cpp"
