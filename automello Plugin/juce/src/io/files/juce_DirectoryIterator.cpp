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

#include "juce_DirectoryIterator.h"


//==============================================================================
DirectoryIterator::DirectoryIterator (const File& directory,
                                      bool isRecursive_,
                                      const String& wildCard_,
                                      const int whatToLookFor_)
  : fileFinder (directory, isRecursive_ ? "*" : wildCard_),
    wildCard (wildCard_),
    path (File::addTrailingSeparator (directory.getFullPathName())),
    index (-1),
    totalNumFiles (-1),
    whatToLookFor (whatToLookFor_),
    isRecursive (isRecursive_),
    hasBeenAdvanced (false)
{
    // you have to specify the type of files you're looking for!
    jassert ((whatToLookFor_ & (File::findFiles | File::findDirectories)) != 0);
    jassert (whatToLookFor_ > 0 && whatToLookFor_ <= 7);
}

DirectoryIterator::~DirectoryIterator()
{
}

bool DirectoryIterator::next()
{
    return next (nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
}

bool DirectoryIterator::next (bool* const isDirResult, bool* const isHiddenResult, int64* const fileSize,
                              Time* const modTime, Time* const creationTime, bool* const isReadOnly)
{
    hasBeenAdvanced = true;

    if (subIterator != nullptr)
    {
        if (subIterator->next (isDirResult, isHiddenResult, fileSize, modTime, creationTime, isReadOnly))
            return true;

        subIterator = nullptr;
    }

    String filename;
    bool isDirectory, isHidden;
    while (fileFinder.next (filename, &isDirectory, &isHidden, fileSize, modTime, creationTime, isReadOnly))
    {
        ++index;

        if (! filename.containsOnly ("."))
        {
            const File fileFound (path + filename, 0);
            bool matches = false;

            if (isDirectory)
            {
                if (isRecursive && ((whatToLookFor & File::ignoreHiddenFiles) == 0 || ! isHidden))
                    subIterator = new DirectoryIterator (fileFound, true, wildCard, whatToLookFor);

                matches = (whatToLookFor & File::findDirectories) != 0;
            }
            else
            {
                matches = (whatToLookFor & File::findFiles) != 0;
            }

            // if recursive, we're not relying on the OS iterator to do the wildcard match, so do it now..
            if (matches && isRecursive)
                matches = filename.matchesWildcard (wildCard, ! File::areFileNamesCaseSensitive());

            if (matches && (whatToLookFor & File::ignoreHiddenFiles) != 0)
                matches = ! isHidden;

            if (matches)
            {
                currentFile = fileFound;
                if (isHiddenResult != nullptr)     *isHiddenResult = isHidden;
                if (isDirResult != nullptr)        *isDirResult = isDirectory;

                return true;
            }
            else if (subIterator != nullptr)
            {
                return next();
            }
        }
    }

    return false;
}

const File& DirectoryIterator::getFile() const
{
    if (subIterator != nullptr && subIterator->hasBeenAdvanced)
        return subIterator->getFile();

    // You need to call DirectoryIterator::next() before asking it for the file that it found!
    jassert (hasBeenAdvanced);

    return currentFile;
}

float DirectoryIterator::getEstimatedProgress() const
{
    if (totalNumFiles < 0)
        totalNumFiles = File (path).getNumberOfChildFiles (File::findFilesAndDirectories);

    if (totalNumFiles <= 0)
        return 0.0f;

    const float detailedIndex = (subIterator != nullptr) ? index + subIterator->getEstimatedProgress()
                                                         : (float) index;

    return detailedIndex / totalNumFiles;
}

END_JUCE_NAMESPACE
