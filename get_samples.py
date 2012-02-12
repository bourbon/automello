#!/usr/bin/python

"""
	get_samples.py -- Automatically extracts samples from a given path to an audio file or directory of audio files
	                  for each MIDI note (within a reasonable range) and writes them to a destination directory.
"""

import sys, os, random, tempfile, shutil

import echonest.audio as audio
import echonest.sorting as sorting
from echonest.selection import fall_on_the
import numpy as np

from monophonic_tonality_sorter import MonophonicTonalitySorter

VALID_FILETYPES = ['mp3', 'wav', 'm4a']		 # which filetypes we care about when finding audio in a directory
SAMPLES_PER_FILE = 50				 		 # default number of samples to get per file
MINIMUM_SAMPLE_LENGTH = 0.18				 # minimum length of a sample

def split_into_segments(path, destination_dir, selection_filter=None, num_segments=None, output_prefix=""):	
	if not selection_filter:
		selection_filter = lambda l, n: l[:n]
	if not num_segments:
		num_segments = SAMPLES_PER_FILE
	
	print "Getting candidate samples from %s" % path
	
	# Load audio file & get Echo Nest anlysis
	audio_file = audio.LocalAudioFile(path)

	# Get the segments from the Echo Nest analysis
	segments = audio_file.analysis.segments
	
	# Filter out all segments less than MINIMUM_SAMPLE_LENGTH
	segments = segments.that(lambda x: x.duration > MINIMUM_SAMPLE_LENGTH)
	
	# Grab all the segments and filter them with the given selection_filter
	segments = selection_filter(segments, num_segments)
	
	for index, segment in enumerate(segments):
		path = os.path.join(destination_dir, "%s%s.wav" % (output_prefix, str(index)))
		print "Writing out candidate sample: %s" % path
		
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
	
	# Get the destination directory for candidates
	candidate_destination_dir = tempfile.mkdtemp()
	
	if len(sys.argv) < 4:
		num_segments = SAMPLES_PER_FILE
	else:
		num_segments = int(sys.argv[3])
	
	# A bunch of different filters
	random_filter = lambda l, n: random.sample(l, n)
	longest_filter = lambda l, n: l.ordered_by(sorting.duration, descending=True)[:n]
	least_noisy_filter = lambda l, n: l.ordered_by(sorting.noisiness)[:n] # noisiness = sum of twelve pitch vectors elements
	random_selection_of_least_noisy_filter = lambda l, n: random.sample(l.ordered_by(sorting.noisiness)[:int(0.2 * len(l))], n)
	least_confidence_filter = lambda l, n: l.ordered_by(sorting.confidence)[:n]
	
	# Get candidate samples
	for path in paths:
		try:
			split_into_segments(path=path, destination_dir=candidate_destination_dir, selection_filter=least_noisy_filter, output_prefix=os.path.split(path[:-4])[1], num_segments=num_segments)
		except Exception, e:
			print "ERROR: Couldn't get candidate samples for %s: %s" % (path, str(e))
	
	# Find the best samples per note out of the candidates
	MonophonicTonalitySorter(candidate_destination_dir, destination_dir)
	
	# Clean up
	shutil.rmtree(candidate_destination_dir)
	
	print "\nSamples written to %s" % (destination_dir)
