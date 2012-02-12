# utility.py
# Various functions useful for song analysis
#
# Created by Colin Raffel on 10/7/10

import numpy as np
import wave
import csv
import os
import struct

def getWavData( wavFile ):
  # Get wav data
  wav = wave.open (wavFile, "r")
  (nChannels, sampleWidth, frameRate, nFrames, compressionType, compressionName) = wav.getparams()
  frames = wav.readframes( nFrames*nChannels )
  out = struct.unpack_from("%dh" % nFrames*nChannels, frames )
  
  # Convert 2 channles to numpy arrays
  if nChannels == 2:
    left = np.array( list( [out[i] for i in range( 0, len( out ), 2 )] ) )
    right = np.array( list( [out[i] for i in range( 1, len( out ), 2 )] ) )
  else:
    left = np.array( out )
    right = left
  audioData = (left + right)/2.0
  # Normalize
  audioData = (32767.0*audioData)/np.max(np.abs(audioData))
  return audioData, frameRate

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

# Convert midi note to Hz
def midiToHz( midiNote ):
  return 440.0*(2.0**((midiNote - 69)/12.0))

# Convert bin in FFT to Hz
def binToHz( bin, N, fs ):
  return fs*bin/(N*1.0)

# Return bins in an FFT which are close to a Hz value
def hzToBins( hz, N, fs, tolerance = 0.02 ):
  # Range near bins in tolerance range
  binRange = np.arange( (1.0 - tolerance)*hz*N/fs, (1.0 + tolerance)*hz*N/fs )
  # Convert arange to integer indices
  bins = np.array( np.round( binRange ), dtype = np.int )
  return bins