/*
  ==============================================================================

   This file is part of the JUCE library - "Jules' Utility Class Extensions"
   Copyright 2004-11 by Raw Material Software Ltd.

  ------------------------------------------------------------------------------

   JUCE can be redistributed and/or modified under the terms of the GNU General
   Public License (Version 2), as published by the Free Software Foundation.
   A copy of the license is included in the JUCE distribution, or can be found
   online at www.gnu.org/licenses.

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.rawmaterialsoftware.com/juce for more information.

  ==============================================================================
*/

#include "../../core/juce_StandardHeader.h"

BEGIN_JUCE_NAMESPACE

#include "juce_MidiBuffer.h"


//==============================================================================
MidiBuffer::MidiBuffer() noexcept
    : bytesUsed (0)
{
}

MidiBuffer::MidiBuffer (const MidiMessage& message) noexcept
    : bytesUsed (0)
{
    addEvent (message, 0);
}

MidiBuffer::MidiBuffer (const MidiBuffer& other) noexcept
    : data (other.data),
      bytesUsed (other.bytesUsed)
{
}

MidiBuffer& MidiBuffer::operator= (const MidiBuffer& other) noexcept
{
    bytesUsed = other.bytesUsed;
    data = other.data;

    return *this;
}

void MidiBuffer::swapWith (MidiBuffer& other) noexcept
{
    data.swapWith (other.data);
    std::swap (bytesUsed, other.bytesUsed);
}

MidiBuffer::~MidiBuffer()
{
}

inline uint8* MidiBuffer::getData() const noexcept
{
    return static_cast <uint8*> (data.getData());
}

inline int MidiBuffer::getEventTime (const void* const d) noexcept
{
    return *static_cast <const int*> (d);
}

inline uint16 MidiBuffer::getEventDataSize (const void* const d) noexcept
{
    return *reinterpret_cast <const uint16*> (static_cast <const char*> (d) + sizeof (int));
}

inline uint16 MidiBuffer::getEventTotalSize (const void* const d) noexcept
{
    return getEventDataSize (d) + sizeof (int) + sizeof (uint16);
}

void MidiBuffer::clear() noexcept
{
    bytesUsed = 0;
}

void MidiBuffer::clear (const int startSample, const int numSamples)
{
    uint8* const start = findEventAfter (getData(), startSample - 1);
    uint8* const end   = findEventAfter (start, startSample + numSamples - 1);

    if (end > start)
    {
        const int bytesToMove = bytesUsed - (int) (end - getData());

        if (bytesToMove > 0)
            memmove (start, end, bytesToMove);

        bytesUsed -= (int) (end - start);
    }
}

void MidiBuffer::addEvent (const MidiMessage& m, const int sampleNumber)
{
    addEvent (m.getRawData(), m.getRawDataSize(), sampleNumber);
}

namespace MidiBufferHelpers
{
    int findActualEventLength (const uint8* const data, const int maxBytes) noexcept
    {
        unsigned int byte = (unsigned int) *data;
        int size = 0;

        if (byte == 0xf0 || byte == 0xf7)
        {
            const uint8* d = data + 1;

            while (d < data + maxBytes)
                if (*d++ == 0xf7)
                    break;

            size = (int) (d - data);
        }
        else if (byte == 0xff)
        {
            int n;
            const int bytesLeft = MidiMessage::readVariableLengthVal (data + 1, n);
            size = jmin (maxBytes, n + 2 + bytesLeft);
        }
        else if (byte >= 0x80)
        {
            size = jmin (maxBytes, MidiMessage::getMessageLengthFromFirstByte ((uint8) byte));
        }

        return size;
    }
}

