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

#ifndef __JUCE_AUDIOFORMATWRITER_JUCEHEADER__
#define __JUCE_AUDIOFORMATWRITER_JUCEHEADER__

#include "../../io/files/juce_FileOutputStream.h"
#include "juce_AudioFormatReader.h"
#include "../audio_sources/juce_AudioSource.h"
#include "../../threads/juce_TimeSliceThread.h"
class AudioThumbnail;


//==============================================================================
/**
    Writes samples to an audio file stream.

    A subclass that writes a specific type of audio format will be created by
    an AudioFormat object.

    After creating one of these with the AudioFormat::createWriterFor() method
    you can call its write() method to store the samples, and then delete it.

    @see AudioFormat, AudioFormatReader
*/
class JUCE_API  AudioFormatWriter
{
protected:
    //==============================================================================
    /** Creates an AudioFormatWriter object.

        @param destStream       the stream to write to - this will be deleted
                                by this object when it is no longer needed
        @param formatName       the description that will be returned by the getFormatName()
                                method
        @param sampleRate       the sample rate to use - the base class just stores
                                this value, it doesn't do anything with it
        @param numberOfChannels the number of channels to write - the base class just stores
                                this value, it doesn't do anything with it
        @param bitsPerSample    the bit depth of the stream - the base class just stores
                                this value, it doesn't do anything with it
    */
    AudioFormatWriter (OutputStream* destStream,
                       const String& formatName,
                       double sampleRate,
                       unsigned int numberOfChannels,
                       unsigned int bitsPerSample);

public:
    /** Destructor. */
    virtual ~AudioFormatWriter();

    //==============================================================================
    /** Returns a description of what type of format this is.

        E.g. "AIFF file"
    */
    const String& getFormatName() const noexcept        { return formatName; }

    //==============================================================================
    /** Writes a set of samples to the audio stream.

        Note that if you're trying to write the contents of an AudioSampleBuffer, you
        can use AudioSampleBuffer::writeToAudioWriter().

        @param samplesToWrite   an array of arrays containing the sample data for
                                each channel to write. This is a zero-terminated
                                array of arrays, and can contain a different number
                                of channels than the actual stream uses, and the
                                writer should do its best to cope with this.
                                If the format is fixed-point, each channel will be formatted
                                as an array of signed integers using the full 32-bit
                                range -0x80000000 to 0x7fffffff, regardless of the source's
                                bit-depth. If it is a floating-point format, you should treat
                                the arrays as arrays of floats, and just cast it to an (int**)
                                to pass it into the method.
        @param numSamples       the number of samples to write
    */
    virtual bool write (const int** samplesToWrite,
                        int numSamples) = 0;

    //==============================================================================
    /** Reads a section of samples from an AudioFormatReader, and writes these to
        the output.

        This will take care of any floating-point conversion that's required to convert
        between the two formats. It won't deal with sample-rate conversion, though.

        If numSamplesToRead < 0, it will write the entire length of the reader.

        @returns false if it can't read or write properly during the operation
    */
    bool writeFromAudioReader (AudioFormatReader& reader,
                               int64 startSample,
                               int64 numSamplesToRead);

    /** Reads some samples from an AudioSource, and writes these to the output.

        The source must already have been initialised with the AudioSource::prepareToPlay() method

        @param source               the source to read from
        @param numSamplesToRead     total number of samples to read and write
        @param samplesPerBlock      the maximum number of samples to fetch from the source
        @returns false if it can't read or write properly during the operation
    */
    bool writeFromAudioSource (AudioSource& source,
                               int numSamplesToRead,
                               int samplesPerBlock = 2048);


    /** Writes some samples from an AudioSampleBuffer.

    */
    bool writeFromAudioSampleBuffer (const AudioSampleBuffer& source,
                                     int startSample, int numSamples);

    //==============================================================================
    /** Returns the sample rate being used. */
    double getSampleRate() const noexcept       { return sampleRate; }

    /** Returns the number of channels being written. */
    int getNumChannels() const noexcept         { return numChannels; }

    /** Returns the bit-depth of the data being written. */
    int getBitsPerSample() const noexcept       { return bitsPerSample; }

    /** Returns true if it's a floating-point format, false if it's fixed-point. */
    bool isFloatingPoint() const noexcept       { return usesFloatingPointData; }

    //==============================================================================
    /**
        Provides a FIFO for an AudioFormatWriter, allowing you to push incoming
        data into a buffer which will be flushed to disk by a background thread.
    */
    class ThreadedWriter
    {
    public:
        /** Creates a ThreadedWriter for a given writer and a thread.

            The writer object which is passed in here will be owned and deleted by
            the ThreadedWriter when it is no longer needed.

            To stop the writer and flush the buffer to disk, simply delete this object.
        */
        ThreadedWriter (AudioFormatWriter* writer,
                        TimeSliceThread& backgroundThread,
                        int numSamplesToBuffer);

        /** Destructor. */
        ~ThreadedWriter();

        /** Pushes some incoming audio data into the FIFO.

            If there's enough free space in the buffer, this will add the data to it,

            If the FIFO is too full to accept this many samples, the method will return
            false - then you could either wait until the background thread has had time to
            consume some of the buffered data and try again, or you can give up
            and lost this block.

            The data must be an array containing the same number of channels as the
            AudioFormatWriter object is using. None of these channels can be null.
        */
        bool write (const float** data, int numSamples);

        /** Allows you to specify a thumbnail that this writer should update with the
            incoming data.
            The thumbnail will be cleared and will the writer will begin adding data to
            it as it arrives. Pass a null pointer to stop the writer updating any thumbnails.
        */
        void setThumbnailToUpdate (AudioThumbnail* thumbnailToUpdate);

       #ifndef DOXYGEN
        class Buffer; // (only public for VC6 compatibility)
       #endif

    private:
        friend class ScopedPointer<Buffer>;
        ScopedPointer<Buffer> buffer;
    };

protected:
    //==============================================================================
    /** The sample rate of the stream. */
    double sampleRate;

    /** The number of channels being written to the stream. */
    unsigned int numChannels;

    /** The bit depth of the file. */
    unsigned int bitsPerSample;

    /** True if it's a floating-point format, false if it's fixed-point. */
    bool usesFloatingPointData;

    /** The output stream for Use by subclasses. */
    OutputStream* output;

    /** Used by AudioFormatWriter subclasses to copy data to different formats. */
    template <class DestSampleType, class SourceSampleType, class DestEndianness>
    struct WriteHelper
    {
        typedef AudioData::Pointer <DestSampleType, DestEndianness, AudioData::Interleaved, AudioData::NonConst>                DestType;
        typedef AudioData::Pointer <SourceSampleType, AudioData::NativeEndian, AudioData::NonInterleaved, AudioData::Const>     SourceType;

        static void write (void* destData, int numDestChannels, const int** source,
                           int numSamples, const int sourceOffset = 0) noexcept
        {
            for (int i = 0; i < numDestChannels; ++i)
            {
                const DestType dest (addBytesToPointer (destData, i * DestType::getBytesPerSample()), numDestChannels);

                if (*source != nullptr)
                {
                    dest.convertSamples (SourceType (*source + sourceOffset), numSamples);
                    ++source;
                }
                else
                {
                    dest.clearSamples (numSamples);
                }
            }
        }
    };

private:
    String formatName;
    friend class ThreadedWriter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioFormatWriter);
};


#endif   // __JUCE_AUDIOFORMATWRITER_JUCEHEADER__
