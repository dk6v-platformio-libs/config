/*
 * Copyright (C) 2026 Dmitry Korobkov <dmitry.korobkov.nn@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <vector>
#include <console.h>
#include <checksum.h>

#include "ConfigParameter.h"

using namespace console;
using namespace checksum;
using namespace config;

// Class ConfigParameterBase

ConfigParameterBase::ConfigParameterBase(ConfigParameterType type, unsigned char id, bool isValid = false)
    : mType(type), mId(id), mIsValid(isValid) {}

unsigned char ConfigParameterBase::getId() {

    return this->mId;
}

ConfigParameterType ConfigParameterBase::getType() {

    return this->mType;
}

bool ConfigParameterBase::isValid() {

    return mIsValid;
}

ByteBuffer::iterator ConfigParameterBase::read(ByteBuffer::iterator &it) {

    ByteBuffer::iterator nextIt = it;

    char id = *nextIt++;
    char type = *nextIt++;

    if (id != mId) {
        LOG("Skip parameter: id=%d (!= %d)", mId, id);
        return it;
    }

    if (type != static_cast<char>(mType)) {
        LOG("Skip parameter: id=%d, type=%d (!= %d)", mId, mType, type);
        return it;
    }

    return nextIt;
}

ByteBuffer::iterator ConfigParameterBase::write(ByteBuffer::iterator &it) {

    ByteBuffer::iterator nextIt = it;

    /* LOG("write: id=0x%x, type=0x%x", mId, mType); */

    *nextIt++ = static_cast<char>(mId);
    *nextIt++ = static_cast<char>(mType);

    return nextIt;
}

// Class ConfigParameter<char>

template <>
ConfigParameter<char>::ConfigParameter(unsigned char id)
    : ConfigParameterBase(ConfigParameterType::BYTE, id), mValue(0), mIsValid(false) {};

template <>
ConfigParameter<char>::ConfigParameter(unsigned char id, const char &value)
    : ConfigParameterBase(ConfigParameterType::BYTE, id), mValue(value), mIsValid(true) {};

template <>
ByteBuffer::iterator ConfigParameter<char>::read(ByteBuffer::iterator &it) {

    auto nextIt = ConfigParameterBase::read(it);

    if (nextIt != it) {

        mValue = *nextIt++;

        char checksum = Checksum(Checksum::CRC8)
            .calculate(reinterpret_cast<char *>(&mValue), sizeof(char));

        if (checksum != *nextIt++) {

            LOG("Invalid checksum (0x%X): id=%d, type=%d",
                        checksum, mId, mType);
            return it;
        }

        LOG("CFG <= R [%02d]: %d, CS: 0x%X", mId, mValue, checksum);
    }

    return nextIt;
}

template <>
ByteBuffer::iterator ConfigParameter<char>::write(ByteBuffer::iterator &it) {

    auto nextIt = ConfigParameterBase::write(it);

    char checksum = Checksum(Checksum::CRC8)
        .calculate(reinterpret_cast<char *>(&mValue), sizeof(char));

    *nextIt++ = mValue;
    *nextIt++ = checksum;

    LOG("CFG => W [%02d]: %d, CS: 0x%X", mId, mValue, checksum);

    return nextIt;
}

// Class ConfigParameter<int>

template <>
ConfigParameter<int>::ConfigParameter(unsigned char id)
    : ConfigParameterBase(ConfigParameterType::NUMBER, id), mValue(0), mIsValid(false) {};

template <>
ConfigParameter<int>::ConfigParameter(unsigned char id, const int &value)
    : ConfigParameterBase(ConfigParameterType::NUMBER, id), mValue(value), mIsValid(true) {};

template <>
ByteBuffer::iterator ConfigParameter<int>::read(ByteBuffer::iterator &it) {

    auto nextIt = ConfigParameterBase::read(it);

    if (nextIt != it) {

        mValue = 0;

        for (unsigned char ix = 0; ix < sizeof(int); ++ix) {

            mValue |= BYTE_SET(ix, 0x00, *nextIt++);
        }

        char checksum = Checksum(Checksum::CRC8)
            .calculate(reinterpret_cast<char *>(&mValue),sizeof(int));

        if (checksum != *nextIt++) {

            LOG("Invalid checksum (0x%X): id=%d, type=%d",
                        checksum, mId, mType);
            return it;
        }

        LOG("CFG <= R [%02d]: %d, CS: 0x%X", mId, mValue, checksum);
    }

    return nextIt;
}

