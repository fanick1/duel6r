//
// Created by fanick on 25.11.21.
//

#ifndef DUEL6R_DECODER_H
#define DUEL6R_DECODER_H

#include <vector>
#include <opus/opus.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include "VoiceSettings.h"
namespace Duel6 {
    class Console;

    class Decoder {
    private:
        // this right here decodes every sound chunk that comes over the wire
        OpusDecoder *opusDecoder;
        Console& console;
        // and we store/buffer the decoded sound in here to be later picked for playing
        SDL_AudioStream *stream;
        static const int decodedBufferMaxLen = VOICE_FRAME_SIZE;  // this must be at least no. of samples returned from SDL_OpenAudioDevice
        static opus_int16 decodedBuffer[decodedBufferMaxLen];
    public:
        Decoder(Console &console);

        ~Decoder();

        Decoder(const Decoder &) = delete;

        Decoder(Decoder &&) = delete;

        Decoder &operator=(const Decoder &) = delete;

        Decoder &operator=(Decoder &&) = delete;

        [[nodiscard]] bool getAudio(Uint8 *buff, int len);

        void pushEncodedData(const Uint8 * buffer, size_t len);
    };
}

#endif //DUEL6R_DECODER_H
