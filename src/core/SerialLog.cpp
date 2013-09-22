/*
    cheali-charger - open source firmware for a variety of LiPo chargers
    Copyright (C) 2013  Paweł Stawicki. All right reserved.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <Arduino.h>

#include "SerialLog.h"
#include "Hardware.h"
#include "Timer.h"
#include "LcdPrint.h"
#include "Program.h"
#include "Settings.h"
#include "memory.h"

namespace SerialLog {
    enum State { On, Off, Starting };
    uint32_t startTime;
    uint32_t currentTime;

    State state = Off;
    uint8_t CRC;
    const AnalogInputs::Name channel1[] PROGMEM = {
            AnalogInputs::VoutBalancer,
            AnalogInputs::Iout,
            AnalogInputs::Cout,
            AnalogInputs::Pout,
            AnalogInputs::Eout,
            AnalogInputs::Textern,
            AnalogInputs::Tintern,
            AnalogInputs::Vin,
            AnalogInputs::Vb1,
            AnalogInputs::Vb2,
            AnalogInputs::Vb3,
            AnalogInputs::Vb4,
            AnalogInputs::Vb5,
            AnalogInputs::Vb6,
    };



void sendTime();

void powerOn()
{

#ifdef ENABLE_SERIAL_LOG
    if(state != Off)
        return;
    if(settings.UART_ == Settings::Disabled)
        return;

#ifndef ENABLE_SERIAL_LOG_WAIT
    Serial.begin(SERIAL_SPEED);
#endif

    state = Starting;
#endif //ENABLE_SERIAL_LOG
}

void powerOff()
{
#ifdef ENABLE_SERIAL_LOG

    if(state == Off)
        return;

#ifndef ENABLE_SERIAL_LOG_WAIT
    Serial.end();
#endif

    state = Off;
#endif //ENABLE_SERIAL_LOG
}

void send()
{
#ifdef ENABLE_SERIAL_LOG

    if(state == Off)
        return;

#ifdef ENABLE_SERIAL_LOG_WAIT
    Serial.begin(SERIAL_SPEED);
#endif

    currentTime = Timer::getMiliseconds();

    if(state == Starting) {
        startTime = currentTime;
        state = On;
    }

    currentTime -= startTime;
    sendTime();

#ifdef ENABLE_SERIAL_LOG_WAIT
    Serial.flush();
    Serial.end();
    pinMode(10, INPUT);
#endif

#endif //ENABLE_SERIAL_LOG
}


void printChar(char c)
{
    Serial.print(c);
    CRC^=c;
}
void printD()
{
    printChar(';');
}

void printString(char *s)
{
    while(*s) {
        printChar(*s);
        s++;
    }
}

void printUInt(uint16_t x)
{
    char buf[8];
    char *str = buf;
    uint8_t maxSize = 7;
    ::printUInt(str, maxSize, x);
    printString(buf);
}



void sendHeader(uint16_t channel)
{
    CRC = 0;
    printChar('$');
    printUInt(channel);
    printD();
    printUInt(Program::programState_);   //state
    printD();

    printUInt(currentTime/1000);   //timestamp
    printChar('.');
    printUInt((currentTime/100)%10);   //timestamp
    printD();
}

void sendEnd()
{
    //checksum
    printUInt(CRC);
    printChar('\r');
    printChar('\n');
}

void sendChannel1()
{
    sendHeader(1);
    //analog inputs
    for(int8_t i=0;i < sizeOfArray(channel1);i++) {
        printUInt(AnalogInputs::getRealValue(pgm::read(&channel1[i])));
        printD();
    }
    sendEnd();
}

void sendChannel2()
{
    sendHeader(2);
    FOR_ALL_INPUTS(it) {
        printUInt(AnalogInputs::getRealValue(it));
        printD();
    }
    printUInt(SMPS::getValue());
    printD();
    printUInt(Discharger::getValue());
    printD();
    printUInt(balancer.balance_);
    printD();

    printUInt(Screen::calculateBattRth());
    printD();

    printUInt(Screen::calculateWiresRth());
    printD();

    for(int8_t i=0;i<MAX_BANANCE_CELLS;i++) {
        printUInt(Screen::calculateRthCell(i));
        printD();
    }

    sendEnd();
}

void sendChannel3()
{
    sendHeader(3);
    FOR_ALL_PHY_INPUTS(it) {
        printUInt(AnalogInputs::getValue(it));
        printD();
    }
    sendEnd();
}


void sendTime()
{
    sendChannel1();
    if(settings.UART_>1)
        sendChannel2();

    if(settings.UART_>2)
        sendChannel3();

}


} //namespace SerialLog