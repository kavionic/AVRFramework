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


#ifndef UTILS_H_
#define UTILS_H_


template<typename T> inline void swap(T& a, T& b) { T t = a; a = b; b = t; }
template<typename T, typename Y> inline T min(const T& a, const Y& b) { return (a>b) ? b : a; }
template<typename T, typename Y> inline T max(const T& a, const Y& b) { return (a>b) ? a : b; }

template<typename T> inline T clamp(const T& bottom, const T& top, const T& value) { return (value < bottom) ? bottom : ((value > top) ? top : value); }
/*{
    if ( value < bottom )
    {
        return bottom;
    } else if ( value > top ) {
        return top;
    } else {
        return value;
    }
}*/

template<typename T> inline T wrap(const T& bottom, const T& top, const T& value)
{
    if ( value < bottom )
    {
        return top - (bottom - value) + 1;
    } else if ( value > top ) {
        return bottom + (value - top) - 1;
    } else {
        return value;
    }        
}

inline char* ParseInt( const char* str, uint16_t& value, uint8_t base = 10 )
{
    value = 0;
    while(*str>='0' && *str<='9')
    {
        char c = *str++;
        uint8_t firstDigit;
        if (c >= '0' && c <= '9') {
             firstDigit = '0';
        } else if (c >= 'A' && c <= 'Z') {
            firstDigit = 'A';
        } else if (c >= 'a' && c <= 'a') {
            firstDigit = 'a';
        } else {
            break;
        }
        uint8_t digit = c - firstDigit;
        if (digit > base - 1) {
            break;
        }
        value = value * base + digit;
    }
    return const_cast<char*>(str);
}

inline char* ParseInt(const char* str, int16_t& value, uint8_t base = 10)
{
    char* newStr;
    if (*str == '-') {
        str++;
        uint16_t tmp;
        newStr = ParseInt(str, tmp, base);
        value = -tmp;
    } else {
        uint16_t tmp;
        newStr = ParseInt(str, tmp, base);
        value = tmp;
    }
    return newStr;
}

inline uint8_t PercentToByte(uint8_t value) { return (uint16_t(value) * 255 + 50) / 100;  }
inline uint8_t ByteToPercent(uint8_t value) { return (uint16_t(value) * 100 + 127) / 255; }

static const uint8_t PRINT_PERCENTAGE_BUFFER_SIZE = 4;
void PrintPercentage(char* buffer, int8_t value, bool print0 = false);
void PrintPercentage(int8_t value, bool print0 = false);

int32_t ParseIPAddress(const char* addressStr);

#define I8(value) static_cast<int8_t>(value)
#define U8(value) static_cast<uint8_t>(value)

#define I16(value) static_cast<int16_t>(value)
#define U16(value) static_cast<uint16_t>(value)

#define I32(value) static_cast<int32_t>(value)
#define U32(value) static_cast<uint32_t>(value)

#define I64(value) static_cast<int64_t>(value)
#define U64(value) static_cast<uint64_t>(value)

#define BIT(pos, value) ((value)<<(pos))
#define BIT8(pos, value)  U8((value)<<(pos))
#define BIT16(pos, value) U16((value)<<(pos))
#define BIT32(pos, value) U32((value)<<(pos))

#define DIV_ROUND(x,divider) (((x) + ((divider) >> 1)) / (divider))

#define ARRAY_COUNT(a) (sizeof(a) / sizeof(a[0]))

#ifndef PIN0_bp
#define PIN0_bp 0
#define PIN1_bp 1
#define PIN2_bp 2
#define PIN3_bp 3
#define PIN4_bp 4
#define PIN5_bp 5
#define PIN6_bp 6
#define PIN7_bp 7
#endif // PIN0_bp

#ifndef PIN0_bm
#define PIN0_bm BIT8(PIN0_bp, 1)
#define PIN1_bm BIT8(PIN1_bp, 1)
#define PIN2_bm BIT8(PIN2_bp, 1)
#define PIN3_bm BIT8(PIN3_bp, 1)
#define PIN4_bm BIT8(PIN4_bp, 1)
#define PIN5_bm BIT8(PIN5_bp, 1)
#define PIN6_bm BIT8(PIN6_bp, 1)
#define PIN7_bm BIT8(PIN7_bp, 1)
#endif // PIN0_bm

#endif /* UTILS_H_ */