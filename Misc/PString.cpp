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


#include "PString.h"

#if PGMU_USE_FAR || PGMU_USE_FAR_OFFSET

int PgmUtils::Printf( const PString& format, ... /*ArgTypes... args*/ )
{
    return 0;
    va_list args;
    va_start(args, format);
    PMUTIL_LOAD_STR(format, buffer);
    int result = vprintf(buffer, args);
    va_end(args);
    return result;
}

int PgmUtils::FPrintf(const va_list& args, FILE* file, const PString& format)
{
    PMUTIL_LOAD_STR(format, buffer);
    int result = vfprintf(file, buffer, args);
    return result;
}

int PgmUtils::FPrintf(FILE* file, const PString& format, ...)
{
//        PMUTIL_LOAD_STR(format, buffer);
    va_list args;
    va_start(args, format);
    PMUTIL_LOAD_STR(format, buffer);
    int result = vfprintf(file, buffer, args);
//    int result = FPrintf(args, file, format, args);
    va_end(args);
    return result;
}

#else
#endif
