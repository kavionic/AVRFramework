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


#ifndef F_MISC_INPUTCAPTURE_H__
#define F_MISC_INPUTCAPTURE_H__

class InputCapture
{
public:
    static const uint16_t CAPTURE_OVERFLOW = 0xffff;
    
    static uint16_t GetCapture(bool risingEdge);
//private:    
    static volatile uint16_t s_TimerCapture;
    static volatile uint8_t  s_TimerCaptureSequence;
    static volatile bool     s_TimerOverflow;
    
};



#endif /* F_MISC_INPUTCAPTURE_H__ */