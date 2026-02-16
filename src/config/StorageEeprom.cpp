/*
 * Copyright (C) 2026 Dmitry Korobkov <dmitry.korobkov.nn@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <Arduino.h>
#include <EEPROM.h>
#include <console.h>

#include "StorageEeprom.h"

using namespace console;
using namespace config;

StorageEeprom::StorageEeprom(unsigned short size)
    : mSize(size) {

    EEPROM.begin(size);
};

unsigned short StorageEeprom::size() {
    return mSize;
}

const char StorageEeprom::read(unsigned short index) {
    return EEPROM.read(index);
}

void StorageEeprom::write(unsigned short index, const char value) {
    if (EEPROM.read(index) != value) {
        EEPROM.write(index, value);
    }
}

void StorageEeprom::commit() {
    EEPROM.commit();
}
