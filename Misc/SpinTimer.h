// This file is part of Kurts AVR framework library.
//
// Copyright (C) 2016 Kurt Skauen <http://kavionic.com/>
//
// TeslaDriver is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// TeslaDriver is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with TeslaDriver. If not, see < http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////////


#ifndef F_TIMER_H__
#define F_TIMER_H__

#include <avr/io.h>

class SpinTimer
{
public:
    static void Initialize();
    
    static uint8_t GetCurrentTime();
    static void  Delay( uint8_t i_nStartTime, uint8_t i_nDelay );
    static uint8_t Delay16( uint8_t i_nStartTime, uint16_t i_nDelay );
    
    static uint8_t SleepuS(uint16_t delay);
    
    static uint8_t SleepMS(uint16_t delay);
};

#endif // F_TIMER_H__
