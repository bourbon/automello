#!/usr/bin/python

"""
	get_samples.py -- Extracts samples from a given path to an audio file or directory of audio files
	                  and writes them to a destination directory.
"""

import sys, os, random

import echonest.audio as audio
import echonest.sorting as sorting
from echonest.selection import fall_on_the
import numpy as np

VALID_FILETYPES = ['mp3', 'wav', 'm4a']		 # which filetypes we care about when finding audio in a directory
NUM_SEGMENTS = 10				 			 # default number of segments to get per file

def get_audio_file(path):
	return audio.LocalAudioFile(path)

def split_into_segments(audio_file, destination_dir, selection_filter=None, num_segments=None, output_prefix=""):	
	if not selection_filter:
		selection_filter = lambda l, n: l[:n]
	if not num_segments:
		num_segments = NUM_SEGMENTS
	
	# Grab all the segments and filter them with the given selection_filter
	segments = selection_filter(audio_file.analysis.segments, num_segments)
	
	for index, segment in enumerate(segments):
		path = os.path.join(destination_dir, "%s%s.wav" % (output_prefix, str(index)))
		print "Writing out sample: %s" % path
		
		# Get AudioData object for segment
		sample = audio.getpieces(audio_file, [segment])
		
		# Normalize it
		sample.data = np.int16(sample.data / float(np.max(np.abs(sample.data))) * np.iinfo(np.int16).max)
		
		# Write it out to disk as wav
		sample.encode(path, mp3=False)

	
if __name__ == '__main__':
	if len(sys.argv) < 2:
		print "usage: python get_samples.py SOURCE_PATH DEST_DIR [SAMPLES_PER_FILE]"
		sys.exit(0)
	
	# Figure out which files to analyze
	path = sys.argv[1]
	if os.path.exists(path) and not os.path.isdir(path):
		paths = [path]
	elif os.path.isdir(path):
		paths = []
		for root, dirs, files in os.walk(path): 
			paths.extend([os.path.join(root, f) for f in files])
		paths = [x for x in paths if x[-3:].lower() in VALID_FILETYPES]
	else:
		raise OSError("Couldn't find anything at path %s" % (path))
		
	# Get the destination directory
	if len(sys.argv) < 3:
		destination_dir = os.path.join(os.path.expanduser('~'), 'Desktop', "samples")
	else:
		destination_dir = sys.argv[2]
	if not os.path.exists(destination_dir):
		os.makedirs(destination_dir)
	print destination_dir
	
	if len(sys.argv) < 4:
		num_segments = NUM_SEGMENTS
	else:
		num_segments = int(sys.argv[3])
	
	# A bunch of different filters
	random_filter = lambda l, n: random.sample(l, n)
	longest_filter = lambda l, n: l.ordered_by(sorting.duration, descending=True)[:n]
	least_noisy_filter = lambda l, n: l.ordered_by(sorting.noisiness)[:n] # noisiness = sum of twelve pitch vectors elements
	random_selection_of_least_noisy_filter = lambda l, n: random.sample(l.ordered_by(sorting.noisiness)[:int(0.2 * len(l))], n)
	least_confidence_filter = lambda l, n: l.ordered_by(sorting.confidence)[:n]
	
	# Do it!
	for path in paths:
		print "Working on %s" % path
		split_into_segments(audio_file=get_audio_file(path), destination_dir=destination_dir, selection_filter=least_noisy_filter, output_prefix=os.path.split(path[:-4])[1], num_segments=num_segments)
	
