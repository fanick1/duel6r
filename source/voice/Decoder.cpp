//
// Created by fanick on 25.11.21.
//

#include "Decoder.h"
#include "Recorder.h"
namespace Duel6{
    opus_int16 Decoder::decodedBuffer[decodedBufferMaxLen];

    Decoder::Decoder() {
        // AUDIO_S16LSB is used throughout the audio code because it is consumed by opus
        const SDL_AudioFormat src_format = AUDIO_S16LSB;
        const Uint8 src_channels = 1;
        const int src_rate = VOICE_FREQ;
        const SDL_AudioFormat dst_format = AUDIO_S16LSB;
        const Uint8 dst_channels = 1;
        const int dst_rate = 48000; // TODO must match freq of Sound::auxdevice
        int error;
        opusDecoder = opus_decoder_create(src_rate, src_channels, &error);
        if (error != OPUS_OK) {
            printf("Failed to initialize Opus decoder %d\n", error);
        }
        //todo take input params
        stream = SDL_NewAudioStream(src_format, src_channels, src_rate, dst_format, dst_channels, dst_rate);
    }

    Decoder::~Decoder() {
        opus_decoder_destroy(opusDecoder);
        SDL_FreeAudioStream(stream);
    }

    bool Decoder::getAudio(Uint8 *buff, int len) {
        int avail = SDL_AudioStreamAvailable(stream);
        if(avail < len) {
//            printf("avail < len, skipping\n");
            return false;
        }
        SDL_AudioStreamGet(stream, buff, len);
//        while (avail >= len) {
//            SDL_AudioStreamGet(stream, buff, len);
////            if (avail > len * 3) { //lag
////                printf("Stream Cleaned\n");
////                SDL_AudioStreamClear(stream);
////            }
//            avail = SDL_AudioStreamAvailable(stream);
//        }
        return true;
    }

    void Decoder::pushEncodedData(const Uint8 * buffer, size_t len) {
        auto decoded = opus_decode(opusDecoder, buffer, len, decodedBuffer, decodedBufferMaxLen, 0);
        if (decoded < 0) {
            printf("opuse_decode error %i\n", decoded);
            return;
        }
        SDL_AudioStreamPut(stream, decodedBuffer, decoded * (sizeof(opus_int16) / sizeof(Uint8)));
    }
}