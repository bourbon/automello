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

#include "../../../../core/juce_StandardHeader.h"

#if JUCE_MSVC
  #pragma warning (push)
#endif

namespace jpeglibNamespace
{
#if JUCE_INCLUDE_JPEGLIB_CODE
  #if JUCE_MINGW
    typedef unsigned char boolean;
  #endif
    #define JPEG_INTERNALS
    #undef FAR
    #include "jpglib/jpeglib.h"

    #include "jpglib/jcapimin.c"
    #include "jpglib/jcapistd.c"
    #include "jpglib/jccoefct.c"
    #include "jpglib/jccolor.c"
    #undef FIX
    #include "jpglib/jcdctmgr.c"
    #undef CONST_BITS
    #include "jpglib/jchuff.c"
    #undef emit_byte
    #include "jpglib/jcinit.c"
    #include "jpglib/jcmainct.c"
    #include "jpglib/jcmarker.c"
    #include "jpglib/jcmaster.c"
    #include "jpglib/jcomapi.c"
    #include "jpglib/jcparam.c"
    #include "jpglib/jcphuff.c"
    #include "jpglib/jcprepct.c"
    #include "jpglib/jcsample.c"
    #include "jpglib/jctrans.c"
    #include "jpglib/jdapistd.c"
    #include "jpglib/jdapimin.c"
    #include "jpglib/jdatasrc.c"
    #include "jpglib/jdcoefct.c"
    #undef FIX
    #include "jpglib/jdcolor.c"
    #undef FIX
    #include "jpglib/jddctmgr.c"
    #undef CONST_BITS
    #undef ASSIGN_STATE
    #include "jpglib/jdhuff.c"
    #include "jpglib/jdinput.c"
    #include "jpglib/jdmainct.c"
    #include "jpglib/jdmarker.c"
    #include "jpglib/jdmaster.c"
    #undef FIX
    #include "jpglib/jdmerge.c"
    #undef ASSIGN_STATE
    #include "jpglib/jdphuff.c"
    #include "jpglib/jdpostct.c"
    #undef FIX
    #include "jpglib/jdsample.c"
    #include "jpglib/jdtrans.c"
    #include "jpglib/jfdctflt.c"
    #include "jpglib/jfdctint.c"
    #undef CONST_BITS
    #undef MULTIPLY
    #undef FIX_0_541196100
    #include "jpglib/jfdctfst.c"
    #undef FIX_0_541196100
    #include "jpglib/jidctflt.c"
    #undef CONST_BITS
    #undef FIX_1_847759065
    #undef MULTIPLY
    #undef DEQUANTIZE
    #undef DESCALE
    #include "jpglib/jidctfst.c"
    #undef CONST_BITS
    #undef FIX_1_847759065
    #undef MULTIPLY
    #undef DEQUANTIZE
    #include "jpglib/jidctint.c"
    #include "jpglib/jidctred.c"
    #include "jpglib/jmemmgr.c"
    #include "jpglib/jmemnobs.c"
    #include "jpglib/jquant1.c"
    #include "jpglib/jquant2.c"
    #include "jpglib/jutils.c"
    #include "jpglib/transupp.c"
#else
    #define JPEG_INTERNALS
    #undef FAR
    #include <jpeglib.h>
#endif
}

#undef max
#undef min

#if JUCE_MSVC
  #pragma warning (pop)
#endif

BEGIN_JUCE_NAMESPACE

#include "../juce_ImageFileFormat.h"
#include "../../../../io/streams/juce_InputStream.h"
#include "../../../../io/streams/juce_MemoryOutputStream.h"
#include "../../colour/juce_PixelFormats.h"

//==============================================================================
namespace JPEGHelpers
{
    using namespace jpeglibNamespace;

    #if ! JUCE_MSVC
     using jpeglibNamespace::boolean;
    #endif

    struct JPEGDecodingFailure {};

    void fatalErrorHandler (j_common_ptr)
    {
        throw JPEGDecodingFailure();
    }

    void silentErrorCallback1 (j_common_ptr)         {}
    void silentErrorCallback2 (j_common_ptr, int)    {}
    void silentErrorCallback3 (j_common_ptr, char*)  {}

    void setupSilentErrorHandler (struct jpeg_error_mgr& err)
    {
        zerostruct (err);

        err.error_exit = fatalErrorHandler;
        err.emit_message = silentErrorCallback2;
        err.output_message = silentErrorCallback1;
        err.format_message = silentErrorCallback3;
        err.reset_error_mgr = silentErrorCallback1;
    }


    //==============================================================================
    void dummyCallback1 (j_decompress_ptr)
    {
    }

