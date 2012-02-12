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

#include "juce_ToolbarButton.h"
#include "../controls/juce_ToolbarItemFactory.h"
#include "../../graphics/imaging/juce_Image.h"
#include "../lookandfeel/juce_LookAndFeel.h"


//==============================================================================
ToolbarButton::ToolbarButton (const int itemId_, const String& buttonText,
                              Drawable* const normalImage_, Drawable* const toggledOnImage_)
   : ToolbarItemComponent (itemId_, buttonText, true),
     normalImage (normalImage_),
     toggledOnImage (toggledOnImage_),
     currentImage (nullptr)
{
    jassert (normalImage_ != nullptr);
}

ToolbarButton::~ToolbarButton()
{
}

//==============================================================================
bool ToolbarButton::getToolbarItemSizes (int toolbarDepth, bool /*isToolbarVertical*/, int& preferredSize, int& minSize, int& maxSize)
{
    preferredSize = minSize = maxSize = toolbarDepth;
    return true;
}

void ToolbarButton::paintButtonArea (Graphics&, int /*width*/, int /*height*/, bool /*isMouseOver*/, bool /*isMouseDown*/)
{
}

void ToolbarButton::contentAreaChanged (const Rectangle<int>&)
{
    buttonStateChanged();
}

void ToolbarButton::updateDrawable()
{
    if (currentImage != nullptr)
    {
        currentImage->setTransformToFit (getContentArea().toFloat(), RectanglePlacement::centred);
        currentImage->setAlpha (isEnabled() ? 1.0f : 0.5f);
    }
}

void ToolbarButton::resized()
{
    ToolbarItemComponent::resized();
    updateDrawable();
}

void ToolbarButton::enablementChanged()
{
    ToolbarItemComponent::enablementChanged();
    updateDrawable();
}

void ToolbarButton::buttonStateChanged()
{
    Drawable* d = normalImage;

    if (getToggleState() && toggledOnImage != nullptr)
        d = toggledOnImage;

    if (d != currentImage)
    {
        removeChildComponent (currentImage);
        currentImage = d;

        if (d != nullptr)
        {
            enablementChanged();
            addAndMakeVisible (d);
            updateDrawable();
        }
    }
}


END_JUCE_NAMESPACE
