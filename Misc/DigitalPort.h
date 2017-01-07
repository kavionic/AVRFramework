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

#ifndef F_MISC_DIGITALPORT_H__
#define F_MISC_DIGITALPORT_H__


#include <avr/io.h>
#include "Utils.h"
/*
enum DigitalPortID_e {
    e_DigitalPortID_A,
    e_DigitalPortID_B,
    e_DigitalPortID_C,
    e_DigitalPortID_D,
    e_DigitalPortID_E,
    e_DigitalPortID_F,
    e_DigitalPortID_R
};

template<uint8_t> struct APortAcessor {};

template<> struct APortAcessor<e_DigitalPortID_A> { PORT_t& GetPort() const { return PORTA; } };
template<> struct APortAcessor<e_DigitalPortID_B> { PORT_t& GetPort() const { return PORTB; } };
template<> struct APortAcessor<e_DigitalPortID_C> { PORT_t& GetPort() const { return PORTC; } };
template<> struct APortAcessor<e_DigitalPortID_D> { PORT_t& GetPort() const { return PORTD; } };
template<> struct APortAcessor<e_DigitalPortID_E> { PORT_t& GetPort() const { return PORTE; } };
template<> struct APortAcessor<e_DigitalPortID_F> { PORT_t& GetPort() const { return PORTF; } };
template<> struct APortAcessor<e_DigitalPortID_R> { PORT_t& GetPort() const { return PORTR; } };

struct PortAccessorDynamic
{
    PortAccessorDynamic(DigitalPortID_e portID)
    {
        switch(portID)
        {
            case e_DigitalPortID_A: m_Port = &PORTA; break;
            case e_DigitalPortID_B: m_Port = &PORTB; break;
            case e_DigitalPortID_C: m_Port = &PORTC; break;
            case e_DigitalPortID_D: m_Port = &PORTD; break;
            case e_DigitalPortID_E: m_Port = &PORTE; break;
            case e_DigitalPortID_F: m_Port = &PORTF; break;
            case e_DigitalPortID_R: m_Port = &PORTR; break;
        }            
    }    
    PortAccessorDynamic(PORT_t* port) : m_Port(port) {}
    PORT_t& GetPort() const { return *m_Port; }
    PORT_t* m_Port;    
};
*/

#if !defined(__AVR_XMEGA__)

#define e_DigitalPortID_B (&PINB)
#define e_DigitalPortID_C (&PINC)
#define e_DigitalPortID_D (&PIND)

typedef volatile uint8_t* DPortAddr_t;

#define DPORT_IN(port)  *(port)
#define DPORT_DDR(port)  *DPortAddr_t((port)+1)
#define DPORT_DATA(port) *DPortAddr_t((port)+2)

#else // __AVR_XMEGA__

#define e_DigitalPortID_A (&PORTA)
#define e_DigitalPortID_B (&PORTB)
#define e_DigitalPortID_C (&PORTC)
#define e_DigitalPortID_D (&PORTD)
#define e_DigitalPortID_E (&PORTE)
#define e_DigitalPortID_F (&PORTF)
#define e_DigitalPortID_R (&PORTR)

#endif

struct DigitalPort
{
#if !defined(__AVR_XMEGA__)
    DigitalPort(DPortAddr_t port) : m_Port(port) {}
#else // __AVR_XMEGA__
    DigitalPort(PORT_t* port) : m_Port(port) {}
#endif
        
#if !defined(__AVR_XMEGA__)
    static inline void SetDirection(DPortAddr_t port, uint8_t pins) { DPORT_DDR(port) = pins; }
    static inline void SetAsInput(DPortAddr_t port, uint8_t pins)   { DPORT_DDR(port) &= ~pins; }
    static inline void SetAsOutput(DPortAddr_t port, uint8_t pins)  { DPORT_DDR(port) |= pins; }

    static inline void Set(DPortAddr_t port, uint8_t pins) { DPORT_DATA(port) = pins; }
    static inline void SetSome(DPortAddr_t port, uint8_t mask, uint8_t pins) { DPORT_DATA(port) = (DPORT_DATA(port) & ~mask) | pins; }
    static inline void SetHigh(DPortAddr_t port, uint8_t pins) { DPORT_DATA(port) |= pins; }
    static inline void SetLow(DPortAddr_t port, uint8_t pins) { DPORT_DATA(port) &= ~pins; }
    static inline uint8_t Get(DPortAddr_t port) { return DPORT_IN(port); }

#else // __AVR_XMEGA__

