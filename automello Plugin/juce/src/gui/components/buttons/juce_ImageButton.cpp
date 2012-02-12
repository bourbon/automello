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

#include "../../../core/juce_StandardHeader.h"

BEGIN_JUCE_NAMESPACE

#include "juce_ImageButton.h"
#include "../../graphics/imaging/juce_ImageCache.h"
#include "../lookandfeel/juce_LookAndFeel.h"


//==============================================================================
ImageButton::ImageButton (const String& text_)
    : Button (text_),
      scaleImageToFit (true),
      preserveProportions (true),
      alphaThreshold (0),
      imageX (0),
      imageY (0),
      imageW (0),
      imageH (0)
{
}

ImageButton::~ImageButton()
{
}

void ImageButton::setImages (const bool resizeButtonNowToFitThisImage,
                             const bool rescaleImagesWhenButtonSizeChanges,
                             const bool preserveImageProportions,
                             const Image& normalImage_,
                             const float imageOpacityWhenNormal,
                             const Colour& overlayColourWhenNormal,
                             const Image& overImage_,
                             const float imageOpacityWhenOver,
                             const Colour& overlayColourWhenOver,
                             const Image& downImage_,
                             const float imageOpacityWhenDown,
                             const Colour& overlayColourWhenDown,
                             const float hitTestAlphaThreshold)
{
    normalImage = normalImage_;
    overImage = overImage_;
    downImage = downImage_;

    if (resizeButtonNowToFitThisImage && normalImage.isValid())
    {
        imageW = normalImage.getWidth();
        imageH = normalImage.getHeight();

        setSize (imageW, imageH);
    }

    scaleImageToFit = rescaleImagesWhenButtonSizeChanges;
    preserveProportions = preserveImageProportions;

    normalOpacity = imageOpacityWhenNormal;
    normalOverlay = overlayColourWhenNormal;
    overOpacity   = imageOpacityWhenOver;
    overOverlay   = overlayColourWhenOver;
    downOpacity   = imageOpacityWhenDown;
    downOverlay   = overlayColourWhenDown;

    alphaThreshold = (unsigned char) jlimit (0, 0xff, roundToInt (255.0f * hitTestAlphaThreshold));

    repaint();
}

Image ImageButton::getCurrentImage() const
{
    if (isDown() || getToggleState())
        return getDownImage();

    if (isOver())
        return getOverImage();

    return getNormalImage();
}

Image ImageButton::getNormalImage() const
{
    return normalImage;
}

Image ImageButton::getOverImage() const
{
    return overImage.isValid() ? overImage
                               : normalImage;
}

Image ImageButton::getDownImage() const
{
    return downImage.isValid() ? downImage
                               : getOverImage();
}

void ImageButton::paintButton (Graphics& g,
                               bool isMouseOverButton,
                               bool isButtonDown)
{
    if (! isEnabled())
    {
        isMouseOverButton = false;
        isButtonDown = false;
    }

    Image im (getCurrentImage());

    if (im.isValid())
    {
        const int iw = im.getWidth();
        const int ih = im.getHeight();
        imageW = getWidth();
        imageH = getHeight();
        imageX = (imageW - iw) >> 1;
        imageY = (imageH - ih) >> 1;

        if (scaleImageToFit)
        {
            if (preserveProportions)
            {
                int newW, newH;
                const float imRatio = ih / (float)iw;
                const float destRatio = imageH / (float)imageW;

                if (imRatio > destRatio)
                {
                    newW = roundToInt (imageH / imRatio);
                    newH = imageH;
                }
                else
                {
                    newW = imageW;
                    newH = roundToInt (imageW * imRatio);
                }

                imageX = (imageW - newW) / 2;
                imageY = (imageH - newH) / 2;
                imageW = newW;
                imageH = newH;
            }
            else
            {
                imageX = 0;
                imageY = 0;
            }
        }

        if (! scaleImageToFit)
        {
            imageW = iw;
            imageH = ih;
        }

        getLookAndFeel().drawImageButton (g, &im, imageX, imageY, imageW, imageH,
                                          isButtonDown ? downOverlay
                                                       : (isMouseOverButton ? overOverlay
                                                                            : normalOverlay),
                                          isButtonDown ? downOpacity
                                                       : (isMouseOverButton ? overOpacity
                                                                            : normalOpacity),
                                          *this);
    }
}

bool ImageButton::hitTest (int x, int y)
{
    if (alphaThreshold == 0)
        return true;

    Image im (getCurrentImage());

    return im.isNull() || (imageW > 0 && imageH > 0
                            && alphaThreshold < im.getPixelAt (((x - imageX) * im.getWidth()) / imageW,
                                                               ((y - imageY) * im.getHeight()) / imageH).getAlpha());
}

END_JUCE_NAMESPACE