    void jpegSkip (j_decompress_ptr decompStruct, long num)
    {
        decompStruct->src->next_input_byte += num;

        num = jmin (num, (long) decompStruct->src->bytes_in_buffer);
        decompStruct->src->bytes_in_buffer -= num;
    }

    boolean jpegFill (j_decompress_ptr)
    {
        return 0;
    }

    //==============================================================================
    const int jpegBufferSize = 512;

    struct JuceJpegDest  : public jpeg_destination_mgr
    {
        OutputStream* output;
        char* buffer;
    };

    void jpegWriteInit (j_compress_ptr)
    {
    }

    void jpegWriteTerminate (j_compress_ptr cinfo)
    {
        JuceJpegDest* const dest = static_cast <JuceJpegDest*> (cinfo->dest);

        const size_t numToWrite = jpegBufferSize - dest->free_in_buffer;
        dest->output->write (dest->buffer, (int) numToWrite);
    }

    boolean jpegWriteFlush (j_compress_ptr cinfo)
    {
        JuceJpegDest* const dest = static_cast <JuceJpegDest*> (cinfo->dest);

        const int numToWrite = jpegBufferSize;

        dest->next_output_byte = reinterpret_cast <JOCTET*> (dest->buffer);
        dest->free_in_buffer = jpegBufferSize;

        return dest->output->write (dest->buffer, numToWrite);
    }
}

//==============================================================================
JPEGImageFormat::JPEGImageFormat()
    : quality (-1.0f)
{
}

JPEGImageFormat::~JPEGImageFormat()     {}

void JPEGImageFormat::setQuality (const float newQuality)
{
    quality = newQuality;
}

String JPEGImageFormat::getFormatName() { return "JPEG"; }

bool JPEGImageFormat::canUnderstand (InputStream& in)
{
    const int bytesNeeded = 10;
    uint8 header [bytesNeeded];

    if (in.read (header, bytesNeeded) == bytesNeeded)
    {
        return header[0] == 0xff
            && header[1] == 0xd8
            && header[2] == 0xff
            && (header[3] == 0xe0 || header[3] == 0xe1);
    }

    return false;
}

#if (JUCE_MAC || JUCE_IOS) && USE_COREGRAPHICS_RENDERING && ! DONT_USE_COREIMAGE_LOADER
 Image juce_loadWithCoreImage (InputStream& input);
#endif

Image JPEGImageFormat::decodeImage (InputStream& in)
{
#if (JUCE_MAC || JUCE_IOS) && USE_COREGRAPHICS_RENDERING && ! DONT_USE_COREIMAGE_LOADER
    return juce_loadWithCoreImage (in);
#else
    using namespace jpeglibNamespace;
    using namespace JPEGHelpers;

    MemoryOutputStream mb;
    mb.writeFromInputStream (in, -1);

    Image image;

    if (mb.getDataSize() > 16)
    {
        struct jpeg_decompress_struct jpegDecompStruct;

        struct jpeg_error_mgr jerr;
        setupSilentErrorHandler (jerr);
        jpegDecompStruct.err = &jerr;

        jpeg_create_decompress (&jpegDecompStruct);

        jpegDecompStruct.src = (jpeg_source_mgr*)(jpegDecompStruct.mem->alloc_small)
            ((j_common_ptr)(&jpegDecompStruct), JPOOL_PERMANENT, sizeof (jpeg_source_mgr));

        jpegDecompStruct.src->init_source       = dummyCallback1;
        jpegDecompStruct.src->fill_input_buffer = jpegFill;
        jpegDecompStruct.src->skip_input_data   = jpegSkip;
        jpegDecompStruct.src->resync_to_restart = jpeg_resync_to_restart;
        jpegDecompStruct.src->term_source       = dummyCallback1;

        jpegDecompStruct.src->next_input_byte   = static_cast <const unsigned char*> (mb.getData());
        jpegDecompStruct.src->bytes_in_buffer   = mb.getDataSize();

        try
        {
            jpeg_read_header (&jpegDecompStruct, TRUE);

            jpeg_calc_output_dimensions (&jpegDecompStruct);

            const int width = jpegDecompStruct.output_width;
            const int height = jpegDecompStruct.output_height;

            jpegDecompStruct.out_color_space = JCS_RGB;

            JSAMPARRAY buffer
                = (*jpegDecompStruct.mem->alloc_sarray) ((j_common_ptr) &jpegDecompStruct,
                                                         JPOOL_IMAGE,
                                                         width * 3, 1);

            if (jpeg_start_decompress (&jpegDecompStruct))
            {
                image = Image (Image::RGB, width, height, false);
                image.getProperties()->set ("originalImageHadAlpha", false);
                const bool hasAlphaChan = image.hasAlphaChannel(); // (the native image creator may not give back what we expect)

                const Image::BitmapData destData (image, Image::BitmapData::writeOnly);

                for (int y = 0; y < height; ++y)
                {
                    jpeg_read_scanlines (&jpegDecompStruct, buffer, 1);

                    const uint8* src = *buffer;
                    uint8* dest = destData.getLinePointer (y);

                    if (hasAlphaChan)
                    {
                        for (int i = width; --i >= 0;)
                        {
                            ((PixelARGB*) dest)->setARGB (0xff, src[0], src[1], src[2]);
                            ((PixelARGB*) dest)->premultiply();
                            dest += destData.pixelStride;
                            src += 3;
                        }
                    }
                    else
                    {
                        for (int i = width; --i >= 0;)
                        {
                            ((PixelRGB*) dest)->setARGB (0xff, src[0], src[1], src[2]);
                            dest += destData.pixelStride;
                            src += 3;
                        }
                    }
                }

                jpeg_finish_decompress (&jpegDecompStruct);

                in.setPosition (((char*) jpegDecompStruct.src->next_input_byte) - (char*) mb.getData());
            }

            jpeg_destroy_decompress (&jpegDecompStruct);
        }
        catch (...)
        {}
    }

    return image;
#endif
}

