// This file is part of ESP8266 Driver.
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


#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef ESP8266_LEAN
#define ESP8266_LOG_ERROR(...) DB_LOG(DP_ERROR, __VA_ARGS__)
//#define ESP8266_LOG_CRITICAL(...) DB_LOG(DP_CRITICAL, __VA_ARGS__)
#else
#define ESP8266_LOG_ERROR(...)
//#define ESP8266_LOG_CRITICAL(...)
#endif

#include "Misc/Utils.h"
#include "Misc/DigitalPort.h"

#include "ESP8266.h"


static char g_SerialReceiveBuffer[WIFI_SERIAL_IN_BUFFER_SIZE];
static int16_t g_SerialReceiveBufferOutPos = 0;
//static volatile bool     g_SerialReceiveBufferHasOverflowed = false;
static volatile bool     g_ReceiveTimedOut = false;

/*
ISR(WIFI_USART_RX_INT)
{
    if (WIFI_USART.STATUS & USART_BUFOVF_bm)
    {
        //ESP8266_LOG_ERROR(PSTRL("**************** USART OVERFLOW!!!!"));
        g_SerialReceiveBufferHasOverflowed = true;
    }        
    while(WIFI_USART.STATUS & USART_RXCIF_bm)
    {
        uint8_t data = WIFI_USART.DATA;
        if ( g_SerialReceiveBufferAvailableBytes < WIFI_SERIAL_IN_BUFFER_SIZE)
        {
            g_SerialReceiveBuffer[g_SerialReceiveBufferInPos] = data;
            g_SerialReceiveBufferInPos++; // = (g_SerialReceiveBufferInPos + 1) & (WIFI_SERIAL_IN_BUFFER_SIZE - 1);
            if (g_SerialReceiveBufferInPos >= WIFI_SERIAL_IN_BUFFER_SIZE) g_SerialReceiveBufferInPos = 0;
            g_SerialReceiveBufferAvailableBytes++;    
        }
        else
        {
            g_SerialReceiveBufferHasOverflowed = true;
        }
    }
}*/

static int16_t GetSerialDataWritePos()
{
    cli();
    int16_t pos = *((volatile uint16_t*)&DMA.CH2.DESTADDR0) - intptr_t(g_SerialReceiveBuffer);
    sei();
    return pos;
}

static bool IsSerialDataAvailable()
{
    return GetSerialDataWritePos() != g_SerialReceiveBufferOutPos;
}

