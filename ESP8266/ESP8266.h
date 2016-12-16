// This file is part of TeslaDriver.
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


#ifndef __ESP8266_H__
#define __ESP8266_H__

#include <stdio.h>


#include "Misc/PString.h"

#ifdef ESP8266_LEAN
#define ESP8266_DONT_TRACK_CONNECTIONS
#endif

namespace WifiMode_e
{
    enum Enum
    {
        e_Station          = 1,
        e_SoftAP           = 2,
        e_SoftAPAndStation = 3
    };
}

namespace WifiMuxMode_e
{
    enum Enum
    {
        e_Single   = 0,
        e_Multiple = 1
    };
}

namespace WifiSecurityMode_e
{
    enum Enum
    {
        e_OPEN          = 0,
        e_WEP           = 1,
        e_WPA_PSK       = 2,
        e_WPA2_PSK      = 3,
        e_WPA_WPA2_PSK  = 4,
        e_Count
    };
}

static const int8_t WIFI_MAX_SSID_LEN   = 32;
static const int8_t WIFI_MAX_PASSWD_LEN = 64;

struct WifiAccessPoint
{
    char                     m_SSID[WIFI_MAX_SSID_LEN];
    WifiSecurityMode_e::Enum m_Security;
    int16_t                  m_Strength;
};

struct WifiStoredAccessPoint
{
    char                     m_SSID[WIFI_MAX_SSID_LEN];
    char                     m_Password[WIFI_MAX_PASSWD_LEN];
    uint8_t                  m_SequenceNum;
};

static const int16_t WIFI_NETWORK_BUFFER_SIZE = 2048;
static const int16_t WIFI_SERIAL_IN_BUFFER_SIZE = WIFI_NETWORK_BUFFER_SIZE + 128;


class ESP8266
{
public:
    enum StatusFlags_e
    {
        e_StatusConnectingToAP = 0x01,
        e_StatusConnectedToAP  = 0x02,
        e_StatusHotspotEnabled = 0x04,
        e_StatusServerRunning  = 0x08
    };

    static const uint8_t WIFI_MAX_LINKS=5;
    
    void Initialize(bool wifiBridge);
    
    bool Run();

    uint8_t        GetStatusFlags() const { return m_StatusFlags; }
#ifndef ESP8266_LEAN
    uint32_t       GetHotspotAddress() const { return m_HotspotAddress; }
    uint32_t       GetHotspotGateway() const { return m_HotspotGateway; }
    uint32_t       GetHotspotNetMask() const { return m_HotspotNetMask; }
    uint32_t       GetStationAddress() const { return m_StationAddress; }
    const uint8_t* GetStationMAC() const     { return m_StationMAC; }
#endif  // ESP8266_LEAN
    const char*    GetConnectedAP() const { return m_AccessPoint; }
    void           ResetRadio();
    bool           RestartRadio();
    bool           SetBaudrate();
    bool           GetModuleVersion();
    bool           PokeRadio();
    bool           SetWifiMode(WifiMode_e::Enum mode);
    void           EnableEcho(bool doEcho);
    void           SetMuxMode(WifiMuxMode_e::Enum mode);
    bool           ListAPs();
    int8_t         DiscoverAccessPoints(WifiAccessPoint* apList, int8_t listSize);
//    bool           ReadAccessPoint(WifiAccessPoint* accessPoint);
    bool           ConnectToAP(const char* ssid, const char* password);
    int8_t         PollConnectToAP();
    bool           DisconnectFromAP();
    
//#ifndef ESP8266_DISABLE_HOTSPOT    
    bool           SetupHotspot(bool enabled, const char* SSID, const char* password, int8_t channel, WifiSecurityMode_e::Enum security, int8_t maxConnections, bool hidden);
    bool           SetHotspotAddress(uint32_t address, uint32_t gateway, uint32_t netmask);
//#endif // ESP8266_DISABLE_HOTSPOT
    
    
#ifndef ESP8266_LEAN
    bool           UpdateHotspotAddress();
#endif // ESP8266_LEAN

    int16_t        GetSignalStrength();
    bool           UpdateStationAddress();
    bool           StartServer(uint16_t port);
    bool           StopServer();

    uint16_t       GetAndClearChannelBytesReceived(int8_t linkID) { uint16_t result = m_TotalDataReceived[linkID]; m_TotalDataReceived[linkID] = 0; return result;}
    int8_t         CanSendIPData(uint8_t linkID);

    bool           SendIPData(uint8_t linkID, void* data, int16_t length);
    bool           CloseConnection(uint8_t linkID);
    
    void           WriteRaw(const char* string, int16_t size);

    bool           ReadChar(char* buffer, uint16_t timeout);
    int16_t        Read(void* buffer, int16_t size, uint16_t timeout = 1000);
    const void*    GetReceiveBuffer(int16_t size, int16_t* bytesAvailable, uint16_t timeout = 10000);
    void           ConsumeReceiveBytes(int16_t size);
    
    bool           ReadCommandResponse(uint16_t timeout, bool logResponse = false);
    bool           ReadResponseLine(uint16_t timeout, bool breakOnIPData);

    const char*    GetResponseLine() const;
    
//    void           RegisterSocket(uint8_t linkID, WifiSocket* socket);
	
#ifndef ESP8266_LEAN
    static uint8_t GetSignalPercentage(int16_t attenuation);
#endif // ESP8266_LEAN

private:
    static const uint8_t RESPONSE_BUFFER_SIZE = 128;

    struct RadioWriter
    {
        static void Write(char c);
    };
    template<typename... ArgTypes>
    void SendCommand(ArgTypes... args)
    {
        StrFmt<RadioWriter>::Print(args...);
    }

    template<typename... ArgTypes>
    bool ExecuteCommand(ArgTypes... args) { SendCommand(args...); return ReadCommandResponse(10000, false); }

//    void SetUSARTBaudRate(uint32_t baudrate);
    void SetUSARTBaudRate(int8_t bscale, uint16_t bsel);

    bool ProcessOutOfBandData();
    void ProcessIPData();
    
    
    uint8_t m_StatusFlags = 0;
#ifndef ESP8266_DONT_TRACK_CONNECTIONS    
    uint8_t m_ConnectedLinks = 0;
#endif // ESP8266_DONT_TRACK_CONNECTIONS
    
//#ifndef ESP8266_LEAN
    uint32_t    m_StationAddress;
    uint8_t     m_StationMAC[6];
//#endif // ESP8266_LEAN

    uint32_t    m_HotspotAddress;
    uint32_t    m_HotspotGateway;
    uint32_t    m_HotspotNetMask;
    
    char        m_AccessPoint[WIFI_MAX_SSID_LEN + 1];
    char        m_Password[WIFI_MAX_PASSWD_LEN + 1];

    char        m_ResponseBuffer[RESPONSE_BUFFER_SIZE];
    uint8_t     m_ResponseDataLength;
    int16_t     m_LinkDataReceived = 0;
    int16_t     m_TotalDataReceived[WIFI_MAX_LINKS];

    // Disabled operators:
//    ESP8266(const ESP8266&);
//    ESP8266& operator=(const ESP8266 &);
};

// Implemented by user code to handle IP data.
extern void ESP8266_ConnectionChangedCallback(uint8_t linkID, bool isConnected);
extern void ESP8266_DataReceivedCallback(uint8_t linkID, int16_t size);
#endif //__ESP8266_H__