    static inline void SetDirection(PORT_t* port, uint8_t pins) { port->DIR = pins; }
    static inline void SetAsInput(PORT_t* port, uint8_t pins)   { port->DIRCLR = pins; }
    static inline void SetAsOutput(PORT_t* port, uint8_t pins)  { port->DIRSET = pins; }

    static inline void Set(PORT_t* port, uint8_t pins) { port->OUT = pins; }
    static inline void SetSome(PORT_t* port, uint8_t mask, uint8_t pins) { port->OUT = (port->OUT & ~mask) | pins; }
    static inline void SetHigh(PORT_t* port, uint8_t pins) { port->OUTSET = pins; }
    static inline void SetLow(PORT_t* port, uint8_t pins) { port->OUTCLR = pins; }
    static inline uint8_t Get(PORT_t* port) { return port->IN; }

#endif

    inline void SetDirection(uint8_t pins) { SetDirection(m_Port, pins); }
    inline void SetAsInput(uint8_t pins)   { SetAsInput(m_Port, pins); }
    inline void SetAsOutput(uint8_t pins)  { SetAsOutput(m_Port, pins); }

    
#if !defined(__AVR_XMEGA__)
    static void EnablePullup(DPortAddr_t port, uint8_t pins)
    {
        SetHigh(port, pins);
//        DPORT_DATA(port) = (DPORT_DATA(port) & ~pins) | pins;
    }
#else // __AVR_XMEGA__
    static void EnablePullup(PORT_t* port, uint8_t pins)
    {
        for ( uint8_t i = 0 ; i < 8 ; ++i )
        {
            if ( pins & 0x01 )
            {
                ((register8_t*)&port->PIN0CTRL)[i] = (((register8_t*)&port->PIN0CTRL)[i] & U8(~PORT_OPC_gm)) | PORT_OPC_PULLUP_gc;
                
            }
            pins >>= 1;
        }
    }    
    static void SetPinsOutputAndPullConfig(PORT_t* port, uint8_t pins, uint8_t config)
    {
        for ( uint8_t i = 0 ; i < 8 ; ++i )
        {
            if ( pins & 0x01 )
            {
                ((register8_t*)&port->PIN0CTRL)[i] = (((register8_t*)&port->PIN0CTRL)[i] & U8(~PORT_OPC_gm)) | config;
                
            }
            pins >>= 1;
        }
    }
    static void InvertPins(PORT_t* port, uint8_t pins)
    {
        for ( uint8_t i = 0 ; i < 8 ; ++i )
        {
            if ( pins & 0x01 )
            {
                ((register8_t*)&port->PIN0CTRL)[i] = (((register8_t*)&port->PIN0CTRL)[i] & U8(~PORT_INVEN_bm)) | PORT_INVEN_bm;
                
            }
            pins >>= 1;
        }
    }
    inline void InvertPins(uint8_t pins) { InvertPins(m_Port, pins); }
    inline void SetPinsOutputAndPullConfig(uint8_t pins, uint8_t config) { SetPinsOutputAndPullConfig(m_Port, pins, config); }
#endif
    inline void EnablePullup(uint8_t pins) { EnablePullup(m_Port, pins); }
        
    inline void Set(uint8_t pins)                      { Set(m_Port, pins); }

    inline void SetSome(uint8_t mask, uint8_t pins)                      { SetSome(m_Port, mask, pins); }

    inline void SetHigh(uint8_t pins)                      { SetHigh(m_Port, pins); }

    inline void SetLow(uint8_t pins)                      { SetLow(m_Port, pins); }

    inline uint8_t Get()  const                  { return Get(m_Port); }

#if !defined(__AVR_XMEGA__)
    DPortAddr_t m_Port;        
#else // __AVR_XMEGA__
    PORT_t* m_Port;
#endif
};
/*
template<int PORT_ID>
struct DigitalPort : DigitalPortBase< APortAcessor<PORT_ID> >
{
    DigitalPort() : DigitalPortBase< APortAcessor<PORT_ID> >(APortAcessor<PORT_ID>()) {}
};


struct DigitalPortDynamic : DigitalPortBase<PortAccessorDynamic>
{
    DigitalPortDynamic(DigitalPortID_e port) : DigitalPortBase<PortAccessorDynamic>(PortAccessorDynamic(port)) {}
    DigitalPortDynamic(const DigitalPortDynamic& port) : DigitalPortBase<PortAccessorDynamic>(PortAccessorDynamic(port.m_Accessor.m_Port)) {}

private:        
    DigitalPortDynamic& operator=(const DigitalPortDynamic&);
        
};
*/


#endif // F_MISC_DIGITALPORT_H__
