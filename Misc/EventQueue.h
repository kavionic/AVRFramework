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


#ifndef EVENTQUEUE_H_
#define EVENTQUEUE_H_


#include <stdio.h>
#include <util/atomic.h>

template<typename T, int QUEUE_SIZE> class EventQueue
{
public:
    static const uint8_t QUEUE_SIZE_MASK = QUEUE_SIZE - 1;

    EventQueue()
    {
        m_QueueInPos = -1;
        m_QueueOutPos = 0;
    }
    
    void AddEvent( const T& event )
    {
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
        {
            if ( m_QueueInPos != -1 )
            {
                if ( m_QueueInPos != m_QueueOutPos )
                {
                    m_Queue[m_QueueInPos] = event;
                    m_QueueInPos = (m_QueueInPos + 1) & QUEUE_SIZE_MASK;
                }
            }
            else
            {
                m_Queue[m_QueueOutPos] = event;
                m_QueueInPos = (m_QueueOutPos + 1) & QUEUE_SIZE_MASK;
            }
        }            
    }

    bool GetEvent( T* event )
    {
        bool result = false;
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
        {
            if ( m_QueueInPos != -1 )
            {
                *event = m_Queue[m_QueueOutPos];
                m_QueueOutPos = (m_QueueOutPos + 1) & QUEUE_SIZE_MASK;
                if ( m_QueueInPos == m_QueueOutPos )
                {
                    m_QueueInPos = -1; // Queue empty
                }
                result = true;
            }
        }
        return result;
    }

    T* PeekEvent()
    {
        return (m_QueueInPos != -1) ? &m_Queue[m_QueueOutPos] : nullptr;
    }

    T m_Queue[QUEUE_SIZE];
    volatile int8_t m_QueueInPos;
    volatile int8_t m_QueueOutPos;
        
};


#endif /* EVENTQUEUE_H_ */