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

#include "Display.h"
#include "DigitalPort.h"
#include "SpinTimer.h"

#include <avr/io.h>
#include <avr/wdt.h>
#include <string.h>


//DigitalPort<e_DigitalPortID_F> g_DisplayPort;
/*static DigitalPort g_DisplayDataPort(e_DigitalPortID_F);
static DigitalPort g_DisplayCtrlPort(e_DigitalPortID_F);

#define DISPLAY_REGISTER_SELECT_PORT DISPLAY_CTRL_PORT
#define DISPLAY_RW_PORT              DISPLAY_CTRL_PORT
#define DISPLAY_ENABLE_PORT          DISPLAY_CTRL_PORT

#define DISPLAY_REGISTER_SELECT PIN4_bm
#define DISPLAY_RW              PIN5_bm
#define DISPLAY_ENABLE          PIN6_bm


*/

#define DISPLAY_DATA_0          PIN0_bm
#define DISPLAY_DATA_1          PIN1_bm
#define DISPLAY_DATA_2          PIN2_bm
#define DISPLAY_DATA_3          PIN3_bm

#ifndef DISPLAY_REGISTER_SELECT_PORT
#define DISPLAY_REGISTER_SELECT_PORT DISPLAY_CTRL_PORT
#endif

#ifndef DISPLAY_RW_PORT
#define DISPLAY_RW_PORT DISPLAY_CTRL_PORT
#endif

#ifndef DISPLAY_ENABLE_PORT
#define DISPLAY_ENABLE_PORT DISPLAY_CTRL_PORT
#endif

#define DISPLAY_DATA_MASK (DISPLAY_DATA_0 | DISPLAY_DATA_1 | DISPLAY_DATA_2 | DISPLAY_DATA_3)
#define DISPLAY_CTRL_MASK (DISPLAY_REGISTER_SELECT | DISPLAY_RW | DISPLAY_ENABLE)

#define DISPLAY_ENABLE_HIGH() DigitalPort::SetHigh(DISPLAY_ENABLE_PORT, DISPLAY_ENABLE)
#define DISPLAY_ENABLE_LOW()  DigitalPort::SetLow(DISPLAY_ENABLE_PORT, DISPLAY_ENABLE)

#define DISPLAY_RW_HIGH() DigitalPort::SetHigh(DISPLAY_RW_PORT, DISPLAY_RW)
#define DISPLAY_RW_LOW()  DigitalPort::SetLow(DISPLAY_RW_PORT, DISPLAY_RW)

#define DISPLAY_REGISTER_SELECT_HIGH() DigitalPort::SetHigh(DISPLAY_REGISTER_SELECT_PORT, DISPLAY_REGISTER_SELECT)
#define DISPLAY_REGISTER_SELECT_LOW()  DigitalPort::SetLow(DISPLAY_REGISTER_SELECT_PORT, DISPLAY_REGISTER_SELECT)

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

