//
// Created by fanick on 29.11.21.
//

#ifndef DUEL6R_VOICE_H
#define DUEL6R_VOICE_H

#include <memory>
namespace Duel6::net {
    class Voice : public Object<Voice, ObjectType::VOICE> {
    public:
        typedef Uint8 dataType;
        typedef std::unique_ptr<dataType[]> bufferType;
        Voice(bufferType & buffer):buffer(buffer){}

        // the chunk will be decoded through this player's decoder
        object_id_t playerId;
        bufferType & buffer;
        size_t len;

        template<class Stream>
        bool serialize(Stream &s) {
            bool result = s & playerId;
            if constexpr (s.isDeserializer()) {
                result &= s.readBuffer(buffer, len);
            } else {
                result &= s.writeBuffer(buffer, len);
            }
            return result;
        }
    };
}
#endif //DUEL6R_VOICE_H
