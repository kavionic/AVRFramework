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


#ifndef __PSTRING_H__
#define __PSTRING_H__

#include <avr/pgmspace.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <alloca.h>
#include <inttypes.h>

//#define PGMU_OFFSET BOOT_SECTION_START
//#define PGMU_USE_FAR 1
//#define PGMU_USE_FAR_OFFSET 1


#if PGMU_USE_FAR
typedef uint_farptr_t PStringPtr_t;
#else
typedef const char* PStringPtr_t;
#endif

#if PGMU_USE_FAR_OFFSET
#define PGMU_ADD_OFFSET(s) (uint_farptr_t(s) + (PGMU_OFFSET))
#else
#define PGMU_ADD_OFFSET(s) (s)
#endif

#if PGMU_USE_FAR || PGMU_USE_FAR_OFFSET
class PString
{
public:
    PString(PStringPtr_t str, size_t size) : m_String(str), m_Size(size) {}
    template<size_t N>
#if PGMU_USE_FAR    
    PString(char const (&str)[N]) : m_String(pgm_get_far_address(str)), m_Size(N) {}
#else
    PString(char const (&str)[N]) : m_String(str), m_Size(N) {}
#endif
    const PStringPtr_t m_String;
    const size_t m_Size;
};

#define PMUTIL_LOAD_STR(in, out) uint_farptr_t formatPtr = PGMU_ADD_OFFSET(in.m_String); char* out = (char*)alloca(in.m_Size); strcpy_PF(out, formatPtr);

#else
class PString
{
public:
    PString(PStringPtr_t str) : m_String(str) {}
    const PStringPtr_t m_String;
};
#endif

#if PGMU_USE_FAR
#  define PSTRL(s)   (__extension__({static const char __c[sizeof(s)] PROGMEM = (s); PString(pgm_get_far_address(__c), sizeof(__c));}))
#  define PSTRPTR(s) PString(s)
#elif PGMU_USE_FAR_OFFSET
#  define PSTRL(s)   PString(PSTR(s), sizeof(s))
#  define PSTRPTR(s) PString(s)
#else
#  define PSTRL(s)   PString(PSTR(s))
#  define PSTRPTR(s) PString(s)
#endif


class PgmUtils
{
public:
#if PGMU_USE_FAR || PGMU_USE_FAR_OFFSET
    static int Printf(const PString& format, ...);

    static int FPrintf(const va_list& args, FILE* file, const PString& format);
    static int FPrintf(FILE* file, const PString& format, ...);

    template<typename... ArgTypes>
    static int Sscanf(const char* string, const PString& format, ArgTypes*... args)
    {
        PMUTIL_LOAD_STR(format, buffer);
        return sscanf(string, buffer, args...);
    }

    static int strcmp(const char* s1, const PString& s2) { return strcmp_PF(s1, PGMU_ADD_OFFSET(s2.m_String)); }
    static int strncmp(const char* s1, const PString& s2, size_t len) { return strncmp_PF(s1, PGMU_ADD_OFFSET(s2.m_String), len); }
        
    static char* strstr(const char* s1, const PString& s2) { return strstr_PF(s1, PGMU_ADD_OFFSET(s2.m_String)); }

    static uint8_t Read8(PStringPtr_t address) { return pgm_read_byte_far(PGMU_ADD_OFFSET(address)); }
    static uint8_t Read16(PStringPtr_t address) { return pgm_read_word_far(PGMU_ADD_OFFSET(address)); }
    static uint8_t Read32(PStringPtr_t address) { return pgm_read_dword_far(PGMU_ADD_OFFSET(address)); }
#else
    template<typename... ArgTypes>
    static int Printf(const PString& format, ArgTypes... args)
    {
        int result = printf_P(format.m_String, args...);
        return result;
    }
//    template<typename... ArgTypes>
    static int FPrintf(const va_list& args, FILE* file, const PString& format, ... /*ArgTypes... args*/)
    {
        return vfprintf_P(file, format.m_String, args);
    }
    static int FPrintf(FILE* file, const PString& format, ... /*ArgTypes... args*/)
    {
        va_list args;
        va_start(args, format);
        int result = vfprintf_P(file, format.m_String, args);
        va_end(args);
        return result;
    }
    template<typename... ArgTypes>
    static int Sscanf(const char* string, const PString& format, ArgTypes... args)
    {
        return sscanf_P(string, format.m_String, args...);
    }
    