template <>
ByteBuffer::iterator ConfigParameter<int>::write(ByteBuffer::iterator &it) {

    auto nextIt = ConfigParameterBase::write(it);

    char checksum = Checksum(Checksum::CRC8)
        .calculate(reinterpret_cast<char*>(&mValue), sizeof(int));

    for (unsigned char ix = 0; ix < sizeof(int); ++ix) {

        *nextIt++ = NBYTE(ix, mValue);
    }

    *nextIt++ = checksum;

    LOG("CFG => W [%02d]: %d, CS: 0x%X", mId, mValue, checksum);

    return nextIt;
}

// Class ConfigParameter<string>

template <>
ConfigParameter<std::string>::ConfigParameter(unsigned char id)
    : ConfigParameterBase(ConfigParameterType::STRING, id), mValue(""), mIsValid(false) {};

template <>
ConfigParameter<std::string>::ConfigParameter(unsigned char id,
                                              const std::string &value)
    : ConfigParameterBase(ConfigParameterType::STRING, id), mValue(value), mIsValid(true) {};

template <>
ByteBuffer::iterator
ConfigParameter<std::string>::read(ByteBuffer::iterator &it) {

    auto nextIt = ConfigParameterBase::read(it);

    if (nextIt != it) {

        char length = *nextIt++;

        mValue.clear();

        for (unsigned char ix = 0; ix < length; ++ix) {

            mValue += *nextIt++;
        }

        char checksum = Checksum(Checksum::CRC8)
            .calculate(mValue.c_str(), mValue.length());

        if (checksum != *nextIt++) {

            LOG("Invalid checksum (0x%X): id=%d, type=%d",
                        checksum, mId, mType);
            return it;
        }

        LOG("CFG <= R [%02d]: '%s', CS: 0x%X", mId, mValue.c_str(), checksum);
    }

    return nextIt;
}

template <>
ByteBuffer::iterator
ConfigParameter<std::string>::write(ByteBuffer::iterator &it) {

    auto nextIt = ConfigParameterBase::write(it);

    char checksum = Checksum(Checksum::CRC8)
        .calculate(mValue.c_str(), mValue.length());

    *nextIt++ = mValue.length();

    for (unsigned char ix = 0; ix < mValue.length(); ++ix) {

        *nextIt++ = mValue[ix];
    }

    *nextIt++ = checksum;

    LOG("CFG => W [%02d]: '%s', CS: 0x%X", mId, mValue.c_str(), checksum);

    return nextIt;
}

// Class ConfigParameter<IPAddress>

template <>
ConfigParameter<IPAddress>::ConfigParameter(unsigned char id)
    : ConfigParameterBase(ConfigParameterType::IP_ADDRESS, id), mValue(), mIsValid(false) {};

template <>
ConfigParameter<IPAddress>::ConfigParameter(unsigned char id,
                                            const IPAddress &value)
    : ConfigParameterBase(ConfigParameterType::IP_ADDRESS, id), mValue(value), mIsValid(true) {};

template <>
ByteBuffer::iterator
ConfigParameter<IPAddress>::read(ByteBuffer::iterator &it) {

    auto nextIt = ConfigParameterBase::read(it);

    if (nextIt != it) {

        unsigned int address = 0;

        for (unsigned char ix = 0; ix < sizeof(int); ++ix) {

            address |= BYTE_SET(ix, 0x00, *nextIt++);
        }

        char checksum = Checksum(Checksum::CRC8)
            .calculate(reinterpret_cast<char*>(&address), sizeof(int));

        if (checksum != static_cast<char>(*nextIt++)) {

            LOG("Invalid checksum (0x%X): id=%d, type=%d",
                        checksum, mId, mType);
            return it;
        }

        mValue = address;

        LOG("CFG <= R [%02d]: %s, CS: 0x%X",
                    mId, IPAddress(address).toString().c_str(), checksum);
    }

    return nextIt;
}

template <>
ByteBuffer::iterator
ConfigParameter<IPAddress>::write(ByteBuffer::iterator &it) {

    auto nextIt = ConfigParameterBase::write(it);
    unsigned int address = static_cast<unsigned int>(mValue);

    for (unsigned char ix = 0; ix < sizeof(int); ++ix) {

        *nextIt++ = NBYTE(ix, address);
    }

    char checksum = Checksum(Checksum::CRC8)
        .calculate(reinterpret_cast<char*>(&address), sizeof(int));

    *nextIt++ = checksum;
    LOG("CFG => W [%02d]: %s, CS: 0x%X",
                mId, mValue.toString().c_str(), checksum);

    return nextIt;
}
