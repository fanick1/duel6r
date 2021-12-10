/*
 * BinarySerializer.h
 *
 *  Created on: Jan 11, 2017
 *      Author: fanick
 */
#ifndef BINARYSTREAMS_BINARYSERIALIZER_H
#define BINARYSTREAMS_BINARYSERIALIZER_H

#include <memory>
#include <stdint.h>

template <typename binarystream>
class BinarySerializer {

private:
    binarystream &stream;

public:
    BinarySerializer(binarystream &s)
        : stream(s) {
    }
    //TODO Constructors
    template<typename M>
    bool operator &(const M &m) {
        return stream << m;
    }

    template<typename M>
    bool operator &(M &m) {
        return stream << m;
    }

    bool writeBuffer(const std::unique_ptr<uint8_t[]> & buffer, size_t & len) {
        return stream.writeBuffer(buffer, len);
    }

    constexpr static bool isSerializer() {
        return true;
    }
    constexpr static bool isDeserializer() {
        return false;
    }
};

#endif