static void DispDelay()
{
    SpinTimer::SleepuS(1);
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

static void DispToggleEnable()
{
    DISPLAY_ENABLE_HIGH();
    DispDelay();
    DISPLAY_ENABLE_LOW();
    DispDelay();
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void DisplayLCD::WaitBusy()
{
    while( Read(true) & LCD_BUSY ) wdt_reset();
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

uint8 DisplayLCD::Read(bool command)
{
    uint8 data;

    DigitalPort::SetAsInput(DISPLAY_DATA_PORT, DISPLAY_DATA_MASK);
    
    if ( command ) {
        DISPLAY_REGISTER_SELECT_LOW();
    } else {
        DISPLAY_REGISTER_SELECT_HIGH();
    }
    DISPLAY_RW_HIGH();    
//    DispDelay();
    DISPLAY_ENABLE_HIGH();
        
    DispDelay();
                
    data = DigitalPort::Get(DISPLAY_DATA_PORT) << 4;

    DISPLAY_ENABLE_LOW();
    DispDelay();
    DISPLAY_ENABLE_HIGH();
    
    DispDelay();
    data |= DigitalPort::Get(DISPLAY_DATA_PORT) & 0x0f;
        
    DISPLAY_ENABLE_LOW();
    DISPLAY_RW_LOW();
    DigitalPort::SetAsOutput(DISPLAY_DATA_PORT, DISPLAY_DATA_MASK);
    DispDelay();
    
    return data;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void DisplayLCD::Write(bool command, uint8 data)
{
    if ( command ) {
        DISPLAY_REGISTER_SELECT_LOW();
    } else {
        DISPLAY_REGISTER_SELECT_HIGH();
    } 
    DigitalPort::SetSome(DISPLAY_DATA_PORT, DISPLAY_DATA_MASK, data >> 4);
    DispToggleEnable();

    DigitalPort::SetSome(DISPLAY_DATA_PORT, DISPLAY_DATA_MASK, data & 0xf);
    DispToggleEnable();
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void DisplayLCD::WriteCharacter(char data)
{
    if ( m_CursorX >= WIDTH )
    {
        Newline(false);
    }
    WaitBusy();
    Write(false, data);
    m_CursorX++;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void DisplayLCD::WriteCommand(uint8 data)
{
    WaitBusy();
    Write(true, data);
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void DisplayLCD::ClearDisplay()
{
    m_CursorX = 0;
    m_CursorY = 0;
    WriteCommand(LCD_CLR);
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void DisplayLCD::ClearTo(uint8 xPos)
{
    while(m_CursorX < xPos )
    {
        WriteCharacter(' ');
    }    
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void DisplayLCD::ClearToEndOfLine()
{
    ClearTo(WIDTH);
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void DisplayLCD::SetCursor( uint8 x, uint8 y )
{
    m_CursorX = x;
    m_CursorY = y;
    
    uint8 address = 0;
    switch(y)
    {
        case 0: address = LCD_LINE_ADDR1; break;
        case 1: address = LCD_LINE_ADDR2; break;
        case 2: address = LCD_LINE_ADDR3; break;
        case 3: address = LCD_LINE_ADDR4; break;
    }
    address += x;

    WriteCommand(LCD_DDRAM | address);
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void DisplayLCD::Newline(bool clearToEnd)
{
    if (!clearToEnd)
    {
        if ( m_CursorY < HEIGHT )
        {
            SetCursor(0, m_CursorY + 1);
        }
        else
        {
            SetCursor(0, 0);
        }
    }
    else
    {
        ClearToEndOfLine();
        Newline(false);
    }        
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void DisplayLCD::PrintString( const char* i_pzString, uint8 i_nMinLength, uint8 maxLength )
{
    uint8 i;
	
    for ( i = 0 ; i_pzString[i] != 0 && i < maxLength ; ++i )
    {
        if ( i_pzString[i] != '\n' ) {
	    WriteCharacter(i_pzString[i]);
        } else {
            Newline(false);
        }                        
	wdt_reset();
    }
    for ( ; i < i_nMinLength ; ++i ) {
	WriteCharacter(' ');
	wdt_reset();
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

int DisplayLCD::Printf_P(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    int result = vfprintf_P(&m_DisplayFile, format, args);
    va_end(args);
    return result;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void DisplayLCD::InitializeDisplay()
{
    DigitalPort::SetAsOutput(DISPLAY_DATA_PORT, DISPLAY_DATA_MASK);
    DigitalPort::EnablePullup(DISPLAY_DATA_PORT, DISPLAY_DATA_MASK);

    DigitalPort::SetAsOutput(DISPLAY_REGISTER_SELECT_PORT, DISPLAY_REGISTER_SELECT);
    DigitalPort::SetAsOutput(DISPLAY_RW_PORT, DISPLAY_RW);
    DigitalPort::SetAsOutput(DISPLAY_ENABLE_PORT, DISPLAY_ENABLE);

    DigitalPort::EnablePullup(DISPLAY_REGISTER_SELECT_PORT, DISPLAY_REGISTER_SELECT);
    DigitalPort::EnablePullup(DISPLAY_RW_PORT, DISPLAY_RW);
    DigitalPort::EnablePullup(DISPLAY_ENABLE_PORT, DISPLAY_ENABLE);
    
    DISPLAY_ENABLE_HIGH();

    SpinTimer::SleepMS(15);
	
    DISPLAY_RW_LOW();
    DISPLAY_REGISTER_SELECT_LOW();

    DigitalPort::SetSome(DISPLAY_DATA_PORT, DISPLAY_DATA_MASK, DISPLAY_DATA_0 | DISPLAY_DATA_1);
    DispToggleEnable();
    
    SpinTimer::SleepuS(4100);

    DispToggleEnable();
    SpinTimer::SleepuS(100);

    DISPLAY_ENABLE_HIGH();
    DISPLAY_REGISTER_SELECT_HIGH();

    WriteCommand(LCD_FUNCTION | LCD_FUNCTION_2LINES); // 4-bit, 2-lines, small font
    WriteCommand(LCD_CLR);  // Clear display
    
    fdev_setup_stream(&m_DisplayFile, DisplayPutchar, NULL, _FDEV_SETUP_WRITE);
    m_DisplayFile.udata = this;
    
/*
    // Render the 8 custom characters used for level bars
    WriteCommand(LCD_CGRAM); // Set CGRAM addr 0
    
    for ( i = 0 ; i < 8 ; ++i )
    {
        for ( y = 7 ; y >= 0 ; --y )
        {
            if ( y <= i )
            {
                Write(false, 0xff);
            } else {
                Write(false, 0x00);
            }
        }
        wdt_reset();
    }*/
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void DisplayLCD::EnableDisplay(bool turnOn, bool enableCursor, bool enableBlinking)
{
    uint8_t cmd = LCD_ON;
    if ( turnOn )         cmd |= LCD_ON_DISPLAY;
    if ( enableCursor )   cmd |= LCD_ON_CURSOR;
    if ( enableBlinking ) cmd |= LCD_ON_BLINK;
    WriteCommand(cmd); // Display on, cursor off, no cursor blinking    
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void DisplayLCD::WriteCustomCharBegin(uint8_t charIndex)
{
    WriteCommand(LCD_CGRAM | (charIndex * 8)); // Set CGRAM address    
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void DisplayLCD::WriteCustomCharRow(uint8_t bits)
{
    WaitBusy();
    Write(false, bits);
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void DisplayLCD::PrintHorizontalBar(int8_t y, uint8_t percentage, int8_t barNum)
{

    // Scale 100 to 98.
    if ( percentage > 66 ) {
        percentage -= 1;
    } else if ( percentage < 33 ) {
        percentage += 1;
    }        
    
    uint8_t fullChars = percentage / 5;
    uint8_t pixels = percentage % 5;

    uint8_t charBits = ((1<<pixels) - 1) << (5 - pixels);

    if ( fullChars == 19 ) charBits |= 0x01;
    
    // Empty:
    WriteCustomCharBegin(0);
    WriteCustomCharRow(0x1f);
    for ( int8_t i = 0 ; i < 6 ; ++i )
    {
        WriteCustomCharRow(0);
    }
    WriteCustomCharRow(0x1f);

    // Full:
    WriteCustomCharBegin(1);
    WriteCustomCharRow(0x1f);
    WriteCustomCharRow(0x00);
    for ( int8_t i = 0 ; i < 4 ; ++i )
    {
        WriteCustomCharRow(0x1f);
    }
    WriteCustomCharRow(0x00);
    WriteCustomCharRow(0x1f);

    // Start cap:
    WriteCustomCharBegin(2);
    WriteCustomCharRow(0x1f);
    WriteCustomCharRow(0x10);
    for ( int8_t i = 0 ; i < 4 ; ++i )
    {
        WriteCustomCharRow(0x1f);
    }
    WriteCustomCharRow(0x10);
    WriteCustomCharRow(0x1f);

    // End cap:
    WriteCustomCharBegin(3);
    WriteCustomCharRow(0x1f);
    for ( int8_t i = 0 ; i < 6 ; ++i )
    {
        WriteCustomCharRow(1);
    }
    WriteCustomCharRow(0x1f);

    // Partial:
    uint8_t partialSpacer;
    if ( fullChars == 0 ) {
        partialSpacer = 0x10;
    } else if ( fullChars == 19 ) {
        partialSpacer = 0x01;
    } else {        
        partialSpacer = 0x00;
    }
    
    WriteCustomCharBegin(4 + barNum);
    WriteCustomCharRow(0x1f);
    WriteCustomCharRow(partialSpacer);
    for ( int8_t i = 0 ; i < 4 ; ++i )
    {
        WriteCustomCharRow(charBits);
    }
    WriteCustomCharRow(partialSpacer);
    WriteCustomCharRow(0x1f);
    
    // Print all full blocks.
    SetCursor(0, y);
    for ( int8_t i = 0 ; i < fullChars ; ++i )
    {
        WriteCharacter((i==0) ? 2 : 1);
    }
    // Print the partial block.
    if ( fullChars != 20 )
    {
        WriteCharacter(4 + barNum);
        fullChars++;
    }    
    // Print all empty blocks.
    for ( int8_t i = fullChars ; i < 19 ; ++i )
    {
        WriteCharacter(0);
    }
    // Print the "end cap".
    if ( fullChars < 20 ) {
        WriteCharacter(3);
    }
    
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void DisplayLCD::PrintScrollingText( const char* text, int8_t maxStrLength, bool leftAlign, int8_t startX, int8_t maxDispLength, int8_t line, int8_t* scrollOffset )
{
    int8_t strLength = strnlen(text, maxStrLength);
    if (strLength > maxDispLength)
    {
        int8_t strOversize = strLength - maxDispLength;
        if (*scrollOffset >= strOversize*2 + 2) *scrollOffset = 0;
        
        int8_t nameOffset = clamp<int8_t>(0, strOversize * 2 - 1, *scrollOffset - 2);
        
        if (nameOffset <= strOversize) {
            text += nameOffset;
        } else {
            text += (strLength - maxDispLength) * 2 - nameOffset;
        }
        SetCursor(startX, line);
        PrintString(text, maxDispLength, maxDispLength);
    }
    else
    {
        SetCursor(startX, line);
        if (!leftAlign) {
            ClearTo(startX + maxDispLength - strLength);
        }
        PrintString(text, strLength, strLength);
        ClearTo(startX + maxDispLength);
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

int DisplayLCD::DisplayPutchar(char c, FILE* stream)
{
    DisplayLCD* display = static_cast<DisplayLCD*>(stream->udata);
    display->PrintString(&c, 1, 1);
    return 0;
}

/*
void DisplayLCD::SetDisplayChar( uint8 level )
{
    if ( level == 0 ) {
        WriteDisplayByte( false, 0x20 ); // Empty
    } else {
        WriteDisplayByte( false, level - 1 ); // Something
    }
}*/
/*
char DisplayLCD::GetBarChar( uint8 level )
{
    if ( level == 0 ) {
        return 0x20;
    } else {
        return level - 1;
    }
}

void DisplayLCD::SetDisplayBar( uint8 bar, uint8 level )
{
    if ( level < 8 ) {
        SetCursor( 11 + bar, 0 );
        WriteCharacter(' ');
        
        SetCursor( 11 + bar, 1 );
        
        WriteCharacter(GetBarChar( level ));
    } else {
        SetCursor( 11 + bar, 1 );
        WriteCharacter(0x07);
        
        SetCursor( 11 + bar, 0 );
        
        WriteCharacter( GetBarChar( level - 8 ) );
    }
}
*/