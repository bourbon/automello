# sortFiles.py
# Given a directory of snippets of files, sort them according to their note and monophonic tonality
#
# Colin Raffel, 2012

import utility
import sys
import os
import shutil
import numpy as np

PITCH_TOLERANCE=.05
PITCHES_TO_RUN=6

class MonophonicTonalitySorter:
  def __init__( self, snippetDirectory, destinationDirectory, fs = 44100, nNotes = 72, baseNote = 24 ):
    # Store input params
    self.snippetDirectory = snippetDirectory
    self.destinationDirectory = destinationDirectory
    self.fs = fs
    self.nNotes = nNotes
    self.baseNote = baseNote
    # Create the destination dir if it doesn't exist
    if not os.path.exists( self.destinationDirectory ):
      os.makedirs( self.destinationDirectory )
    # Sort the files and copy them out
    self.sortFiles()
  
  def sortFiles( self ):
    # Get list of wav files, recursively
    fileList = utility.getFiles( self.snippetDirectory, '.wav' )
    # Array of per-file, per-note tonalities.  Each file gets a score for each pitch
    tonalities = np.zeros( ( len( fileList ), self.nNotes ) )
    print "Getting tonalities..."
    for n in np.arange( len( fileList ) ):      
      # Get audio data
      audioData, fs = utility.getWavData( fileList[n] )
      # Make sure the sampling rate matches...
      assert fs == self.fs
      # Get the tonality scores for all notes for this file
      tonalities[n] = self.getTonality( audioData )
    # Hash for the fundamental frequencies of each file
    fileFrequencies = {}
    # Find the best file for each note
    print "Finding best candidates for notes"
    for n in np.arange( self.nNotes ):
      # Get the tonality scores
      tonalitiesForThisNote = tonalities[:,n]
      # Get the sorted indices of tonality scores
      tonalitiesSort = np.argsort( tonalitiesForThisNote )[::-1]
      # Keep track of which sorted array index we're getting the frequency for
      sortedIndex = 0
      # What frequency is the note we're looking for?
      targetHz = utility.midiToHz( self.baseNote + n )
      # What's the detected Hz of the note?
      detectedHz = 1.0
      # Until we find an audio file whose YIN detected pitch is sufficiently close
      while ((targetHz/detectedHz) < (1 - PITCH_TOLERANCE) or (targetHz/detectedHz) > (1 + PITCH_TOLERANCE)) and sortedIndex < tonalitiesSort.shape[0] and sortedIndex < PITCHES_TO_RUN:
        # If this file has not been YIN analyzed yet, analyze it
        if not fileFrequencies.has_key( tonalitiesSort[sortedIndex] ):        
          audioData, fs = utility.getWavData( fileList[tonalitiesSort[sortedIndex]] )
          # ... and store it so that you don't have to calculate it next time
          fileFrequencies[tonalitiesSort[sortedIndex]] = self.yinPitchDetect( audioData )
        detectedHz = fileFrequencies[tonalitiesSort[sortedIndex]]
        # Check the next file next time
        sortedIndex += 1
      # If we didn't run out of files, copy out the file that we found (with the closest pitch)
      if sortedIndex < len( fileList ):
        shutil.copy( fileList[tonalitiesSort[sortedIndex-1]], os.path.join( self.destinationDirectory, str(n + self.baseNote) + ".wav" ) )
      else:
        shutil.copy( fileList[tonalitiesSort[0]], os.path.join( self.destinationDirectory, str(n + self.baseNote) + ".wav" ) )
  
  # Get the tonality score for some audio data
  def getTonality( self, audioData ):
    # Number of samples in the audio data
    N = audioData.shape[0]
    # Tonality scores for each note
    tonalityScores = np.zeros( self.nNotes )
    # Get magnitude spectrum for this audio data
    AudioData = np.abs( np.fft.rfft( audioData ) )
    # Calculate spectral crest factor and RMS... not sure if we should use these
    spectralCrestFactor = np.max( AudioData )/np.mean( AudioData )
    RMS = np.sqrt( np.sum( audioData**2.0 ) )/(N*1.0)
    # For each note
    for note in np.arange( self.nNotes ):
      # Create a mask for only bins which are harmonics of the note in question
      mask = self.createMask( N, utility.midiToHz( self.baseNote + note ) )
      # Calculate "monophonic tonality" - max of bins corresponding to harmonics/spectral mean
      monophonicTonality = np.max( AudioData[mask == 1] )/np.mean( AudioData[mask == 0] )
      # Write out tonality score
      tonalityScores[note] = monophonicTonality*RMS
    return tonalityScores
  
  # Create an array of 1s and 0s with 1s at harmonic multiples of the base frequency
  def createMask( self, N, baseFrequency ):
    # Number of bins in the mask
    nBins = N/2 - 1
    # Maximum frequency that we want to create a mask for
    maxFrequency = utility.midiToHz( self.baseNote + self.nNotes + 24 )
    # Make sure it's not out of range... above nyquist or so
    if maxFrequency > (self.fs*.9)/2.0:
      maxFrequency = (self.fs*.9)/2.0
    # Create the mask
    mask = np.zeros( nBins )
    # Over all harmonic frequencies
    for harmonic in baseFrequency*(2**np.arange( 4 )):
      if harmonic > maxFrequency:
        break
      # Set bins near this harmonic to 1
      mask[utility.hzToBins( harmonic, N, self.fs )] = 1
    return mask

  # Yin pitch detector.  Returns f0 of input frame.  
  def yinPitchDetect( self, frame, threshold=0.15, W=None ):
    # Assume window size = frame size/2
    if not W:
      W = np.floor(frame.shape[0]/2.0)
    # Number of lags possible
    nLags = frame.shape[0] - W
    # Pre-allocate squared difference
    squaredDifference = np.zeros( nLags )
    # Calculate squared difference for all lags
    for tau in np.arange( nLags ):
      squaredDifference[tau] = np.sum( (frame[:W] - frame[tau:W+tau])**2.0 )
    # Calculate the "cumulative-mean-normalized" square difference
    squaredDifferenceNormalized = np.zeros(nLags)
    squaredDifferenceNormalized[0] = 1.0
    for tau in np.arange( 1, nLags ):
      squaredDifferenceNormalized[tau] = squaredDifference[tau]/(np.sum( squaredDifference[1:tau] )/tau)
    # Find first local minima which is below the threshold
    f0 = None
    for n in np.arange( 1, nLags-1 ):
      if squaredDifferenceNormalized[n-1] > squaredDifferenceNormalized[n] \
      and squaredDifferenceNormalized[n+1] > squaredDifferenceNormalized[n] \
      and squaredDifferenceNormalized[n] < threshold:
        f0 = n
        break
    # No local minima found below threshold
    if not f0:
      f0 = np.argmin( squaredDifferenceNormalized[1:-1] )
    # Parabolic interpolation
    peakOffset = (squaredDifferenceNormalized[f0+1] - squaredDifferenceNormalized[f0-1])\
      /(2.0*(2.0*squaredDifferenceNormalized[f0] - squaredDifferenceNormalized[f0+1] - squaredDifferenceNormalized[f0-1]))
    f0 = f0 + peakOffset
    f0 = self.fs/f0
    return f0


if __name__ == "__main__":
  if len(sys.argv) < 2:
    print "Usage: %s snippetDirectory outputDirectory" % sys.argv[0]
    sys.exit(-1)
  MonophonicTonalitySorter( sys.argv[1], sys.argv[2] )