    static int strcmp(const char* s1, const PString& s2) { return strcmp_P(s1, s2.m_String); }
    static int strncmp(const char* s1, const PString& s2, size_t len) { return strncmp_P(s1, s2.m_String, len); }

    static char* strstr(const char* s1, const PString& s2) { return strstr_P(s1, s2.m_String); }

    static uint8_t Read8(PStringPtr_t address) { return pgm_read_byte_near(address); }
    static uint8_t Read16(PStringPtr_t address) { return pgm_read_word_near(address); }
    static uint8_t Read32(PStringPtr_t address) { return pgm_read_dword_near(address); }

#endif
    template<typename... ArgTypes>
    static void LogMessage(ArgTypes... args) { Printf(args...); }
};

struct SFStop {};

template<typename PutCharPred>
struct StrFmt
{
    static void Print() { PutCharPred::Write('\r'); PutCharPred::Write('\n'); }
    static void Print(const SFStop&) {}

    static void SendValue(int8_t value)
    {
        SendValue(int16_t(value));
    }
    static void SendValue(uint8_t value)
    {
        SendValue(uint16_t(value));
    }

    static void SendValue(int16_t value)
    {
        char str[10];
        itoa(value, str, 10);
        SendValue(str);
    }
    static void SendValue(uint16_t value)
    {
        char str[10];
        utoa(value, str, 10);
        SendValue(str);
    }
    static void SendValue(int32_t value)
    {
        char str[16];
        ltoa(value, str, 10);
        SendValue(str);
    }
    static void SendValue(uint32_t value)
    {
        char str[16];
        ultoa(value, str, 10);
        SendValue(str);
    }
    static void SendValue(const char* str)
    {
        for(uint8_t i = 0; str[i] != '\0'; ++i)
        {
            PutCharPred::Write(str[i]);
        }
    }
    static void SendValue(const PString& str)
    {
        char c;
        for(uint8_t i = 0; (c = PgmUtils::Read8(str.m_String + i)) != '\0'; ++i)
        {
            PutCharPred::Write(c);
        }
    }

    template<typename Arg1Type, typename... ArgTypes>
    static void Print(Arg1Type arg1, ArgTypes... args)
    {
        SendValue(arg1);
        Print(args...);
    }
};

struct DebugLogWriter
{
    static void Write(char c);   
};

#define DP_SPAM     0
#define DP_INFO     1
#define DP_ERROR    2
#define DP_CRITICAL 3

#ifndef DEBUG_MUTE_SPAM
#    define DEBUG_LOG0(...) StrFmt<DebugLogWriter>::Print(__VA_ARGS__)
#else
#    define DEBUG_LOG0(...)
#endif

#ifndef DEBUG_MUTE_INFO
#    define DEBUG_LOG1(...) StrFmt<DebugLogWriter>::Print(__VA_ARGS__)
#else
#    define DEBUG_LOG1(...)
#endif

#ifndef DEBUG_MUTE_ERROR
#    define DEBUG_LOG2(...) StrFmt<DebugLogWriter>::Print(__VA_ARGS__)
#else
#    define DEBUG_LOG2(...)
#endif

#ifndef DEBUG_MUTE_CRITICAL
#    define DEBUG_LOG3(...) StrFmt<DebugLogWriter>::Print(__VA_ARGS__)
#else
#    define DEBUG_LOG3(...)
#endif

#define DB_LOG_CAT(PRI, ...) DEBUG_LOG##PRI(__VA_ARGS__)
#define DB_LOG(PRI, ...) DB_LOG_CAT(PRI, __VA_ARGS__)

#endif //__PSTRING_H__
