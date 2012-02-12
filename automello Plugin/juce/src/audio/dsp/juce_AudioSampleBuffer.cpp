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

#include "juce_AudioSampleBuffer.h"
#include "../audio_file_formats/juce_AudioFormatReader.h"
#include "../audio_file_formats/juce_AudioFormatWriter.h"


//==============================================================================
AudioSampleBuffer::AudioSampleBuffer (const int numChannels_,
                                      const int numSamples) noexcept
  : numChannels (numChannels_),
    size (numSamples)
{
    jassert (numSamples >= 0);
    jassert (numChannels_ > 0);

    allocateData();
}

AudioSampleBuffer::AudioSampleBuffer (const AudioSampleBuffer& other) noexcept
  : numChannels (other.numChannels),
    size (other.size)
{
    allocateData();
    const size_t numBytes = size * sizeof (float);

    for (int i = 0; i < numChannels; ++i)
        memcpy (channels[i], other.channels[i], numBytes);
}

void AudioSampleBuffer::allocateData()
{
    const size_t channelListSize = (numChannels + 1) * sizeof (float*);
    allocatedBytes = (int) (numChannels * size * sizeof (float) + channelListSize + 32);
    allocatedData.malloc (allocatedBytes);
    channels = reinterpret_cast <float**> (allocatedData.getData());

    float* chan = (float*) (allocatedData + channelListSize);
    for (int i = 0; i < numChannels; ++i)
    {
        channels[i] = chan;
        chan += size;
    }

    channels [numChannels] = 0;
}

AudioSampleBuffer::AudioSampleBuffer (float** dataToReferTo,
                                      const int numChannels_,
                                      const int numSamples) noexcept
    : numChannels (numChannels_),
      size (numSamples),
      allocatedBytes (0)
{
    jassert (numChannels_ > 0);
    allocateChannels (dataToReferTo, 0);
}

AudioSampleBuffer::AudioSampleBuffer (float** dataToReferTo,
                                      const int numChannels_,
                                      const int startSample,
                                      const int numSamples) noexcept
    : numChannels (numChannels_),
      size (numSamples),
      allocatedBytes (0)
{
    jassert (numChannels_ > 0);
    allocateChannels (dataToReferTo, startSample);
}

void AudioSampleBuffer::setDataToReferTo (float** dataToReferTo,
                                          const int newNumChannels,
                                          const int newNumSamples) noexcept
{
    jassert (newNumChannels > 0);

    allocatedBytes = 0;
    allocatedData.free();

    numChannels = newNumChannels;
    size = newNumSamples;

    allocateChannels (dataToReferTo, 0);
}

void AudioSampleBuffer::allocateChannels (float** const dataToReferTo, int offset)
{
    // (try to avoid doing a malloc here, as that'll blow up things like Pro-Tools)
    if (numChannels < numElementsInArray (preallocatedChannelSpace))
    {
        channels = static_cast <float**> (preallocatedChannelSpace);
    }
    else
    {
        allocatedData.malloc (numChannels + 1, sizeof (float*));
        channels = reinterpret_cast <float**> (allocatedData.getData());
    }

    for (int i = 0; i < numChannels; ++i)
    {
        // you have to pass in the same number of valid pointers as numChannels
        jassert (dataToReferTo[i] != nullptr);

        channels[i] = dataToReferTo[i] + offset;
    }

    channels [numChannels] = 0;
}

AudioSampleBuffer& AudioSampleBuffer::operator= (const AudioSampleBuffer& other) noexcept
{
    if (this != &other)
    {
        setSize (other.getNumChannels(), other.getNumSamples(), false, false, false);

        const size_t numBytes = size * sizeof (float);

        for (int i = 0; i < numChannels; ++i)
            memcpy (channels[i], other.channels[i], numBytes);
    }

    return *this;
}

AudioSampleBuffer::~AudioSampleBuffer() noexcept
{
}

