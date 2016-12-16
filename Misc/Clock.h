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


#ifndef CLOCK_H_
#define CLOCK_H_

#include <avr/io.h>

#include "Utils.h"

class Clock
{
public:
    static uint32_t GetTime();
    static uint32_t GetUpdateCycle() { return s_Updates; }
    static void IncrementTime() { s_Time++; }

#ifdef CLOCK_CPUCYCLES_PER_UPDATE
    static uint32_t GetHighresTimer()
    {
        for (;;)
        {
            uint16_t timer = SYSTEM_TIMER.CNT;
            uint32_t updates = s_Updates;
            if (SYSTEM_TIMER.CNT >= timer) // Check for overflow
            {
                return (updates << CLOCK_PERIOD_BIT_COUNT) | timer;
            }
        }            
    }        
    static void AddCycles()
    {
        if (U32(CLOCK_CPUCYCLES_PER_UPDATE) + s_Cycles > CPU_FREQ / 1000) {
            s_Time++;
            s_Cycles += CLOCK_CPUCYCLES_PER_UPDATE - CPU_FREQ / 1000;
        } else {
            s_Cycles += CLOCK_CPUCYCLES_PER_UPDATE;
        }
        s_Updates++;
    }
#endif // CLOCK_CPUCYCLES_PER_UPDATE

    static uint8_t GetHour();
    static uint8_t GetMinutes();
    static uint8_t GetSeconds();
    static uint8_t GetHundreds();

    static uint8_t GetHour(uint32_t time);
    static uint8_t GetMinutes(uint32_t time);
    static uint8_t GetSeconds(uint32_t time);
    static uint8_t GetHundreds(uint32_t time);
        
        
private:
    static volatile uint32_t s_Time;
#ifdef CLOCK_CPUCYCLES_PER_UPDATE    
    static volatile uint16_t s_Cycles;
    static volatile uint32_t s_Updates;
#endif
};


#endif /* CLOCK_H_ */