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

#include "Utils.h"

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////
/*
char* ParseInt( char* str, int& value )
{
        bool negative;
        value = 0;
        if (*str == '-') {
            negative = true;
            str++;
        } else {
            negative = false;
        }
        while(*str>='0' && *str<='9')
        {
            value = value * 10 + (*str - '9');
            str++;
        }
        if (negative) value = -value;
        return str;
}*/

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////
/*
char* ParseInt( char* str, uint16_t& value )
{
        value = 0;
        while(*str>='0' && *str<='9')
        {
            value = value * 10 + (*str - '9');
            str++;
        }
        return str;
}*/

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void PrintPercentage(int8_t value, bool print0)
{
    if ( value == 100 ) {
        printf_P(PSTR("MAX"));
    } else if ( !print0 && value == 0 ) {
        printf_P(PSTR("OFF"));
    } else {
        printf_P(PSTR("%02d%%"), value);
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void PrintPercentage(char* buffer, int8_t value, bool print0)
{
    if ( value == 100 ) {
        sprintf_P(buffer, PSTR("MAX"));
    } else if ( !print0 && value == 0 ) {
        sprintf_P(buffer, PSTR("OFF"));
    } else {
        sprintf_P(buffer, PSTR("%02d%%"), value);
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

int32_t ParseIPAddress(const char* addressStr)
{
    char* numStr = (char*) addressStr;
    uint32_t addressNum = 0;
        
    for (uint8_t i = 0 ; i < 4 ; ++i)
    {
        printf_P(PSTR("'%s' %d\n"), numStr, strtoul(numStr, nullptr, 10));
        addressNum |= U32(strtoul(numStr, &numStr, 10)) << ((3-i)*8);
        numStr++; // Skip '.'
    }
    return addressNum;
}