void AudioSampleBuffer::setSize (const int newNumChannels,
                                 const int newNumSamples,
                                 const bool keepExistingContent,
                                 const bool clearExtraSpace,
                                 const bool avoidReallocating) noexcept
{
    jassert (newNumChannels > 0);

    if (newNumSamples != size || newNumChannels != numChannels)
    {
        const size_t channelListSize = (newNumChannels + 1) * sizeof (float*);
        const size_t newTotalBytes = (newNumChannels * newNumSamples * sizeof (float)) + channelListSize + 32;

        if (keepExistingContent)
        {
            HeapBlock <char> newData;
            newData.allocate (newTotalBytes, clearExtraSpace);

            const int numChansToCopy = jmin (numChannels, newNumChannels);
            const size_t numBytesToCopy = sizeof (float) * jmin (newNumSamples, size);

            float** const newChannels = reinterpret_cast <float**> (newData.getData());
            float* newChan = reinterpret_cast <float*> (newData + channelListSize);
            for (int i = 0; i < numChansToCopy; ++i)
            {
                memcpy (newChan, channels[i], numBytesToCopy);
                newChannels[i] = newChan;
                newChan += newNumSamples;
            }

            allocatedData.swapWith (newData);
            allocatedBytes = (int) newTotalBytes;
            channels = newChannels;
        }
        else
        {
            if (avoidReallocating && allocatedBytes >= newTotalBytes)
            {
                if (clearExtraSpace)
                    allocatedData.clear (newTotalBytes);
            }
            else
            {
                allocatedBytes = newTotalBytes;
                allocatedData.allocate (newTotalBytes, clearExtraSpace);
                channels = reinterpret_cast <float**> (allocatedData.getData());
            }

            float* chan = reinterpret_cast <float*> (allocatedData + channelListSize);
            for (int i = 0; i < newNumChannels; ++i)
            {
                channels[i] = chan;
                chan += newNumSamples;
            }
        }

        channels [newNumChannels] = 0;
        size = newNumSamples;
        numChannels = newNumChannels;
    }
}

void AudioSampleBuffer::clear() noexcept
{
    for (int i = 0; i < numChannels; ++i)
        zeromem (channels[i], size * sizeof (float));
}

void AudioSampleBuffer::clear (const int startSample,
                               const int numSamples) noexcept
{
    jassert (startSample >= 0 && startSample + numSamples <= size);

    for (int i = 0; i < numChannels; ++i)
        zeromem (channels [i] + startSample, numSamples * sizeof (float));
}

void AudioSampleBuffer::clear (const int channel,
                               const int startSample,
                               const int numSamples) noexcept
{
    jassert (isPositiveAndBelow (channel, numChannels));
    jassert (startSample >= 0 && startSample + numSamples <= size);

    zeromem (channels [channel] + startSample, numSamples * sizeof (float));
}

void AudioSampleBuffer::applyGain (const int channel,
                                   const int startSample,
                                   int numSamples,
                                   const float gain) noexcept
{
    jassert (isPositiveAndBelow (channel, numChannels));
    jassert (startSample >= 0 && startSample + numSamples <= size);

    if (gain != 1.0f)
    {
        float* d = channels [channel] + startSample;

        if (gain == 0.0f)
        {
            zeromem (d, sizeof (float) * numSamples);
        }
        else
        {
            while (--numSamples >= 0)
                *d++ *= gain;
        }
    }
}

void AudioSampleBuffer::applyGainRamp (const int channel,
                                       const int startSample,
                                       int numSamples,
                                       float startGain,
                                       float endGain) noexcept
{
    if (startGain == endGain)
    {
        applyGain (channel, startSample, numSamples, startGain);
    }
    else
    {
        jassert (isPositiveAndBelow (channel, numChannels));
        jassert (startSample >= 0 && startSample + numSamples <= size);

        const float increment = (endGain - startGain) / numSamples;
        float* d = channels [channel] + startSample;

        while (--numSamples >= 0)
        {
            *d++ *= startGain;
            startGain += increment;
        }
    }
}

void AudioSampleBuffer::applyGain (const int startSample,
                                   const int numSamples,
                                   const float gain) noexcept
{
    for (int i = 0; i < numChannels; ++i)
        applyGain (i, startSample, numSamples, gain);
}

void AudioSampleBuffer::addFrom (const int destChannel,
                                 const int destStartSample,
                                 const AudioSampleBuffer& source,
                                 const int sourceChannel,
                                 const int sourceStartSample,
                                 int numSamples,
                                 const float gain) noexcept
{
    jassert (&source != this || sourceChannel != destChannel);
    jassert (isPositiveAndBelow (destChannel, numChannels));
    jassert (destStartSample >= 0 && destStartSample + numSamples <= size);
    jassert (isPositiveAndBelow (sourceChannel, source.numChannels));
    jassert (sourceStartSample >= 0 && sourceStartSample + numSamples <= source.size);

    if (gain != 0.0f && numSamples > 0)
    {
        float* d = channels [destChannel] + destStartSample;
        const float* s  = source.channels [sourceChannel] + sourceStartSample;

        if (gain != 1.0f)
        {
            while (--numSamples >= 0)
                *d++ += gain * *s++;
        }
        else
        {
            while (--numSamples >= 0)
                *d++ += *s++;
        }
    }
}