static int16_t GetAvailableSerialBytes()
{
    int16_t writePos = GetSerialDataWritePos();

    if (g_SerialReceiveBufferOutPos <= writePos) {
        return writePos - g_SerialReceiveBufferOutPos;    
    } else {
        return writePos + WIFI_SERIAL_IN_BUFFER_SIZE - g_SerialReceiveBufferOutPos;
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void ESP8266::Initialize(bool wifiBridge)
{
    // Set mode of operation
    if (!wifiBridge) {
//        WIFI_USART.CTRLA = USART_RXCINTLVL_HI_gc; // Enable receive interrupt.
    }
    WIFI_USART.CTRLC = USART_CMODE_ASYNCHRONOUS_gc | USART_PMODE_DISABLED_gc | USART_CHSIZE_8BIT_gc;			// async, no parity, 8 bit data, 1 stop bit
    // Enable transmitter and receiver
    WIFI_USART.CTRLB = USART_TXEN_bm | USART_RXEN_bm | USART_CLK2X_bm;

    SetUSARTBaudRate(-5, 1079); // 115200
    //SetUSARTBaudRate(9600);
    DigitalPort::SetAsOutput(WIFI_RADIO_FLASHEN_PORT, WIFI_RADIO_FLASHEN_PIN);
    DigitalPort::SetHigh(WIFI_RADIO_FLASHEN_PORT, WIFI_RADIO_FLASHEN_PIN);
    
    DigitalPort::SetAsOutput(WIFI_USART_OUT_PORT, WIFI_USART_OUT_PIN);
    DigitalPort::SetAsOutput(WIFI_RADIO_RESET_PORT, WIFI_RADIO_RESET_PIN);
    DigitalPort::SetHigh(WIFI_RADIO_RESET_PORT, WIFI_RADIO_RESET_PIN);
    WIFI_USART_OUT_PORT->REMAP = PORT_USART0_bm; // Move USART from pin 2/3 to pin 6/7.

    DMA.CTRL = DMA_ENABLE_bm | DMA_DBUFMODE_CH01_gc | DMA_PRIMODE_RR0123_gc;

    DMA.CH2.TRFCNT    = WIFI_SERIAL_IN_BUFFER_SIZE;
    DMA.CH2.REPCNT    = 0;
    DMA.CH2.ADDRCTRL  = DMA_CH_SRCDIR_FIXED_gc | DMA_CH_SRCRELOAD_NONE_gc | DMA_CH_DESTDIR_INC_gc | DMA_CH_DESTRELOAD_BLOCK_gc;
    DMA.CH2.SRCADDR0  = ((intptr_t)&WIFI_USART.DATA) & 0xff;
    DMA.CH2.SRCADDR1  = (((intptr_t)&WIFI_USART.DATA) >> 8) & 0xff;
    DMA.CH2.SRCADDR2  = 0;
    DMA.CH2.DESTADDR0 = ((intptr_t)g_SerialReceiveBuffer) & 0xff;
    DMA.CH2.DESTADDR1 = (((intptr_t)g_SerialReceiveBuffer) >> 8) & 0xff;
    DMA.CH2.DESTADDR2 = 0;
    DMA.CH2.TRIGSRC   = DMA_CH_TRIGSRC_USARTF0_RXC_gc;
    DMA.CH2.CTRLA     = DMA_CH_ENABLE_bm | DMA_CH_REPEAT_bm | DMA_CH_SINGLE_bm | DMA_CH_BURSTLEN_1BYTE_gc;

#ifndef ESP8266_LEAN
    if (wifiBridge)
    {
        cli();
        for (;;)
        {
            if ((WIFI_USART.STATUS & USART_RXCIF_bm)) {
                uint8_t c = WIFI_USART.DATA;
                while(!(DEBUG_USART.STATUS & USART_DREIF_bm));
                DEBUG_USART.DATA = c;
            }
            if ((DEBUG_USART.STATUS & USART_RXCIF_bm)) {
                uint8_t c = DEBUG_USART.DATA;
                while(!(WIFI_USART.STATUS & USART_DREIF_bm));
                WIFI_USART.DATA = c;
            }
        }
    }
#endif
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool ESP8266::Run()
{
    if (g_ReceiveTimedOut) {
        DB_LOG(DP_ERROR, PSTRL("ERROR: WIFI USART timeout!"));
        return false;
    }
/*    if (g_SerialReceiveBufferHasOverflowed) {
        DB_LOG(DP_ERROR, PSTRL("ERROR: WIFI USART buffer overflow!"));
        return false;
    }*/
    if (m_StatusFlags & (e_StatusConnectedToAP | e_StatusHotspotEnabled))
    {
        if(IsSerialDataAvailable())
        {
//            if (g_SerialReceiveBufferHasOverflowed) {
//                ESP8266_LOG_CRITICAL(PSTRL("ERROR: WIFI USART buffer overflow!"));
//                return false;
//            }
            if ( !ReadResponseLine(1000, true) ) {
                ESP8266_LOG_ERROR(PSTRL("ERROR: RDP8266::Run() failed to read response line!"));
                return false;
            }
            m_ResponseDataLength = 0;
        }
/*        for (int8_t i = 0 ; i < WIFI_MAX_LINKS; ++i)
        {
            if (m_TotalDataReceived[i] != 0)
            {
                WifiSetVal16 msg;
                WifiPackageHeader::InitMsg(msg, WifiCmd_e::e_BytesReceivedReply);
                msg.m_Value = m_TotalDataReceived[i];
                m_TotalDataReceived[i] = 0;
                SendIPData(i, &msg, sizeof(msg));
            }
        }*/
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool ESP8266::ReadCommandResponse(uint16_t timeout, bool logResponse)
{
    for(;;)
    {
        if (ReadResponseLine(timeout, false))
        {
            if (strcmp(m_ResponseBuffer, "OK") == 0)
            {
                return true;
            }
            else if (strcmp(m_ResponseBuffer, "ERROR") == 0)
            {
                DB_LOG(DP_ERROR, PSTRL("ReadCommandResponse() ERROR."));
                return false;
            }
            else if (logResponse)
            {
                DB_LOG(DP_INFO, PSTRL(": "), m_ResponseBuffer);
            }
        }
        else
        {
            DB_LOG(DP_ERROR, PSTRL("ERROR: Failed to read ESP8266 response!"));
            return false;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool ESP8266::ProcessOutOfBandData()
{
    bool result = false;
    if (m_ResponseDataLength != 0)
    {
#ifndef ESP8266_DONT_TRACK_CONNECTIONS        
        if ( m_ResponseDataLength == 9 && PgmUtils::strcmp(m_ResponseBuffer+1, PSTRL(",CONNECT")) == 0)
        {
            uint8_t linkID = m_ResponseBuffer[0] - '0';
            ESP8266_LOG_ERROR(PSTRL("New connection at: "), linkID);
            m_ConnectedLinks |= BIT8(linkID, 1);
            ESP8266_ConnectionChangedCallback(linkID, true);
            result = true;
        }
        else if ( m_ResponseDataLength == 8 && PgmUtils::strcmp(m_ResponseBuffer+1, PSTRL(",CLOSED")) == 0)
        {
            uint8_t linkID = m_ResponseBuffer[0] - '0';
            ESP8266_LOG_ERROR(PSTRL("Connection closed: "), linkID);
            m_ConnectedLinks &= U8(~BIT8(linkID, 1));
            ESP8266_ConnectionChangedCallback(linkID, false);
            result = true;
        }
        else
#endif // ESP8266_DONT_TRACK_CONNECTIONS
        if (m_ResponseDataLength >= 8 && PgmUtils::strncmp(m_ResponseBuffer + m_ResponseDataLength - 8, PSTRL(",SEND OK"), 8) == 0)
        {
            //ESP8266_LOG_ERROR(PSTRL("Send OK: "), m_ResponseBuffer);
            result = true;
        }
/*        else
        {
            ESP8266_LOG_ERROR(PSTRL("Unknown data: "), m_ResponseBuffer);
            return true;
        }*/
        if (result) {
            m_ResponseDataLength = 0;
        }            
    }
    return result;
}
///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool ESP8266::ReadResponseLine(uint16_t timeout, bool breakOnIPData)
{    
    m_ResponseDataLength = 0;
    for(;;)
    {
        char c;
        if (!ReadChar(&c, timeout)) {
            m_ResponseBuffer[m_ResponseDataLength] = 0;
            DB_LOG(DP_ERROR, PSTRL("ERROR: ReadResponseLine() timeout! "), m_ResponseBuffer);
            return false;
        }
        
        if (c == '\r') continue;
        
        //ESP8266_LOG_ERROR((c) ? c : '?');
        if (c == '\n')
        {
            m_ResponseBuffer[m_ResponseDataLength] = 0;
            if (ProcessOutOfBandData())
            {
                if (breakOnIPData) {
                    return true;
                } else {
                    continue;
                }                    
            }                
            //ESP8266_LOG_ERROR(PSTRL("#: "), m_ResponseBuffer);
            return true;
        }
        if (m_ResponseDataLength < RESPONSE_BUFFER_SIZE - 1)
        {
            m_ResponseBuffer[m_ResponseDataLength++] = c;
        }
        else
        {
            m_ResponseBuffer[m_ResponseDataLength] = 0;
            for (uint8_t i = 0 ; i < m_ResponseDataLength ;++i ) {
                if (m_ResponseBuffer[i] == 0) m_ResponseBuffer[i] = '?';
            }
            DB_LOG(DP_CRITICAL, PSTRL("ERROR: ReadResponseLine() buffer overflow! "), m_ResponseDataLength);
            DB_LOG(DP_ERROR, m_ResponseBuffer);
            return false;
        }
        if (m_ResponseDataLength == 4 && PgmUtils::strncmp(m_ResponseBuffer, PSTRL("+IPD"), 4) == 0)
        {
            ProcessIPData();
            if (breakOnIPData)
            {
                m_ResponseDataLength = 0;
                return true;
            }
        }            
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void ESP8266::ResetRadio()
{
    DB_LOG(DP_INFO, PSTRL("Resetting radio."));
    

    DigitalPort::SetLow(WIFI_RADIO_RESET_PORT, WIFI_RADIO_RESET_PIN);
    SpinTimer::SleepMS(100);
//    SetUSARTBaudRate(115200);
    SetUSARTBaudRate(-5, 1079); // 115200
    g_SerialReceiveBufferOutPos = 0;
    DMA.CH2.DESTADDR0 = ((intptr_t)g_SerialReceiveBuffer) & 0xff;
    DMA.CH2.DESTADDR1 = (((intptr_t)g_SerialReceiveBuffer) >> 8) & 0xff;
    DMA.CH2.DESTADDR2 = 0;
//    g_SerialReceiveBufferHasOverflowed = false;
    m_StatusFlags &= ~(e_StatusConnectingToAP | e_StatusConnectedToAP | e_StatusHotspotEnabled | e_StatusServerRunning);
#ifndef ESP8266_DONT_TRACK_CONNECTIONS
    m_ConnectedLinks = 0;
#endif // ESP8266_DONT_TRACK_CONNECTIONS
    
    DigitalPort::SetHigh(WIFI_RADIO_RESET_PORT, WIFI_RADIO_RESET_PIN);
//    SpinTimer::SleepMS(1);

    char c;
    char buffer[7];
    while(ReadChar(&c, 15000))
    {
        memmove(buffer, buffer + 1, 6);
        buffer[6] = c;
        if ( PgmUtils::strncmp(buffer, PSTRL("ready\r\n"), 7) == 0 ) {
            break;
        }
//        ESP8266_LOG_ERROR(c);
        DB_LOG(DP_INFO, PSTRL("."), SFStop());
    }
    DB_LOG(DP_INFO, PSTRL("\r\nReset done."));
    g_ReceiveTimedOut = false;
    memset(m_TotalDataReceived, 0, sizeof(m_TotalDataReceived));
    DisconnectFromAP();
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool ESP8266::RestartRadio()
{
    ESP8266_LOG_ERROR(PSTRL("AT+RST"));

    m_StatusFlags &= ~(e_StatusConnectingToAP | e_StatusConnectedToAP | e_StatusHotspotEnabled | e_StatusServerRunning);
    SendCommand(PSTRL("AT+RST"));
//    SetUSARTBaudRate(115200);
    SetUSARTBaudRate(-5, 1079); // 115200
    
    if ( ReadCommandResponse(60000) )
    {
        ESP8266_LOG_ERROR(PSTRL("Radio restarted."));
        for(;;)
        {
            if ( !ReadResponseLine(30000, false) ) {
                ESP8266_LOG_ERROR(PSTRL("ERROR: Failed to restart the radio."));
                return false;
            }
            
            ESP8266_LOG_ERROR(m_ResponseBuffer);
            if ( PgmUtils::strcmp(m_ResponseBuffer, PSTRL("ready")) == 0 ) {
                ESP8266_LOG_ERROR(PSTRL("Radio booted."));
                return true;
            }
        }
        return true;
    } else {
        DB_LOG(DP_ERROR, PSTRL("ERROR: Failed to restart the radio."));
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool ESP8266::SetBaudrate()
{
//    uint32_t baudrate = 115200*23;
    uint32_t baudrate = 115200*10;
//    uint32_t baudrate = 115200*4;
 
    ESP8266_LOG_ERROR(PSTRL("Set baudrate"));   
    if ( ExecuteCommand(PSTRL("AT+UART_CUR="), baudrate, PSTRL(",8,1,0,0")) )
    {
        ESP8266_LOG_ERROR(PSTRL("Baudrate set:"));
        
//        SetUSARTBaudRate(baudrate);
        SetUSARTBaudRate(-5, 79); // 1152000
        
        SpinTimer::SleepMS(1);
        return true;
    } else {
        DB_LOG(DP_ERROR, PSTRL("ERROR: Failed to set baudrate."));
//        SetUSARTBaudRate(baudrate);
        SetUSARTBaudRate(-5, 1079); // 115200
        return true;
    }
    
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool ESP8266::PokeRadio()
{
    if ( !ExecuteCommand(PSTRL("AT+GMR")) ) {
        DB_LOG(DP_ERROR, PSTRL("ERROR: Failed to poke."));
        return false;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

#ifndef ESP8266_LEAN
bool ESP8266::GetModuleVersion()
{
    SendCommand(PSTRL("AT+GMR"));
    
    ESP8266_LOG_ERROR(PSTRL("Request Version..."));
    if ( ReadCommandResponse(5000, true) ) {
        return true;
    } else {
        ESP8266_LOG_ERROR(PSTRL("ERROR: Failed to get version info."));
        return false;
    }
}
#endif // ESP8266_LEAN

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool ESP8266::SetWifiMode(WifiMode_e::Enum mode)
{
    if ( ExecuteCommand(PSTRL("AT+CWMODE_CUR="), mode) ) {
        ESP8266_LOG_ERROR(PSTRL("Wifi mode set: "), mode);
        return true;
    } else {
        ESP8266_LOG_ERROR(PSTRL("ERROR: Failed to set wifi mode."));
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void ESP8266::EnableEcho(bool doEcho)
{
    if ( !ExecuteCommand(PSTRL("ATE"), (doEcho) ? 1 : 0) ) {
        DB_LOG(DP_ERROR, PSTRL("ERROR: Failed to set echo mode."));
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void ESP8266::SetMuxMode(WifiMuxMode_e::Enum mode)
{
    //DB_LOG(DP_SPAM, PSTRL("AT+CIPMUX="), mode);
    if ( !ExecuteCommand(PSTRL("AT+CIPMUX="), mode) ) {
        DB_LOG(DP_ERROR, PSTRL("ERROR: Failed to set mux mode "), mode);
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////
#ifndef ESP8266_LEAN
bool ESP8266::ListAPs()
{
    SendCommand(PSTRL("AT+CWLAP"));
    
    ESP8266_LOG_ERROR(PSTRL("APs:"));
    if ( ReadCommandResponse(10000, true) ) {
        return true;
    } else {
        ESP8266_LOG_ERROR(PSTRL("ERROR: Failed to get AP list."));
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

int8_t ESP8266::DiscoverAccessPoints(WifiAccessPoint* apList, int8_t listSize)
{
    if (m_StatusFlags & e_StatusConnectingToAP) {
        ESP8266_LOG_ERROR(PSTRL("ERROR: DiscoverAccessPoints() called while connecting to AP."));
        return -1;
    }
    SendCommand(PSTRL("AT+CWLAP"));
    // Parse response: '+CWLAP:(security[0-4],"SSID",strength,"mac",channel)'

    int8_t count = 0;
    for (;;)
    {
        if (ReadResponseLine(10000, false))
        {
            if (strcmp(m_ResponseBuffer, "OK") == 0) {
                return count;
            }
            if (strcmp(m_ResponseBuffer, "ERROR") == 0) {
                ESP8266_LOG_ERROR(PSTRL("ERROR: ESP8266::ReadAccessPoint() AT+CWLAP failed."));
                return -1;
            }
            if (count == listSize) continue;
            WifiAccessPoint& accessPoint = apList[count];
            if (PgmUtils::strncmp(m_ResponseBuffer, PSTRL("+CWLAP:("), 8) == 0)
            {
                char* str = m_ResponseBuffer + 8;
                accessPoint.m_Security = WifiSecurityMode_e::Enum(strtoul(str, &str, 10));
                str += 2;
                char* end = strchr(str, '"');
                if (end != nullptr)
                {
                    uint8_t nameLen = end - str;
                    if (nameLen > WIFI_MAX_SSID_LEN) {
                        nameLen = WIFI_MAX_SSID_LEN;
                    } else {
                        accessPoint.m_SSID[nameLen] = 0;
                    }                        
                    memcpy(accessPoint.m_SSID, str, nameLen);
                    str = end + 1;
                    if (*str == ',')
                    {
                        str++;
                        accessPoint.m_Strength = strtol(str, nullptr, 10);
                        count++;
                    }
                }
            }
        }
        else
        {
            ESP8266_LOG_ERROR(PSTRL("ERROR: ESP8266::ReadAccessPoint() failed to read response."));
            return -1;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////
/*
bool ESP8266::ReadAccessPoint(WifiAccessPoint* accessPoint)
{
    if (ReadResponseLine(10000, false))
    {
        if (PgmUtils::strcmp(m_ResponseBuffer, PSTRL("OK")) == 0) {
            return false;
        }
        if (PgmUtils::strcmp(m_ResponseBuffer, PSTRL("ERROR")) == 0) {
            ESP8266_LOG_ERROR(PSTRL("ERROR: ESP8266::ReadAccessPoint() AT+CWLAP failed."));
            return false;
        }
        if (PgmUtils::strncmp(m_ResponseBuffer, PSTRL("+CWLAP:"), 7) == 0)
        {
            const char* str = m_ResponseBuffer + 7;
            accessPoint->m_Security = strtoul(str, &str, 10);
            str += 1;
            const char* end = strchr(str, '"');
            if (end != nullptr)
            {
                uint8_t nameLen = end - str;
                if (nameLen > WIFI_MAX_SSID_LEN) nameLen = WIFI_MAX_SSID_LEN;
                strncpy(accessPoint->m_SSID, str, nameLen);
                str = end + 1;
                if (*str == ',')
                {
                    str++;
                    accessPoint->m_Strength = strtol(str, nullptr, 10);
                    return true;
                }
            }
        }
        return false;
    }
    else
    {
        ESP8266_LOG_ERROR(PSTRL("ERROR: ESP8266::ReadAccessPoint() failed to read response."));
        return false;        
    }
}*/
#endif // ESP8266_LEAN

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

int16_t ESP8266::GetSignalStrength()
{
    if (m_StatusFlags & e_StatusConnectingToAP) {
        ESP8266_LOG_ERROR(PSTRL("ERROR: GetSignalStrength() called while connecting to AP."));
        return 0x7fff;
    } else if ((m_StatusFlags & e_StatusConnectedToAP) == 0) {
        return (m_StatusFlags & e_StatusHotspotEnabled) ? 0 : 0x7fff;
    }        
    //ESP8266_LOG_ERROR(PSTRL("AT+CWLAP="), m_AccessPoint);
    //SendCommand(PSTRL("AT+CWLAP=\"%s\",\"cc:5d:4e:06:24:ac\"\r\n"), m_AccessPoint);
    SendCommand(PSTRL("AT+CWJAP_CUR?"));
    
    int16_t signalStrength = 0x7fff;
    for(;;)
    {
        if (!ReadResponseLine(5000, false)) {
            ESP8266_LOG_ERROR(PSTRL("ERROR: ESP8266::GetSignalStrength() failed to read command response."));
            return 0x7fff;
        }
        if (strcmp(m_ResponseBuffer, "OK") == 0) {
            return signalStrength;
        }
        if (strcmp(m_ResponseBuffer, "ERROR") == 0) {
            ESP8266_LOG_ERROR(PSTRL("ERROR: ESP8266::UpdateHotspotAddress() AT+CIPAP failed."));
            return 0x7fff;
        }
        if (m_ResponseDataLength == 0)
        {
            continue;
        }
        const char* str = m_ResponseBuffer;
        
        // Parse response: '+CWLAP:(security[0-4],"SSID",strength,"mac",channel)'
        // Parse response: '+CWJAP_CUR:"HMV35_2","cc:5d:4e:06:24:ac",2,-78'
        if (PgmUtils::strncmp(str, PSTRL("+CWJAP_CUR:"), 11) == 0)
        {
            str = PgmUtils::strstr(str+11, PSTRL(",\"")); // Find start of SSID
            if (str != nullptr)
            {
                str = PgmUtils::strstr(str + 2, PSTRL("\",")); // Find end of SSID
                if (str != nullptr)
                {
                    str = PgmUtils::strstr(str + 2, PSTRL(",")); // Find end of MAC
                    if (str != nullptr)
                    {
                        ParseInt(str + 1, signalStrength);
//                        signalStrength = strtol(str + 1, nullptr, 10);
                        continue;
                    }                        
                }
            }
        }
        ESP8266_LOG_ERROR(PSTRL("ERROR: GetSignalStrength() Failed to parse result: "), m_ResponseBuffer);
    }
    return 0x7fff;
}


///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool ESP8266::UpdateStationAddress()
{
    SendCommand(PSTRL("AT+CIFSR"));

    bool addressFound = false;    
    for(;;)
    {
        if (!ReadResponseLine(1000, false)) {
            ESP8266_LOG_ERROR(PSTRL("ERROR: ESP8266::UpdateStationAddress() failed to read command response."));
            return false;
        }
        if (strcmp(m_ResponseBuffer, "OK") == 0) {
            return addressFound;
        }
        if (strcmp(m_ResponseBuffer, "ERROR") == 0) {
            ESP8266_LOG_ERROR(PSTRL("ERROR: ESP8266::UpdateStationAddress() AT+CIFSR failed."));
            return false;
        }
        if (PgmUtils::strncmp(m_ResponseBuffer, PSTRL("+CIFSR:STAIP,\""), 14) == 0)
        {
            ESP8266_LOG_ERROR(PSTRL("Station IP: "), m_ResponseBuffer + 13);
            m_StationAddress = 0;
            char* numStr = m_ResponseBuffer + 14;
            for (uint8_t i = 0 ; i < 4 ; ++i)
            {
                uint16_t addressByte;
                numStr = ParseInt(numStr, addressByte);
                ESP8266_LOG_ERROR(PSTRL("'"), numStr, PSTRL("' "), addressByte);
                m_StationAddress |= U32(addressByte) << ((3-i)*8);
                numStr++; // Skip '.'
            }
            addressFound = true;
        }
        else if (PgmUtils::strncmp(m_ResponseBuffer, PSTRL("+CIFSR:STAMAC,\""), 15) == 0)
        {
            ESP8266_LOG_ERROR(PSTRL("Station MAC: "), m_ResponseBuffer + 14);
            char* numStr = m_ResponseBuffer + 15;
            for (uint8_t i = 0 ; i < 6 ; ++i)
            {
                uint16_t addressByte;
                numStr = ParseInt(numStr, addressByte, 16);
                m_StationMAC[5-i] = addressByte; // strtoul(numStr, &numStr, 16);
                numStr++; // Skip ':'
            }
        }            
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool ESP8266::ConnectToAP(const char* ssid, const char* password)
{
    if (m_StatusFlags & e_StatusConnectedToAP){
        DisconnectFromAP();
    }        
    
    if ( strlen(ssid) >= sizeof(m_AccessPoint) ) {
        ESP8266_LOG_ERROR(PSTRL("ERROR: ConnectToAp() ssid to long"));
        return false;
    }
    if ( strlen(password) >= sizeof(m_Password) ) {
        ESP8266_LOG_ERROR(PSTRL("ERROR: ConnectToAp() password to long"));
        return false;
    }
    SetWifiMode((m_StatusFlags & e_StatusHotspotEnabled) ? WifiMode_e::e_SoftAPAndStation : WifiMode_e::e_Station);
    
    strncpy(m_AccessPoint, ssid, WIFI_MAX_SSID_LEN);
    strncpy(m_Password, password, WIFI_MAX_PASSWD_LEN);
    
    ESP8266_LOG_ERROR(PSTRL("AT+CWJAP_CUR=\""), ssid, PSTRL("\",\""), password, PSTRL("\""));
//    SendCommand(PSTRL("AT+CWJAP_CUR=\"%.32s\",\"%.64s\"\r\n"), ssid, password);
    SendCommand(PSTRL("AT+CWJAP_CUR=\""), ssid, PSTRL("\",\""), password, PSTRL("\"")); // FIXME!! MIGHT NOT BE NULL TERMINATED!
    m_StatusFlags |= e_StatusConnectingToAP;

    return true;
}    

///////////////////////////////////////////////////////////////////////////////
/// Return -1 if connection failed, 1 if it succeeded, and 0 if not done.
///////////////////////////////////////////////////////////////////////////////

int8_t ESP8266::PollConnectToAP()
{
    if (!IsSerialDataAvailable()) {
        return 0;
    }
    if (ReadResponseLine(10000, true))
    {
//        ESP8266_LOG_CRITICAL(m_ResponseBuffer);
        if (strcmp(m_ResponseBuffer,"OK") == 0) {
            m_StatusFlags = (m_StatusFlags & U8(~e_StatusConnectingToAP)) | e_StatusConnectedToAP;
//            ESP8266_LOG_CRITICAL(m_ResponseBuffer);
            return 1;
        } else if (strcmp(m_ResponseBuffer, "FAIL") == 0) {
            m_StatusFlags &= U8(~e_StatusConnectingToAP);
            return -1;
        } else {
//            ESP8266_LOG_CRITICAL(m_ResponseBuffer);
            return 0;
        }
    }
    else
    {
        m_StatusFlags &= U8(~e_StatusConnectingToAP);
        return -1;
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool ESP8266::DisconnectFromAP()
{
    if (m_StatusFlags & e_StatusConnectedToAP)
    {
        if (m_StatusFlags & e_StatusServerRunning) {
            StopServer();
        }            
        m_StatusFlags &= ~e_StatusConnectedToAP;
    
        if ( ExecuteCommand(PSTRL("AT+CWQAP")) ) {
            ESP8266_LOG_ERROR(PSTRL("Disconnected from AP."));
            return true;
        } else {
            ESP8266_LOG_ERROR(PSTRL("ERROR: Failed to disconnect from AP."));
            return false;
        }
//???        SetWifiMode((m_StatusFlags & e_StatusHotspotEnabled) ? WifiMode_e::e_SoftAP : WifiMode_e::e_Station);
    }
    return true;
}        

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

//#ifndef ESP8266_DISABLE_HOTSPOT
bool ESP8266::SetupHotspot( bool enabled, const char* SSID, const char* password, int8_t channel, WifiSecurityMode_e::Enum security, int8_t maxConnections, bool hidden )
{
    if (enabled)
    {
        SetWifiMode((m_StatusFlags & (e_StatusConnectingToAP | e_StatusConnectedToAP)) ? WifiMode_e::e_SoftAPAndStation : WifiMode_e::e_SoftAP);
//        RestartRadio();
//        SetBaudrate();
        //ESP8266_LOG_ERROR(PSTRL("AT+CWSAP_CUR=\"%s\",\"%s\",%d,%d,%d,%d\n"), SSID, password, channel, security, maxConnections, hidden);
        //ExecuteCommand(PSTRL("AT+CWSAP_CUR=\"%s\",\"%s\",%d,%d,%d,%d\r\n"), SSID, password, channel, security, maxConnections, hidden);
        
        if ( ExecuteCommand(PSTRL("AT+CWSAP_CUR=\""), SSID, PSTRL("\",\""), password, PSTRL("\","), channel, ",", security, ",", maxConnections) ) {
            ESP8266_LOG_ERROR(PSTRL("Hotspot started."));
            m_StatusFlags |= e_StatusHotspotEnabled;
            
#ifndef ESP8266_LEAN
            UpdateHotspotAddress();
#endif // ESP8266_LEAN
#define MAKEIP(a, b, c, d) (U32(a)<<24 | U32(b)<<16 | U32(c)<<8 | U32(d))

//            SetHotspotAddress(MAKEIP(192, 168, 4, 2), MAKEIP(192, 168, 1, 1), MAKEIP(255, 255, 255, 0));
            return true;
        } else {
            ESP8266_LOG_ERROR(PSTRL("ERROR: Failed to setup hotspot."));
            return false;
        }
    }
    else
    {
        m_StatusFlags &= U8(~e_StatusHotspotEnabled);
        SetWifiMode(WifiMode_e::e_Station);
        return true;
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool ESP8266::SetHotspotAddress(uint32_t address, uint32_t gateway, uint32_t netmask)
{
//    if (ExecuteCommand(PSTRL("AT+CIPAP_CUR=\"%d.%d.%d.%d\",\"%d.%d.%d.%d\",\"%d.%d.%d.%d\"\r\n"), U8(address >> 24), U8(address >> 16), U8(address >> 8), U8(address >> 24)
//                                                                                                , U8(gateway >> 24), U8(gateway >> 16), U8(gateway >> 8), U8(gateway >> 24)
//                                                                                                , U8(netmask >> 24), U8(netmask >> 16), U8(netmask >> 8), U8(netmask >> 24)))
    if (ExecuteCommand(PSTRL("AT+CIPAP_CUR=\""), U8(address >> 24), ".", U8(address >> 16), ".", U8(address >> 8), ".", U8(address >> 24), PSTRL("\",\"")
                                               , U8(gateway >> 24), ".", U8(gateway >> 16), ".", U8(gateway >> 8), ".", U8(gateway >> 24), PSTRL("\",\"")
                                               , U8(netmask >> 24), ".", U8(netmask >> 16), ".", U8(netmask >> 8), ".", U8(netmask >> 24), PSTRL("\"")))
    {        
        ESP8266_LOG_ERROR(PSTRL("Hotspot address set."));
        m_StatusFlags |= e_StatusHotspotEnabled;
        m_HotspotAddress = address;
        return true;
    }
    else
    {
        ESP8266_LOG_ERROR(PSTRL("ERROR: Failed to set hotspot address."));
        return false;
    }
    
}
//#endif // ESP8266_DISABLE_HOTSPOT


///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////
#ifndef ESP8266_LEAN
bool ESP8266::UpdateHotspotAddress()
{
    SendCommand(PSTRL("AT+CIPAP_CUR?"));
    
    bool addressFound = false;
    
    m_HotspotAddress = 0;
    m_HotspotGateway = 0;
    m_HotspotNetMask = 0;
    
    for(;;)
    {
        if (!ReadResponseLine(1000, false)) {
            ESP8266_LOG_ERROR(PSTRL("ERROR: ESP8266::UpdateHotspotAddress() failed to read command response."));
            return false;
        }
        if (strcmp(m_ResponseBuffer, "OK") == 0) {
            return addressFound;
        }
        if (strcmp(m_ResponseBuffer, "ERROR") == 0) {
            ESP8266_LOG_ERROR(PSTRL("ERROR: ESP8266::UpdateHotspotAddress() AT+CIPAP failed."));
            return false;
        }
        if (PgmUtils::strncmp(m_ResponseBuffer, PSTRL("+CIPAP_CUR:ip:\""), 15) == 0)
        {
            ESP8266_LOG_ERROR(PSTRL("Hotspot IP: "), m_ResponseBuffer + 14);
            m_HotspotAddress = ParseIPAddress(m_ResponseBuffer + 15);
            addressFound = true;
        }
        else if (PgmUtils::strncmp(m_ResponseBuffer, PSTRL("+CIPAP_CUR:gateway:\""), 20) == 0)
        {
            ESP8266_LOG_ERROR(PSTRL("Hotspot GW: "), m_ResponseBuffer + 19);
            m_HotspotGateway = ParseIPAddress(m_ResponseBuffer + 20);
        }
        else if (PgmUtils::strncmp(m_ResponseBuffer, PSTRL("+CIPAP_CUR:netmask:\""), 20) == 0)
        {
            ESP8266_LOG_ERROR(PSTRL("Hotspot NM: "), m_ResponseBuffer + 19);
            m_HotspotNetMask = ParseIPAddress(m_ResponseBuffer + 20);
        }
        else
        {
            ESP8266_LOG_ERROR(PSTRL("UpdateHotspotAddress(): unknown response "), m_ResponseBuffer);            
        }
    }    
}
#endif // ESP8266_LEAN

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool ESP8266::StartServer(uint16_t port)
{
    if (!(m_StatusFlags & (e_StatusConnectedToAP | e_StatusHotspotEnabled))) {
        DB_LOG(DP_ERROR, PSTRL("ERROR: Attempt to start server without being connected to AP."));
        return false;
    }
    if (m_StatusFlags & e_StatusServerRunning) {
        if (!StopServer()) {
            return false;
        }
    }
    
    DB_LOG(DP_INFO, PSTRL("AT+CIPSERVER=1,"), port);
    if ( ExecuteCommand(PSTRL("AT+CIPSERVER=1,"), port) ) {
        DB_LOG(DP_INFO, PSTRL("Server started."));
        return true;
    } else {
        DB_LOG(DP_ERROR, PSTRL("ERROR: Failed to start server."));
        return false;
    }    
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool ESP8266::StopServer()
{
    if (m_StatusFlags & e_StatusServerRunning)
    {
        if ( ExecuteCommand(PSTRL("AT+CIPSERVER=0")) ) {
            DB_LOG(DP_INFO, PSTRL("Server stopped."));
            m_StatusFlags &= U8(~e_StatusServerRunning);
#ifndef ESP8266_DONT_TRACK_CONNECTIONS
            m_ConnectedLinks = 0;
#endif // ESP8266_DONT_TRACK_CONNECTIONS
            return true;
        } else {
            DB_LOG(DP_ERROR, PSTRL("ERROR: Failed to stop server."));
            return false;
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////
/*
int8_t ESP8266::CanSendIPData(uint8_t linkID)
{
    //ESP8266_LOG_ERROR(PSTRL("AT+CWLAP="), m_AccessPoint);
    //SendCommand(PSTRL("AT+CWLAP=\"%s\",\"cc:5d:4e:06:24:ac\"\r\n"), m_AccessPoint);
    SendCommand(PSTRL("AT+CIPBUFSTATUS=n"), linkID);
    
    char responseBuffer[128];
    if ( ReadCommandResponse(5000, responseBuffer, sizeof(responseBuffer)) )
    {
//        ESP8266_LOG_ERROR(PSTRL("CanSendIPData: "), responseBuffer);
        
        uint8_t len = strlen(responseBuffer);
        return PgmUtils::strcmp(responseBuffer + len - 2, PSTRL(",0")) != 0;
    } else {
        ESP8266_LOG_ERROR(PSTRL("ERROR: CanSendIPData() Failed to get status."));
    }
    return -1;
}*/

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool ESP8266::SendIPData(uint8_t linkID, void* data, int16_t length)
{
    static bool isSending = false;
    
    if (isSending) {
        ESP8266_LOG_ERROR(PSTRL("ERROR: SendIPData() called recursively."));
    }
    isSending = true;

#ifndef ESP8266_DONT_TRACK_CONNECTIONS
    if (!(m_ConnectedLinks & BIT8(linkID, 1))) {
        ESP8266_LOG_ERROR(PSTRL("ERROR: Attempt to send data on closed link "), linkID);
        isSending = false;
        return false;
    }
#endif // ESP8266_DONT_TRACK_CONNECTIONS
    
/*    for(;;)
    {
        int8_t canSend = CanSendIPData(linkID);
        if (canSend > 0) {
            break;
        } else if (canSend < 0) {
        isSending = false;
            return false;
        }
        ESP8266_LOG_ERROR(PSTRL("POLL SEND"));
    }*/
    
    //ESP8266_LOG_ERROR(PSTRL("AT+CIPSEND=%u,%u\n"), linkID, length);

    if ( ExecuteCommand(PSTRL("AT+CIPSENDBUF="), linkID, ",", length) )
    {
        for (;;)
        {
            char c;
            if (!ReadChar(&c, 500)) {
                ESP8266_LOG_ERROR(PSTRL("ERROR: SendIP Failed to read ESP8266 send prompt!"));
                break;
            }
            if (c == '>') {
                break;
            }
        }            
                    
        for (int16_t i = 0 ; i < length ; ++i)
        {
            // Wait for the transmit buffer to be empty
            while ( !( WIFI_USART.STATUS & USART_DREIF_bm) );
            // Put our character into the transmit buffer
            WIFI_USART.DATA = reinterpret_cast<const uint8_t*>(data)[i];
        }
        //ESP8266_LOG_ERROR(PSTRL("*"));
        for(;;)
        {
            if (ReadResponseLine(5000, false))
            {
                if (m_ResponseDataLength>=6 && PgmUtils::strncmp(m_ResponseBuffer + m_ResponseDataLength - 6, PSTRL(" bytes"), 6) == 0) {
                    break;
                }
            }
            else
            {
                ESP8266_LOG_ERROR(PSTRL("ERROR: SendIP Failed to read ESP8266 response!"));
                isSending = false;
                return false;
            }
        }            
    }
    else
    {
        ESP8266_LOG_ERROR(PSTRL("ERROR: SendIPData(%d) init failed."), linkID);
    }
    isSending = false;
    return false;        
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool ESP8266::CloseConnection(uint8_t linkID)
{
#ifndef ESP8266_DONT_TRACK_CONNECTIONS
    if (!(m_ConnectedLinks & BIT8(linkID, 1))) {
        return true;
    }
#endif // ESP8266_DONT_TRACK_CONNECTIONS
   
    if ( ExecuteCommand(PSTRL("AT+CIPCLOSE="), linkID) ) {
        ESP8266_LOG_ERROR(PSTRL("Connection closed: "), linkID);
#ifndef ESP8266_DONT_TRACK_CONNECTIONS
        m_ConnectedLinks &= U8(~BIT8(linkID, 1));
#endif // ESP8266_DONT_TRACK_CONNECTIONS
        return true;
    } else {
        ESP8266_LOG_ERROR(PSTRL("ERROR: Failed to close connection."));
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////
#ifndef ESP8266_LEAN
uint8_t ESP8266::GetSignalPercentage(int16_t attenuation)
{
    int16_t signalStrength;
            
    if ( attenuation < -100 ) {
        signalStrength = 0;
    } else if (attenuation > -50) {
        signalStrength = 100;
    } else {
        signalStrength = 100 + (50 + attenuation) * 2;
    }
    return signalStrength;
}
#endif // ESP8266_LEAN

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////
/*
int ESP8266::PutChar(char c, FILE *stream)
{
    if (c == '\n')
    PutChar('\r', stream);
    
    // Wait for the transmit buffer to be empty
    while ( !( WIFI_USART.STATUS & USART_DREIF_bm) );
    // Put our character into the transmit buffer
    WIFI_USART.DATA = c;
    
    return 0;
}

int ESP8266::PutChar(char c)
{
    // Wait for the transmit buffer to be empty
    while ( !( WIFI_USART.STATUS & USART_DREIF_bm) );
    // Put our character into the transmit buffer
    WIFI_USART.DATA = c;    
    return 0;
}*/

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void ESP8266::WriteRaw(const char* string, int16_t size)
{
    for (int16_t i = 0 ; i < size ; ++i)
    {
        // Wait for the transmit buffer to be empty
        while ( !( WIFI_USART.STATUS & USART_DREIF_bm) );
        // Put our character into the transmit buffer
        WIFI_USART.DATA = string[i];
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool ESP8266::ReadChar(char* buffer, uint16_t timeout)
{
    uint32_t startTime = 0;
        
    if ( timeout != 0 ) startTime = Clock::GetTime();
    do
    {
        if (IsSerialDataAvailable())
        {
            *buffer = g_SerialReceiveBuffer[g_SerialReceiveBufferOutPos++];
            if (g_SerialReceiveBufferOutPos >= WIFI_SERIAL_IN_BUFFER_SIZE) g_SerialReceiveBufferOutPos = 0;
            return true;
        }
    } while(timeout != 0 && (Clock::GetTime() - startTime) < timeout);
    ESP8266_LOG_ERROR(PSTRL("TIMEOUT!!"));
    if (timeout != 0) {
        g_ReceiveTimedOut = true;
    }        
    return false;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

int16_t ESP8266::Read(void* buffer, int16_t size, uint16_t timeout)
{
    char* data = reinterpret_cast<char*>(buffer);
    for (int16_t i = 0 ; i < size ; ++i)
    {
        if (!ReadChar(data++, timeout)) {
            ESP8266_LOG_ERROR(PSTRL("ERROR: ESP8266::Read() timed out!"));
            m_LinkDataReceived -= size - i;
            return size - i;
        }
    }
    m_LinkDataReceived -= size;
    return size;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

const void* ESP8266::GetReceiveBuffer(int16_t size, int16_t* bytesAvailable, uint16_t timeout)
{
    //ESP8266_LOG_ERROR(PSTRL("Receive %d of %d bytes\n"), size, m_LinkDataReceived);
    if (g_ReceiveTimedOut) {
        ESP8266_LOG_ERROR(PSTRL("ERROR: ESP8266::GetReceiveBuffer() WIFI USART buffer timeout!"));
        return nullptr;
    }
/*    if (g_SerialReceiveBufferHasOverflowed) {
        ESP8266_LOG_ERROR(PSTRL("ERROR: ESP8266::GetReceiveBuffer() WIFI USART buffer overflow!"));
        return nullptr;
    }*/
    int16_t availableBytes = GetAvailableSerialBytes();
    if (availableBytes >= size)
    {
        *bytesAvailable = min(WIFI_SERIAL_IN_BUFFER_SIZE - g_SerialReceiveBufferOutPos, size);
        return const_cast<const char*>(g_SerialReceiveBuffer + g_SerialReceiveBufferOutPos);
    }
    else if ( timeout != 0 )
    {
        uint32_t startTime = Clock::GetTime();
        do
        {
            int16_t availableBytes = GetAvailableSerialBytes();
            //ESP8266_LOG_ERROR(PSTRL("%d/%d\n"), availableBytes, size);
            if (availableBytes >= size)
            {
                *bytesAvailable = min(availableBytes, min(size, WIFI_SERIAL_IN_BUFFER_SIZE - g_SerialReceiveBufferOutPos));
                return const_cast<const char*>(g_SerialReceiveBuffer + g_SerialReceiveBufferOutPos);
            }
        } while(Clock::GetTime() - startTime < timeout);
        ESP8266_LOG_ERROR(PSTRL("ESP8266::GetReceiveBuffer() TIMEOUT!!"));
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void ESP8266::ConsumeReceiveBytes(int16_t size)
{
    int16_t availableBytes = GetAvailableSerialBytes();
    if (size > availableBytes) {
        ESP8266_LOG_ERROR(PSTRL("ERROR: ConsumeReceiveBytes() attempt to consume "), size, PSTRL(" bytes. Only "), availableBytes, PSTRL(" available."));
        size = availableBytes;
    }
    g_SerialReceiveBufferOutPos += size; // (g_SerialReceiveBufferOutPos + size) & (WIFI_SERIAL_IN_BUFFER_SIZE - 1);
    if (g_SerialReceiveBufferOutPos >= WIFI_SERIAL_IN_BUFFER_SIZE) g_SerialReceiveBufferOutPos -= WIFI_SERIAL_IN_BUFFER_SIZE;
    m_LinkDataReceived -= size;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void ESP8266::ProcessIPData()
{
    //ESP8266_LOG_ERROR(PSTRL("#IP<"));
    //ESP8266_LOG_ERROR(PSTRL("ProcessIPData: "), m_ResponseBuffer);
    // "+IPD" is already read;
    m_ResponseDataLength = 0;
    for(;;)
    {
        char c;
        if (!ReadChar(&c, 1000))
        {
            //ESP8266_LOG_ERROR(PSTRL("PIP2"));
            m_ResponseBuffer[m_ResponseDataLength] = 0;
            ESP8266_LOG_ERROR(PSTRL("ERROR: Timeout while reading +IPD header: "), m_ResponseBuffer);
            m_ResponseDataLength = 0;
            return;
        }
        //ESP8266_LOG_ERROR((c) ? c : '?');
        if (c == ':')
        {
            m_ResponseBuffer[m_ResponseDataLength] = 0;
            m_ResponseDataLength = 0;
            //ESP8266_LOG_ERROR(PSTRL("ProcessIPData 2: "), m_ResponseBuffer);

            uint8_t linkID;
            uint16_t dataSize;
            char* str = m_ResponseBuffer;
            if (*str == ',')
            {
                linkID = str[1] - '0'; str += 2; // strtol(str + 1, &str, 10);
                if (*str == ',')
                {
                    ParseInt(str + 1, dataSize);
//                    dataSize = strtol(str + 1, nullptr, 10);
                    //ESP8266_LOG_ERROR(PSTRL("ProcessIPData: \""), m_ResponseBuffer, PSTRL("\", "), linkID, ", " , dataSize);
                    if (dataSize > 2048) {
                        ESP8266_LOG_ERROR(PSTRL("ERROR: Received to large +IPD segment: "), dataSize);
                        return;
                    }

                    m_LinkDataReceived = dataSize;
                    m_TotalDataReceived[linkID] += dataSize;
                    //ESP8266_LOG_ERROR(PSTRL("Received %d bytes on %d:\n"), dataSize, linkID);
                    ESP8266_DataReceivedCallback(linkID, m_LinkDataReceived);
                    //ESP8266_LOG_ERROR(PSTRL("DONE"));
                    if ( m_LinkDataReceived != 0 ) {
                        ESP8266_LOG_ERROR(PSTRL("ERROR: Wrong number of bytes read by socket: "), m_LinkDataReceived);
                    }
                    //ESP8266_LOG_ERROR(PSTRL("Done processing"));
                    return;
                }
            }                
//                m_ResponseBuffer[m_ResponseDataLength] = 0;
            ESP8266_LOG_ERROR(PSTRL("ERROR: Failed to parse +IPD header: "), m_ResponseBuffer);
            m_ResponseDataLength = 0;
            return;
        }
        else
        {
            if (m_ResponseDataLength < RESPONSE_BUFFER_SIZE - 1)
            {
                m_ResponseBuffer[m_ResponseDataLength++] = c;
            }
            else
            {
                ESP8266_LOG_ERROR(PSTRL("ERROR: Response buffer overflow while processing +IPD!"));
                m_ResponseDataLength = 0;
                return;
            }
        }
    }        
    //ESP8266_LOG_ERROR(PSTRL("#IP>"));
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////
#if 1
void CalculateBaudrateParams(uint32_t baudrate, int8_t& bscale, uint16_t& bsel)
{
    bscale = 0;
    bsel = 0;
    int32_t smallestOffset = 10000000;

    int8_t clockShifter = 3;

    // Check negative bscale values:
    for (int8_t bscaleCurNeg = 1 ; bscaleCurNeg <= 7 ; ++bscaleCurNeg)
    {
        int8_t fShift = bscaleCurNeg - clockShifter;
        uint32_t f = (fShift >= 0) ? (CPU_FREQ << fShift) : (CPU_FREQ >> (-fShift));
        uint16_t bselCur = DIV_ROUND(f, baudrate) - (1 << bscaleCurNeg);
        if (bselCur <= 4095)
        {
            //            baudrateF = (CPU_FREQ/clockScale) / (scaleP2 * bsel + 1.0);
            //            baudrateF = (CPU_FREQ/clockScale/scaleP2) / (bsel + 1.0*scaleP2);
            //            baudrateF = (CPU_FREQ/clockScale/scaleP2) / (bsel + scaleP2);
            int16_t divider = bselCur + (1 << bscaleCurNeg);
            uint32_t realBaudrate = DIV_ROUND(f, divider);

            int32_t offset = realBaudrate - baudrate;
            if (offset < 0) offset = -offset;
            if (offset < smallestOffset)
            {
                bscale = -bscaleCurNeg;
                bsel = bselCur;
                smallestOffset = offset;
            }
        }
    }
    // Check positive bscale values:
    for (int8_t bscaleCur = 0 ; bscaleCur <= 7 ; ++bscaleCur)
    {
        uint32_t divider = baudrate << (bscaleCur + clockShifter);
        uint16_t bselCur = DIV_ROUND(CPU_FREQ, divider) - 1;
        if (bselCur <= 4095)
        {
            divider = (bselCur + 1) << bscaleCur;
            uint32_t realBaudrate = ((CPU_FREQ >> clockShifter) + (divider >> 1)) / divider;

            int32_t offset = realBaudrate - baudrate;
            if (offset < 0) offset = -offset;
            if (offset < smallestOffset)
            {
                bscale = bscaleCur;
                bsel = bselCur;
                smallestOffset = offset;
            }
        }
    }
}
#endif

/*
void ESP8266::SendCommand( const PString& format, ... )
{
    va_list args;
    va_start(args, format);
    PgmUtils::FPrintf(args, &m_File, format);
    va_end(args);
}*/

/*bool ESP8266::ExecuteCommand( const PString& format, ... )
{
    va_list args;
    va_start(args, format);
    PgmUtils::FPrintf(args, &m_File, format);
    va_end(args);
    return ReadCommandResponse(1000);
}*/

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

#if 0
void ESP8266::SetUSARTBaudRate(uint32_t baudrate)
{
    int8_t bscale;
    uint16_t bsel;
    CalculateBaudrateParams(baudrate, bscale, bsel);
    ESP8266_LOG_ERROR(PSTRL("bscale: "), bscale, PSTRL(", bsel: "), bsel);
    WIFI_USART.BAUDCTRLB = ((bscale << USART_BSCALE_gp) & USART_BSCALE_gm) | (bsel >> 8); ///*USART_BSCALE3_bm | USART_BSCALE2_bm | */ USART_BSCALE1_bm | USART_BSCALE0_bm; // BSCALE = 3
    WIFI_USART.BAUDCTRLA = bsel;
    WIFI_USART.CTRLB = USART_TXEN_bm | USART_RXEN_bm | USART_CLK2X_bm;
}
#endif
void ESP8266::SetUSARTBaudRate(int8_t bscale, uint16_t bsel)
{
//    CalculateBaudrateParams(baudrate, bscale, bsel);
    WIFI_USART.BAUDCTRLB = ((bscale << USART_BSCALE_gp) & USART_BSCALE_gm) | (bsel >> 8); ///*USART_BSCALE3_bm | USART_BSCALE2_bm | */ USART_BSCALE1_bm | USART_BSCALE0_bm; // BSCALE = 3
    WIFI_USART.BAUDCTRLA = bsel;
    WIFI_USART.CTRLB = USART_TXEN_bm | USART_RXEN_bm | USART_CLK2X_bm;
}

void ESP8266::RadioWriter::Write(char c)
{
    //            ESP8266::PutChar(c);
    // Wait for the transmit buffer to be empty
    while ( !( WIFI_USART.STATUS & USART_DREIF_bm) );
    // Put our character into the transmit buffer
    WIFI_USART.DATA = c;
}
