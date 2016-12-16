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

#include "InputCapture.h"

#include <avr/sleep.h>

volatile uint16_t InputCapture::s_TimerCapture;
volatile uint8_t  InputCapture::s_TimerCaptureSequence = 0;
volatile bool     InputCapture::s_TimerOverflow = false;


#define INPUTCAPTURE_CAPTURE_REGISTER ICR1
#define INPUTCAPTURE_COUNTER_REGISTER TCNT1
#define INPUTCAPTURE_SET_RISING() do  { TCCR1B |= 1<<ICES1; } while(0)
#define INPUTCAPTURE_SET_FALLING() do { TCCR1B &= ~(1<<ICES1); } while(0)
#define INPUTCAPTURE_OVERFLOW_ISR TIMER1_OVF_vect
#define INPUTCAPTURE_CAPTURE_ISR  TIMER1_CAPT_vect


ISR(INPUTCAPTURE_OVERFLOW_ISR)
{
    InputCapture::s_TimerOverflow = true;
}


ISR(INPUTCAPTURE_CAPTURE_ISR)
{
    InputCapture::s_TimerCapture=INPUTCAPTURE_CAPTURE_REGISTER;
    INPUTCAPTURE_COUNTER_REGISTER = INPUTCAPTURE_COUNTER_REGISTER - InputCapture::s_TimerCapture;
    InputCapture::s_TimerCaptureSequence++;
}

uint16_t InputCapture::GetCapture(bool risingEdge)
{
    if ( risingEdge ) {
        INPUTCAPTURE_SET_RISING();
    } else {
        INPUTCAPTURE_SET_FALLING();
    }
    InputCapture::s_TimerOverflow = false;
    
    uint8_t curSequence = InputCapture::s_TimerCaptureSequence;
    
    while(curSequence == InputCapture::s_TimerCaptureSequence && !InputCapture::s_TimerOverflow) {
        wdt_reset();
        cli();
        if ( curSequence == InputCapture::s_TimerCaptureSequence && !InputCapture::s_TimerOverflow )
        {
            sleep_enable();
            sei();
            sleep_cpu();
            sleep_disable();
        }
    }         
    return (!InputCapture::s_TimerOverflow) ? InputCapture::s_TimerCapture : CAPTURE_OVERFLOW;
}