void AudioSampleBuffer::addFrom (const int destChannel,
                                 const int destStartSample,
                                 const float* source,
                                 int numSamples,
                                 const float gain) noexcept
{
    jassert (isPositiveAndBelow (destChannel, numChannels));
    jassert (destStartSample >= 0 && destStartSample + numSamples <= size);
    jassert (source != nullptr);

    if (gain != 0.0f && numSamples > 0)
    {
        float* d = channels [destChannel] + destStartSample;

        if (gain != 1.0f)
        {
            while (--numSamples >= 0)
                *d++ += gain * *source++;
        }
        else
        {
            while (--numSamples >= 0)
                *d++ += *source++;
        }
    }
}

void AudioSampleBuffer::addFromWithRamp (const int destChannel,
                                         const int destStartSample,
                                         const float* source,
                                         int numSamples,
                                         float startGain,
                                         const float endGain) noexcept
{
    jassert (isPositiveAndBelow (destChannel, numChannels));
    jassert (destStartSample >= 0 && destStartSample + numSamples <= size);
    jassert (source != nullptr);

    if (startGain == endGain)
    {
        addFrom (destChannel,
                 destStartSample,
                 source,
                 numSamples,
                 startGain);
    }
    else
    {
        if (numSamples > 0 && (startGain != 0.0f || endGain != 0.0f))
        {
            const float increment = (endGain - startGain) / numSamples;
            float* d = channels [destChannel] + destStartSample;

            while (--numSamples >= 0)
            {
                *d++ += startGain * *source++;
                startGain += increment;
            }
        }
    }
}

void AudioSampleBuffer::copyFrom (const int destChannel,
                                  const int destStartSample,
                                  const AudioSampleBuffer& source,
                                  const int sourceChannel,
                                  const int sourceStartSample,
                                  int numSamples) noexcept
{
    jassert (&source != this || sourceChannel != destChannel);
    jassert (isPositiveAndBelow (destChannel, numChannels));
    jassert (destStartSample >= 0 && destStartSample + numSamples <= size);
    jassert (isPositiveAndBelow (sourceChannel, source.numChannels));
    jassert (sourceStartSample >= 0 && sourceStartSample + numSamples <= source.size);

    if (numSamples > 0)
    {
        memcpy (channels [destChannel] + destStartSample,
                source.channels [sourceChannel] + sourceStartSample,
                sizeof (float) * numSamples);
    }
}

void AudioSampleBuffer::copyFrom (const int destChannel,
                                  const int destStartSample,
                                  const float* source,
                                  int numSamples) noexcept
{
    jassert (isPositiveAndBelow (destChannel, numChannels));
    jassert (destStartSample >= 0 && destStartSample + numSamples <= size);
    jassert (source != nullptr);

    if (numSamples > 0)
    {
        memcpy (channels [destChannel] + destStartSample,
                source,
                sizeof (float) * numSamples);
    }
}

void AudioSampleBuffer::copyFrom (const int destChannel,
                                  const int destStartSample,
                                  const float* source,
                                  int numSamples,
                                  const float gain) noexcept
{
    jassert (isPositiveAndBelow (destChannel, numChannels));
    jassert (destStartSample >= 0 && destStartSample + numSamples <= size);
    jassert (source != nullptr);

    if (numSamples > 0)
    {
        float* d = channels [destChannel] + destStartSample;

        if (gain != 1.0f)
        {
            if (gain == 0)
            {
                zeromem (d, sizeof (float) * numSamples);
            }
            else
            {
                while (--numSamples >= 0)
                    *d++ = gain * *source++;
            }
        }
        else
        {
            memcpy (d, source, sizeof (float) * numSamples);
        }
    }
}