void MidiBuffer::addEvent (const void* const newData, const int maxBytes, const int sampleNumber)
{
    const int numBytes = MidiBufferHelpers::findActualEventLength (static_cast <const uint8*> (newData), maxBytes);

    if (numBytes > 0)
    {
        int spaceNeeded = bytesUsed + numBytes + sizeof (int) + sizeof (uint16);
        data.ensureSize ((spaceNeeded + spaceNeeded / 2 + 8) & ~7);

        uint8* d = findEventAfter (getData(), sampleNumber);
        const int bytesToMove = bytesUsed - (int) (d - getData());

        if (bytesToMove > 0)
            memmove (d + numBytes + sizeof (int) + sizeof (uint16), d, bytesToMove);

        *reinterpret_cast <int*> (d) = sampleNumber;
        d += sizeof (int);
        *reinterpret_cast <uint16*> (d) = (uint16) numBytes;
        d += sizeof (uint16);

        memcpy (d, newData, numBytes);

        bytesUsed += numBytes + sizeof (int) + sizeof (uint16);
    }
}

void MidiBuffer::addEvents (const MidiBuffer& otherBuffer,
                            const int startSample,
                            const int numSamples,
                            const int sampleDeltaToAdd)
{
    Iterator i (otherBuffer);
    i.setNextSamplePosition (startSample);

    const uint8* eventData;
    int eventSize, position;

    while (i.getNextEvent (eventData, eventSize, position)
            && (position < startSample + numSamples || numSamples < 0))
    {
        addEvent (eventData, eventSize, position + sampleDeltaToAdd);
    }
}

void MidiBuffer::ensureSize (size_t minimumNumBytes)
{
    data.ensureSize (minimumNumBytes);
}

bool MidiBuffer::isEmpty() const noexcept
{
    return bytesUsed == 0;
}

int MidiBuffer::getNumEvents() const noexcept
{
    int n = 0;
    const uint8* d = getData();
    const uint8* const end = d + bytesUsed;

    while (d < end)
    {
        d += getEventTotalSize (d);
        ++n;
    }

    return n;
}

int MidiBuffer::getFirstEventTime() const noexcept
{
    return bytesUsed > 0 ? getEventTime (data.getData()) : 0;
}

int MidiBuffer::getLastEventTime() const noexcept
{
    if (bytesUsed == 0)
        return 0;

    const uint8* d = getData();
    const uint8* const endData = d + bytesUsed;

    for (;;)
    {
        const uint8* const nextOne = d + getEventTotalSize (d);

        if (nextOne >= endData)
            return getEventTime (d);

        d = nextOne;
    }
}

uint8* MidiBuffer::findEventAfter (uint8* d, const int samplePosition) const noexcept
{
    const uint8* const endData = getData() + bytesUsed;

    while (d < endData && getEventTime (d) <= samplePosition)
        d += getEventTotalSize (d);

    return d;
}

//==============================================================================
MidiBuffer::Iterator::Iterator (const MidiBuffer& buffer_) noexcept
    : buffer (buffer_),
      data (buffer_.getData())
{
}

MidiBuffer::Iterator::~Iterator() noexcept
{
}

//==============================================================================
void MidiBuffer::Iterator::setNextSamplePosition (const int samplePosition) noexcept
{
    data = buffer.getData();
    const uint8* dataEnd = data + buffer.bytesUsed;

    while (data < dataEnd && getEventTime (data) < samplePosition)
        data += getEventTotalSize (data);
}

bool MidiBuffer::Iterator::getNextEvent (const uint8* &midiData, int& numBytes, int& samplePosition) noexcept
{
    if (data >= buffer.getData() + buffer.bytesUsed)
        return false;

    samplePosition = getEventTime (data);
    numBytes = getEventDataSize (data);
    data += sizeof (int) + sizeof (uint16);
    midiData = data;
    data += numBytes;

    return true;
}

bool MidiBuffer::Iterator::getNextEvent (MidiMessage& result, int& samplePosition) noexcept
{
    if (data >= buffer.getData() + buffer.bytesUsed)
        return false;

    samplePosition = getEventTime (data);
    const int numBytes = getEventDataSize (data);
    data += sizeof (int) + sizeof (uint16);
    result = MidiMessage (data, numBytes, samplePosition);
    data += numBytes;

    return true;
}


END_JUCE_NAMESPACE
