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

#include <util/atomic.h>

#include "Clock.h"

volatile uint32_t Clock::s_Time;

#ifdef CLOCK_CPUCYCLES_PER_UPDATE
volatile uint16_t Clock::s_Cycles;
volatile uint32_t Clock::s_Updates;
#endif


uint32_t Clock::GetTime()
{
    uint32_t time;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        time = s_Time;
    }
    return time;
}

uint8_t Clock::GetHour()
{
    return GetTime() / 1000 / 60 / 60;
}

uint8_t Clock::GetMinutes()
{
    return GetTime() / 1000 / 60 % 60;
}

uint8_t Clock::GetSeconds()
{
    return GetTime() / 1000 % 60;
}

uint8_t Clock::GetHundreds()
{
    return GetTime() / 10 % 100;
}

uint8_t Clock::GetHour( uint32_t time )
{
    return time / 1000 / 60 / 60;
}

uint8_t Clock::GetMinutes( uint32_t time )
{
	return time / 1000 / 60 % 60;
}

uint8_t Clock::GetSeconds( uint32_t time)
{
	return time / 1000 % 60;
}

uint8_t Clock::GetHundreds( uint32_t time)
{
	return time / 10 % 100;
}
