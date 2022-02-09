#include "animation.h"

namespace animation {

    void Image::substitute(const Palette& palette, std::vector<Color> & result, uint8_t opacity, uint8_t transparentIndex) const {
        size_t size = pixels.size();
        result.resize(size);
        if (size == 0) {
            return;
        }
        const float o = opacity / 255.0f;
        size_t lastPixel = pixels[0];
        if (lastPixel == transparentIndex) {
            *(uint32_t *) (&result[0]) = 0;
        } else {
            result[0] = palette.colors[lastPixel];
            result[0].a *= o;
        }
        Color *lastColor = &result[0];
        for (size_t i = 1; i < size; i++) {
            if (pixels[i] == transparentIndex) {
                *(uint32_t *) (&result[i]) = 0;
                continue;
            }
            if (pixels[i] == lastPixel) {
                result[i] = *lastColor;
            } else { [[unlikely]]
                lastPixel = pixels[i];
                result[i] = palette.colors[lastPixel];
                result[i].a *= o;
                lastColor = &result[i];

            }
        }
    }
}