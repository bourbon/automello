# sortFiles.py
# Given a directory of snippets of files, sort them according to their note and monophonic tonality
#
# Colin Raffel, 2012

import utility
import sys
import os
import shutil
import numpy as np

class fileSorter:
  def __init__( self, snippetDirectory, destinationDirectory, fs = 44100, nNotes = 60, baseNote = 24 ):
    self.snippetDirectory = snippetDirectory
    self.destinationDirectory = destinationDirectory
    self.fs = fs
    self.nNotes = nNotes
    self.baseNote = baseNote
    self.sortFiles()
    
  def sortFiles( self ):
    fileList = utility.getFiles( self.snippetDirectory, '.wav' )
    tonalities = np.zeros( ( len( fileList ), self.nNotes ) )
    for n in np.arange( len( fileList ) ):
      print (n*1.0)/len( fileList )
      audioData, fs = utility.getWavData( fileList[n] )
      assert fs == self.fs
      tonalities[n] = self.getTonality( audioData )
    fileFrequencies = {}
    for n in np.arange( self.nNotes ):
      tonalitiesForThisNote = tonalities[:,n]
      tonalitiesSort = np.argsort( tonalitiesForThisNote )[::-1]
      sortedIndex = 0
      targetHz = utility.midiToHz( self.baseNote + n )
      detectedHz = 1.0
      while ((targetHz/detectedHz) < .95 or (targetHz/detectedHz) > 1.05) and sortedIndex < tonalitiesSort.shape[0] and sortedIndex < 10:
        if not fileFrequencies.has_key( tonalitiesSort[sortedIndex] ):        
          audioData, fs = utility.getWavData( fileList[tonalitiesSort[sortedIndex]] )
          fileFrequencies[tonalitiesSort[sortedIndex]] = self.yinPitchDetect( audioData  )
        detectedHz = fileFrequencies[tonalitiesSort[sortedIndex]]
        print targetHz, detectedHz
        sortedIndex += 1
      if sortedIndex < len( fileList ):
        shutil.copy( fileList[tonalitiesSort[sortedIndex-1]], os.path.join( self.destinationDirectory, str(n + self.baseNote) + ".wav" ) )
      else:
        shutil.copy( fileList[tonalitiesSort[0]], os.path.join( self.destinationDirectory, str(n + self.baseNote) + ".wav" ) )
      
  def getTonality( self, audioData ):
    N = audioData.shape[0]
    tonalityScores = np.zeros( self.nNotes )
    AudioData = np.abs( np.fft.rfft( audioData ) )
    spectralCrestFactor = np.max( AudioData )/np.mean( AudioData )
    RMS = np.sqrt( np.sum( audioData**2.0 ) )/(N*1.0)
    for note in np.arange( self.nNotes ):
      mask = self.createMask( N, utility.midiToHz( self.baseNote + note ) )
      monophonicTonality = np.max( AudioData[mask == 1] )/np.mean( AudioData[mask == 0] )
      tonalityScores[note] = monophonicTonality*RMS
    return tonalityScores
      
  def createMask( self, N, baseFrequency ):
    nBins = N/2 - 1
    maxFrequency = utility.midiToHz( self.baseNote + self.nNotes + 24 )
    if maxFrequency > (self.fs*.9)/2.0:
      maxFrequency = (self.fs*.9)/2.0
    mask = np.zeros( nBins )
    for harmonic in baseFrequency*(2**np.arange( 4 )):
      if harmonic > maxFrequency:
        break
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
      f0 = np.argmin( squaredDifferenceNormalized )
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
  fileSorter( sys.argv[1], sys.argv[2] )