bool JPEGImageFormat::writeImageToStream (const Image& image, OutputStream& out)
{
    using namespace jpeglibNamespace;
    using namespace JPEGHelpers;

    if (image.hasAlphaChannel())
    {
        // this method could fill the background in white and still save the image..
        jassertfalse;
        return true;
    }

    struct jpeg_compress_struct jpegCompStruct;

    struct jpeg_error_mgr jerr;
    setupSilentErrorHandler (jerr);
    jpegCompStruct.err = &jerr;

    jpeg_create_compress (&jpegCompStruct);

    JuceJpegDest dest;
    jpegCompStruct.dest = &dest;

    dest.output = &out;
    HeapBlock <char> tempBuffer (jpegBufferSize);
    dest.buffer = tempBuffer;
    dest.next_output_byte = (JOCTET*) dest.buffer;
    dest.free_in_buffer = jpegBufferSize;
    dest.init_destination = jpegWriteInit;
    dest.empty_output_buffer = jpegWriteFlush;
    dest.term_destination = jpegWriteTerminate;

    jpegCompStruct.image_width = image.getWidth();
    jpegCompStruct.image_height = image.getHeight();
    jpegCompStruct.input_components = 3;
    jpegCompStruct.in_color_space = JCS_RGB;
    jpegCompStruct.write_JFIF_header = 1;

    jpegCompStruct.X_density = 72;
    jpegCompStruct.Y_density = 72;

    jpeg_set_defaults (&jpegCompStruct);

    jpegCompStruct.dct_method = JDCT_FLOAT;
    jpegCompStruct.optimize_coding = 1;
    //jpegCompStruct.smoothing_factor = 10;

    if (quality < 0.0f)
        quality = 0.85f;

    jpeg_set_quality (&jpegCompStruct, jlimit (0, 100, roundToInt (quality * 100.0f)), TRUE);

    jpeg_start_compress (&jpegCompStruct, TRUE);

    const int strideBytes = jpegCompStruct.image_width * jpegCompStruct.input_components;

    JSAMPARRAY buffer = (*jpegCompStruct.mem->alloc_sarray) ((j_common_ptr) &jpegCompStruct,
                                                             JPOOL_IMAGE, strideBytes, 1);

    const Image::BitmapData srcData (image, Image::BitmapData::readOnly);

    while (jpegCompStruct.next_scanline < jpegCompStruct.image_height)
    {
        const uint8* src = srcData.getLinePointer (jpegCompStruct.next_scanline);
        uint8* dst = *buffer;

        for (int i = jpegCompStruct.image_width; --i >= 0;)
        {
            *dst++ = ((const PixelRGB*) src)->getRed();
            *dst++ = ((const PixelRGB*) src)->getGreen();
            *dst++ = ((const PixelRGB*) src)->getBlue();
            src += srcData.pixelStride;
        }

        jpeg_write_scanlines (&jpegCompStruct, buffer, 1);
    }

    jpeg_finish_compress (&jpegCompStruct);
    jpeg_destroy_compress (&jpegCompStruct);

    out.flush();

    return true;
}


END_JUCE_NAMESPACE
