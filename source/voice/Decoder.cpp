//
// Created by fanick on 25.11.21.
//

#include "Decoder.h"
#include "../console/Console.h"

namespace Duel6{
    opus_int16 Decoder::decodedBuffer[decodedBufferMaxLen];

    Decoder::Decoder(Console &console):console(console) {
        // AUDIO_S16LSB is used throughout the audio code because it is consumed by opus

        SDL_AudioSpec audioSpec;
        //REAL WTF RIGHT HERE. YOU HAVE TO PASS THE VALUE FROM THIS STRUCT into SDL_NewAudioStream
        audioSpec.format = AUDIO_S16LSB; // DO NOT MODIFY !
        const Uint8 src_channels = 1;
        const int src_rate = VOICE_FREQ;
        const Uint8 dst_channels = 1;
        const int dst_rate = 48000; // TODO must match freq of Sound::auxdevice
        int error;
        opusDecoder = opus_decoder_create(src_rate, 1, &error);
        if (error != OPUS_OK) {
            console.printLine(Format("Failed to initialize Opus decoder {0}") << error);
            printf("Failed to initialize Opus decoder %d\n", error);
        }
        //todo take input params
        //WARNING: the audioSpec.format cannot be replaced by passing the format value by value (AUDIO_S16LSB) when compiling for macOS (SDL reports the format as invalid in runtime)
        stream = SDL_NewAudioStream(audioSpec.format, src_channels, src_rate, audioSpec.format, dst_channels, dst_rate);
        if (!stream) {
            const char *lastError = SDL_GetError();
            printf("Decoder::Decoder SDL_NewAudioStream failed %s\n", lastError);
            console.printLine(Format("Decoder::Decoder SDL_NewAudioStream failed {0}") << lastError);
        }
    }

    Decoder::~Decoder() {
        opus_decoder_destroy(opusDecoder);
        SDL_FreeAudioStream(stream);
    }

    bool Decoder::getAudio(Uint8 *buff, int len) {
        int avail = SDL_AudioStreamAvailable(stream);
        if(avail < len) {
            return false;
        }
        int res = SDL_AudioStreamGet(stream, buff, len);
        if(res == -1){
            auto lastError = SDL_GetError();
            printf("Decoder::getAudio SDL_AudioStreamGet error %s\n", lastError);
            console.printLine(Format("Decoder::getAudio SDL_AudioStreamGet error {0}") << lastError); 
        }
        return true;
    }

    void Decoder::pushEncodedData(const Uint8 * buffer, size_t len) {
        auto decoded = opus_decode(opusDecoder, buffer, len, decodedBuffer, decodedBufferMaxLen, 0);
        if (decoded < 0) {
            printf("opuse_decode error %i\n", decoded);
            console.printLine(Format("opuse_decode error {0}") << decoded);
            return;
        }
        int res = SDL_AudioStreamPut(stream, decodedBuffer, decoded * (sizeof(opus_int16) / sizeof(Uint8)));
        if(res == -1){
            auto lastError = SDL_GetError();
            printf("Decoder::pushEncodedData SDL_AudioStreamPut error %s\n", lastError);
            console.printLine(Format("Decoder::pushEncodedData SDL_AudioStreamPut error {0}") << lastError); 
        }
    }
}