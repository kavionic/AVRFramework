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

#include <avr/wdt.h>

#include "SpinTimer.h"

#ifndef SPINTIMER_TIMERL
#error "SPINTIMER_TIMERL not defined. It should name an 8-bit counter register to be used for timing."
#endif
#ifndef SPINTIMER_FREQ
#error "SPINTIMER_FREQ not defined. It should be defined to the clock frequency of SPINTIMER_TIMERL."
#endif
#define SPINTIMER_USEC (SPINTIMER_FREQ / 1000000)

uint8 SpinTimer::GetCurrentTime()
{
    return SPINTIMER_TIMERL;
}

void SpinTimer::Delay( uint8 i_nStartTime, uint8 i_nDelay )
{
    uint8 nTime;
    
    for(;;)
    {
        nTime = SPINTIMER_TIMERL - i_nStartTime;
        if ( nTime >= i_nDelay ) {
            break;
        }
    }
}

uint8 SpinTimer::SleepuS( uint16 delay )
{
    uint8 curTime = SPINTIMER_TIMERL;
    while( delay >= 60000 / SPINTIMER_USEC )
    {
        curTime = Delay16(curTime, 60000);
        delay -= 60000 / SPINTIMER_USEC;
    }
    if ( delay > 0 )
    {
        curTime = Delay16(curTime, delay * SPINTIMER_USEC);
    }
    return curTime;
}

uint8 SpinTimer::SleepMS( uint16 delay )
{
    uint8 curTime = SPINTIMER_TIMERL;
    while(delay--)
    {
        curTime = Delay16(curTime, 1000 * SPINTIMER_USEC);
    }
    return curTime;
}

uint8 SpinTimer::Delay16( uint8 i_nStartTime, uint16 i_nDelay )
{
    for (;;)
    {
        wdt_reset();
        if ( i_nDelay > 64 )
        {
            Delay( i_nStartTime, 64 );
            i_nStartTime += 64;
            i_nDelay -= 64;
        } else {
            uint8 nRemaining = (uint8)i_nDelay;
            Delay( i_nStartTime, nRemaining );
            i_nStartTime += nRemaining;
            break;
        }
    }
    return i_nStartTime;
}
