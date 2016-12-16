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

#ifndef F_MISC_SPI_H__
#define F_MISC_SPI_H__


#include <avr/io.h>
#include <avr/wdt.h>

#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega168A__)

#define SPI_PRESCALER_DIV4_gc   0
#define SPI_PRESCALER_DIV16_gc  (1<<SPR0)
#define SPI_PRESCALER_DIV64_gc  (1<<SPR1)
#define SPI_PRESCALER_DIV128_gc ((1<<SPR1) | (1<<SPR0))

#define SPI_ENABLE_bm (1<<SPE)
#define SPI_DORD_bm   (1<<DORD)
#define SPI_MASTER_bm (1<<MSTR)

#define SPI_MODE0_bm (1<<CPHA)
#define SPI_MODE1_bm (1<<CPOL)

#define SPI_IF_bm    (1<<SPIF)
#define SPI_WRCOL_bm (1<<WCOL)

#define SPI_CLK2X_bm (1<<SPI2X)

#elif defined (__AVR_ATxmega128A3U__) || defined(__AVR_ATxmega256A3U__)
#else
#error
#endif

#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega168A__)

struct SPI
{
    enum Divider_e
    {
        e_Divder_2XFlag = 0x40,
        
        e_Divider_2 = e_Divder_2XFlag | SPI_PRESCALER_DIV4_gc,
        e_Divider_4 = SPI_PRESCALER_DIV4_gc,
        e_Divider_8 = e_Divder_2XFlag | SPI_PRESCALER_DIV16_gc,
        e_Divider_16 = SPI_PRESCALER_DIV16_gc,
        e_Divider_32 = e_Divder_2XFlag | SPI_PRESCALER_DIV64_gc,
        e_Divider_64 = SPI_PRESCALER_DIV64_gc,
        e_Divider_128 = SPI_PRESCALER_DIV128_gc
    };
    enum ClockMode_e
    {
        e_ClockMode0 = 0,
        e_ClockMode1 = SPI_MODE0_bm,
        e_ClockMode2 = SPI_MODE1_bm,
        e_ClockMode3 = SPI_MODE1_bm | SPI_MODE0_bm
    };
    
    enum Mode_e { e_Master, e_Slave };
    enum DataOrder_e { e_MSB, e_LSB };

    SPI() {}
    void Init(Divider_e divider, ClockMode_e clockMode = e_ClockMode0,  Mode_e mode = e_Master, DataOrder_e dataOrder = e_MSB)
    {
        uint8_t t = clockMode | SPI_ENABLE_bm;
        
        t |= (divider & ~e_Divder_2XFlag);
        
        if ( mode == e_Master )   t |= SPI_MASTER_bm;
        if ( dataOrder == e_LSB ) t |= SPI_DORD_bm;

        SPCR = t;
        if ( divider & e_Divder_2XFlag )
        {
            SPSR |= SPI_CLK2X_bm;
        }
    }

    void Enable(bool enable) { if (enable) { SPCR |= SPI_ENABLE_bm; } else { SPCR &= ~SPI_ENABLE_bm; } }
    inline bool GetInterruptFlag() { return (SPSR & SPI_IF_bm) != 0; }
    inline bool GetWriteCollisionFlag() { return (SPSR & SPI_WRCOL_bm) != 0; }

    inline void Write(uint8_t data)
    {
        while(GetInterruptFlag()) { Read(); /*wdt_reset()*/; }
        SPDR = data;
    }
    inline uint8_t Read()
    {
        while(!GetInterruptFlag()) /*wdt_reset()*/;
        return SPDR;
    }
    
    uint8_t Transfer(uint8_t data)
    {
        Write(data);
        uint8_t result = Read();       
        return result;
    }
};

#elif defined (__AVR_ATxmega128A3U__) || defined(__AVR_ATxmega256A3U__)

#if 0
struct SPI
{
    enum Divider_e
    {
        e_Divder_2XFlag = SPI_CLK2X_bm,
        
        e_Divider_2 = e_Divder_2XFlag | SPI_PRESCALER_DIV4_gc,
        e_Divider_4 = SPI_PRESCALER_DIV4_gc,
        e_Divider_8 = e_Divder_2XFlag | SPI_PRESCALER_DIV16_gc,
        e_Divider_16 = SPI_PRESCALER_DIV16_gc,
        e_Divider_32 = e_Divder_2XFlag | SPI_PRESCALER_DIV64_gc,
        e_Divider_64 = SPI_PRESCALER_DIV64_gc,
        e_Divider_128 = SPI_PRESCALER_DIV128_gc
    };
    enum ClockMode_e
    {
        e_ClockMode0 = 0,
        e_ClockMode1 = SPI_MODE0_bm,
        e_ClockMode2 = SPI_MODE1_bm,
        e_ClockMode3 = SPI_MODE1_bm | SPI_MODE0_bm
    };
    
    enum Mode_e { e_Master, e_Slave };
    enum DataOrder_e { e_MSB, e_LSB };

