# utility.py
# Various functions useful for song analysis
#
# Created by Colin Raffel on 10/7/10

import numpy as np
import scipy.io.wavfile as wavfile
import csv
import os
import scipy.signal as signal

def getWavData( wavFile ):
  # Get wav data
  fs, audioData = wavfile.read(wavFile)
  # Convert to mono
  if (len(audioData.shape) > 1) and (audioData.shape[1] > 1):
    audioData = np.mean( audioData, axis=1 )
  # Normalize
  audioData = (32767.0*audioData)/np.max(np.abs(audioData))
  return audioData, fs

# Returns a list of annotations in a text file  
def getBeats( file ):
  # Try loading the beats
  try:
    beats = np.genfromtxt( file )
  # Numpy couldn't load
  except:
    print "Error reading the file, returning empty beat list"
    return np.array([])
  # Some annotation files have two columns
  if beats.ndim == 2:
    beats = beats[:,1]
  # Some annotation files have "end" at the end
  if np.isnan( beats[-1] ):
    beats = beats[:-1]
  return beats

# Get files of a certain type in a directory, recursively
def getFiles( path, extension ):
  fileList = []
  for root, subdirectories, files in os.walk( path ):
    for file in files:
      # Only get files of the given type
      if os.path.splitext(file)[1] == extension:
        fileList.append(os.path.join(root, file))
  return fileList

# Get subdirectories for a given directory
def getSubdirectories( path ):
  dirList = []
  for root, subdirectories, files in os.walk( path ):
    for dir in subdirectories:
      dirList.append( os.path.join( root, dir ) )
  return dirList

# Get annotation file paths/names
def getAnnotations( directory ):
  namePrefix = os.path.split( directory )[1]
  rawPath = os.path.join( directory, namePrefix + "-footraw.txt" )
  predictPath = os.path.join( directory, namePrefix + "-footpredict.txt" )
  handCuesPath = os.path.join( directory, namePrefix + "-handcues.txt" )
  return getBeats( rawPath ), getBeats( predictPath ), getBeats( handCuesPath )

# Get tempo and confidence
def getTempoFromAnnotations( beats ):
  # Compute inner-beat-intervals
  innerBeatIntervals = np.diff( beats )
  # Mean -> tempo
  tempo = 60.0/np.mean( innerBeatIntervals )
  return tempo

def toMono( audioData ):
  # For now, sum left and right channels
  audioData = (audioData[::2] + audioData[1::2])/2.0
  return audioData
  
# Down sample a signal by taking the mean over frames
def downsampleMean( data, frameSize ):
  nFrames = np.floor(data.shape[0]/frameSize)
  dataDownsampled = np.zeros(nFrames)
  for n in np.arange(nFrames):
    dataDownsampled[n] = np.mean(data[n*frameSize:(n+1)*frameSize])
  return dataDownsampled

def downsampleMax( data, frameSize ):
  nFrames = np.floor(data.shape[0]/frameSize)
  dataDownsampled = np.zeros(nFrames+1)
  #for n in np.arange(nFrames):
  #  currentMax = np.max(data[n*frameSize:(n+1)*frameSize])
  #  currentMin = np.min(data[n*frameSize:(n+1)*frameSize])
  #  if currentMax > np.abs(currentMin):
  #    dataDownsampled[n] = currentMax
  #  else:
  #    dataDownsampled[n] = currentMin
  for n in np.arange(nFrames):
    dataDownsampled[n] = np.max(np.abs(data[n*frameSize:(n+1)*frameSize]))

  if data.shape[0] > nFrames*frameSize:
    dataDownsampled[nFrames] = np.max(data[nFrames*frameSize:])
  
  alternate = np.tile(np.array([1, -1]), dataDownsampled.shape[0]/2)
  if alternate.shape[0] < dataDownsampled.shape[0]:
    alternate = np.append(alternate, np.array([1]))
  dataDownsampled = dataDownsampled*alternate
  return dataDownsampled
  
# Calculate geometric mean for lage arrays of large values
def geometricMean( data ):
  N = float(data.shape[0])
  largestValue = np.max( data )
  frameSize = 2
  nFrames = np.floor(N/2)
  geometricMean = 1.0
  for n in np.arange( nFrames ):
    geometricMean = geometricMean * np.power( data[2*n]*data[2*n+1], 1/N )
  if np.floor(N/2) != N/2:
    geometricMean = geometricMean * np.power( data[-1], 1/N )
  return geometricMean

# Split a signal into frames
def splitSignal( x, hop, frameSize ):
  nFrames = np.floor( (x.shape[0]-frameSize)/(1.0*hop) )
  # Pre-allocate matrix
  xSplit = np.zeros( (nFrames, frameSize) )
  for n in np.arange(nFrames):
    xSplit[n] = x[n*hop:n*hop+frameSize]
  return xSplit

# Parabolic interpolation
def parabolicInterpolate( signal, i ):
  # Make sure we aren't out of boundsi
  if i > 0 and i < signal.shape[0] - 1:
    offset = (signal[i+1] - signal[i-1])/(2.0*(2.0*signal[i] - signal[i+1] - signal[i-1]))
  # Otherwrise don't calculate offset
  else:
    offset = 0
  return i + offset

def roundArray( input ):
  output = "["
  for value in input:
    output += "%.3f, " % value
  output =  output[:-2] + "]"
  return output

def midiToHz( midiNote ):
  return 440.0*(2.0**((midiNote - 69)/12.0))

def binToHz( bin, N, fs ):
  return fs*bin/(N*1.0)

def hzToBins( hz, N, fs, tolerance = 0.02 ):
  binRange = np.arange( (1.0 - tolerance)*hz*N/fs, (1.0 + tolerance)*hz*N/fs )
  bins = np.array( np.round( binRange ), dtype = np.int )
  return bins