void AudioSampleBuffer::copyFromWithRamp (const int destChannel,
                                          const int destStartSample,
                                          const float* source,
                                          int numSamples,
                                          float startGain,
                                          float endGain) noexcept
{
    jassert (isPositiveAndBelow (destChannel, numChannels));
    jassert (destStartSample >= 0 && destStartSample + numSamples <= size);
    jassert (source != nullptr);

    if (startGain == endGain)
    {
        copyFrom (destChannel,
                  destStartSample,
                  source,
                  numSamples,
                  startGain);
    }
    else
    {
        if (numSamples > 0 && (startGain != 0.0f || endGain != 0.0f))
        {
            const float increment = (endGain - startGain) / numSamples;
            float* d = channels [destChannel] + destStartSample;

            while (--numSamples >= 0)
            {
                *d++ = startGain * *source++;
                startGain += increment;
            }
        }
    }
}

void AudioSampleBuffer::findMinMax (const int channel,
                                    const int startSample,
                                    int numSamples,
                                    float& minVal,
                                    float& maxVal) const noexcept
{
    jassert (isPositiveAndBelow (channel, numChannels));
    jassert (startSample >= 0 && startSample + numSamples <= size);

    findMinAndMax (channels [channel] + startSample, numSamples, minVal, maxVal);
}

float AudioSampleBuffer::getMagnitude (const int channel,
                                       const int startSample,
                                       const int numSamples) const noexcept
{
    jassert (isPositiveAndBelow (channel, numChannels));
    jassert (startSample >= 0 && startSample + numSamples <= size);

    float mn, mx;
    findMinMax (channel, startSample, numSamples, mn, mx);

    return jmax (mn, -mn, mx, -mx);
}

float AudioSampleBuffer::getMagnitude (const int startSample,
                                       const int numSamples) const noexcept
{
    float mag = 0.0f;

    for (int i = 0; i < numChannels; ++i)
        mag = jmax (mag, getMagnitude (i, startSample, numSamples));

    return mag;
}

float AudioSampleBuffer::getRMSLevel (const int channel,
                                      const int startSample,
                                      const int numSamples) const noexcept
{
    jassert (isPositiveAndBelow (channel, numChannels));
    jassert (startSample >= 0 && startSample + numSamples <= size);

    if (numSamples <= 0 || channel < 0 || channel >= numChannels)
        return 0.0f;

    const float* const data = channels [channel] + startSample;
    double sum = 0.0;

    for (int i = 0; i < numSamples; ++i)
    {
        const float sample = data [i];
        sum += sample * sample;
    }

    return (float) std::sqrt (sum / numSamples);
}

void AudioSampleBuffer::readFromAudioReader (AudioFormatReader* reader,
                                             const int startSample,
                                             const int numSamples,
                                             const int64 readerStartSample,
                                             const bool useLeftChan,
                                             const bool useRightChan)
{
    jassert (reader != nullptr);
    jassert (startSample >= 0 && startSample + numSamples <= size);

    if (numSamples > 0)
    {
        int* chans[3];

        if (useLeftChan == useRightChan)
        {
            chans[0] = reinterpret_cast<int*> (getSampleData (0, startSample));
            chans[1] = (reader->numChannels > 1 && getNumChannels() > 1) ? reinterpret_cast<int*> (getSampleData (1, startSample)) : 0;
        }
        else if (useLeftChan || (reader->numChannels == 1))
        {
            chans[0] = reinterpret_cast<int*> (getSampleData (0, startSample));
            chans[1] = nullptr;
        }
        else if (useRightChan)
        {
            chans[0] = nullptr;
            chans[1] = reinterpret_cast<int*> (getSampleData (0, startSample));
        }

        chans[2] = nullptr;

        reader->read (chans, 2, readerStartSample, numSamples, true);

        if (! reader->usesFloatingPointData)
        {
            for (int j = 0; j < 2; ++j)
            {
                float* const d = reinterpret_cast <float*> (chans[j]);

                if (d != nullptr)
                {
                    const float multiplier = 1.0f / 0x7fffffff;

                    for (int i = 0; i < numSamples; ++i)
                        d[i] = *reinterpret_cast<int*> (d + i) * multiplier;
                }
            }
        }

        if (numChannels > 1 && (chans[0] == nullptr || chans[1] == nullptr))
        {
            // if this is a stereo buffer and the source was mono, dupe the first channel..
            memcpy (getSampleData (1, startSample),
                    getSampleData (0, startSample),
                    sizeof (float) * numSamples);
        }
    }
}

void AudioSampleBuffer::writeToAudioWriter (AudioFormatWriter* writer,
                                            const int startSample,
                                            const int numSamples) const
{
    jassert (writer != nullptr);
    writer->writeFromAudioSampleBuffer (*this, startSample, numSamples);
}

END_JUCE_NAMESPACE