    SPI(SPI_t& spiAddress) : m_SPI(&spiAddress) {}
    SPI(const SPI& spi) : m_SPI(spi.m_SPI) {}

    void Init(Divider_e divider, ClockMode_e clockMode = e_ClockMode0,  Mode_e mode = e_Master, DataOrder_e dataOrder = e_MSB)
    {
        uint8_t t = clockMode | SPI_ENABLE_bm;
        t |= divider;
        if ( mode == e_Master )   t |= SPI_MASTER_bm;
        if ( dataOrder == e_LSB ) t |= SPI_DORD_bm;
        m_SPI->CTRL = t;
    }

    inline void Enable(bool enable) { if (enable) { m_SPI->CTRL |= SPI_ENABLE_bm; } else { m_SPI->CTRL &= ~SPI_ENABLE_bm; } }
    inline void SetDivider(Divider_e divider) { m_SPI->CTRL = (m_SPI->CTRL & ~(SPI_CLK2X_bm |SPI_PRESCALER_gm)) | divider; }
    
    inline bool GetInterruptFlag() { return (m_SPI->STATUS & SPI_IF_bm) != 0; }
    inline bool GetWriteCollisionFlag() { return (m_SPI->STATUS & SPI_WRCOL_bm) != 0; }

    inline void Write(uint8_t data)
    {
        while(GetInterruptFlag()) { Read(); /*wdt_reset();*/ }
        m_SPI->DATA = data;
    }
    inline uint8_t Read()
    {
        while(!GetInterruptFlag())/* wdt_reset()*/;
        return m_SPI->DATA;
    }

    uint8_t Transfer(uint8_t data)
    {
        Write(data);
        uint8_t result = Read();
        return result;
    }
    SPI_t* m_SPI;
};

#else // USART SPI mode

struct SPI
{
    enum Divider_e
    {
        e_Divider_2 = 2/2-1,
        e_Divider_4 = 4/2-1,
        e_Divider_8 = 8/2-1,
        e_Divider_16 = 16/2-1,
        e_Divider_32 = 32/2-1,
        e_Divider_64 = 64/2-1,
        e_Divider_128 = 128/2-1
    };
    enum ClockMode_e
    {
        e_ClockMode0 = 0,
        e_ClockMode1 = 1,
        e_ClockMode2 = 2,
        e_ClockMode3 = 3
    };
    
    enum Mode_e { e_Master };
    enum DataOrder_e { e_MSB, e_LSB };

    SPI(USART_t& usartAddress) : m_USART(&usartAddress) {}
    SPI(const SPI& spi) : m_USART(spi.m_USART) {}

    void Init(Divider_e divider, ClockMode_e clockMode = e_ClockMode0,  Mode_e mode = e_Master, DataOrder_e dataOrder = e_MSB)
    {
        m_USART->CTRLA = 0; // No interrupts
        m_USART->CTRLB = USART_RXEN_bm | USART_TXEN_bm;
        m_USART->CTRLC = USART_CMODE_MSPI_gc | USART_PMODE_DISABLED_gc | ((dataOrder == e_MSB) ? 0 : (1<<2)) | ((clockMode & 0x01) ? (1<<1) : 0);
        // TODO: (clockMode & 0x02) Set clock invert
        m_USART->BAUDCTRLA = divider;
        m_USART->BAUDCTRLB = 0;        
    }

    inline void Enable(bool enable) { if (enable) { m_USART->CTRLB = USART_RXEN_bm | USART_TXEN_bm; } else { m_USART->CTRLB &= uint8_t(~(USART_RXEN_bm | USART_TXEN_bm)); } }
    inline void SetDivider(Divider_e divider) { m_USART->BAUDCTRLA = divider; }
    
    inline bool CanSend() { return (m_USART->STATUS & USART_DREIF_bm) != 0; }
    inline bool HasData() { return (m_USART->STATUS & USART_RXCIF_bm) != 0; }
    inline bool GetWriteCollisionFlag() { return false; }

    inline void Write(uint8_t data)
    {
        while(!CanSend());
        m_USART->DATA = data;
    }
    inline uint8_t Read()
    {
        while(!HasData());
        return m_USART->DATA;
    }

    inline uint8_t Transfer(uint8_t data)
    {
        Write(data);
        uint8_t result = Read();
        return result;
    }
    
    inline void StartBurst(uint8_t data)
    {
        while(!CanSend()); // Make sure FIFO is empty.
        m_USART->DATA = data; // Fill FIFO
        while(!CanSend());  // Make sure the first bytes has been clocked out so the first call to ReadBurst() don't clobber the FIFO.
    }
    inline uint8_t ReadBurst(uint8_t data)
    {
        m_USART->DATA = data;
        while(!CanSend());
//        while(!HasData());
        return m_USART->DATA;
    }
    inline uint8_t EndBurst()
    {
        while(!HasData());
        return m_USART->DATA;
    }
    
    USART_t* m_USART;
};

#endif // USART SPI mode


#endif

#endif // F_MISC_SPI_H__
