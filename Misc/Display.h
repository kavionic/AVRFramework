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


#ifndef F_MISC_DISPLAY_H__
#define F_MISC_DISPLAY_H__

#include <avr/io.h>
#include <stdio.h>

#include "CustomTypes.h"

#define LCD_LINES          2     /**< number of visible lines of the display */
#define LCD_DISP_LENGTH   16     /**< visibles characters per line of the display */
#define LCD_LINE_LENGTH 0x40     /**< internal line length of the display    */
#define LCD_LINE_ADDR1  0x00     /**< DDRAM address of first char of line 1 */
#define LCD_LINE_ADDR2  0x40     /**< DDRAM address of first char of line 2 */
#define LCD_LINE_ADDR3  0x14     /**< DDRAM address of first char of line 3 */
#define LCD_LINE_ADDR4  0x54     /**< DDRAM address of first char of line 4 */
#define LCD_WRAP_LINES     0     /**< 0: no wrap, 1: wrap at end of visibile line */

/**
 *  @name Definitions for LCD command instructions
 *  The constants define the various LCD controller instructions which can be passed to the 
 *  function lcd_command(), see HD44780 data sheet for a complete description.
 */

/* instruction register bit positions, see HD44780U data sheet */
#define LCD_CLR               (1<<0)      /* DB0: clear display                  */
#define LCD_HOME              1      /* DB1: return to home position        */

#define LCD_ENTRY_MODE        (1<<2)      /* DB2: set entry mode                 */
#define LCD_ENTRY_INC         (1<<1)      /*   DB1: 1=increment, 0=decrement     */
#define LCD_ENTRY_SHIFT       (1<<0)      /*   DB2: 1=display shift on           */

#define LCD_ON                (1<<3)      /* DB3: turn lcd/cursor on             */
#define LCD_ON_DISPLAY        (1<<2)      /*   DB2: turn display on              */
#define LCD_ON_CURSOR         (1<<1)      /*   DB1: turn cursor on               */
#define LCD_ON_BLINK          (1<<0)      /*     DB0: blinking cursor ?          */

#define LCD_MOVE              (1<<4)      /* DB4: move cursor/display            */
#define LCD_MOVE_DISP         (1<<3)      /*   DB3: move display (0-> cursor) ?  */
#define LCD_MOVE_RIGHT        (1<<2)      /*   DB2: move right (0-> left) ?      */

#define LCD_FUNCTION          (1<<5)      /* DB5: function set                   */
#define LCD_FUNCTION_8BIT     (1<<4)      /*   DB4: set 8BIT mode (0->4BIT mode) */
#define LCD_FUNCTION_2LINES   (1<<3)      /*   DB3: two lines (0->one line)      */
#define LCD_FUNCTION_10DOTS   (1<<2)      /*   DB2: 5x10 font (0->5x7 font)      */

#define LCD_CGRAM             (1<<6)      /* DB6: set CG RAM address             */
#define LCD_DDRAM             (1<<7)      /* DB7: set DD RAM address             */

#define LCD_BUSY              (1<<7)      /* DB7: LCD is busy                    */


#define LCD_MODE_DEFAULT     ((1<<LCD_ENTRY_MODE) | (1<<LCD_ENTRY_INC) )

// Bit definitions for custom characters
#define CB_00000    0x00
#define CB_00001    0x01
#define CB_00010    0x02
#define CB_00011    0x03
#define CB_00100    0x04
#define CB_00101    0x05
#define CB_00110    0x06
#define CB_00111    0x07
#define CB_01000    0x08
#define CB_01001    0x09
#define CB_01010    0x0a
#define CB_01011    0x0b
#define CB_01100    0x0c
#define CB_01101    0x0d
#define CB_01110    0x0e
#define CB_01111    0x0f
#define CB_10000    0x10
#define CB_10001    0x11
#define CB_10010    0x12
#define CB_10011    0x13
#define CB_10100    0x14
#define CB_10101    0x15
#define CB_10110    0x16
#define CB_10111    0x17
#define CB_11000    0x18
#define CB_11001    0x19
#define CB_11010    0x1a
#define CB_11011    0x1b
#define CB_11100    0x1c
#define CB_11101    0x1d
#define CB_11110    0x1e
#define CB_11111    0x1f

class DisplayLCD
{
public:
    static const uint8 WIDTH  = 20;
    static const uint8 HEIGHT = 4;
    
    void InitializeDisplay();

    void EnableDisplay(bool turnOn, bool enableCursor = false, bool enableBlinking = false);

    void ClearDisplay();
    void ClearTo(uint8 xPos);
    void ClearToEndOfLine();
    void SetCursor( uint8 x, uint8 y );
    uint8_t GetCursorX() const { return m_CursorX; }
    uint8_t GetCursorY() const { return m_CursorY; }
        
    void Newline(bool clearToEnd);
//    void PrintString( const char* i_pzString, uint8 i_nMinLength );
    void PrintString( const char* i_pzString, uint8 i_nMinLength, uint8 maxLength = 80 );
    void PrintHorizontalBar(int8_t y, uint8_t percentage, int8_t barNum = 0);

    void PrintScrollingText(const char* text, int8_t maxStrLength, bool leftAlign, int8_t startX, int8_t maxDispLength, int8_t line, int8_t* scrollOffset);

    int Printf_P(const char* format, ...);

    void WriteCustomCharBegin(uint8_t charIndex);
    void WriteCustomCharRow(uint8_t bits);

    char GetBarChar( uint8 level );
    void SetDisplayBar( uint8 bar, uint8 level );
    
    uint8 Read(bool command);
    
    void Write(bool command, uint8 data);
    void WriteData(uint8 data) __attribute__ ((deprecated)) { WriteCharacter(data); }
    void WriteCharacter(char data);
    void WriteCommand(uint8 data);

    void WaitBusy();
    
private:    
    static int DisplayPutchar(char c, FILE* stream);
    
    uint8 m_CursorX:6;
    uint8 m_CursorY:2;
    
    FILE m_DisplayFile;    
};


#endif // F_MISC_DISPLAY_H__
