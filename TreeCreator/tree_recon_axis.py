import os
import glob
import numpy as np
from scipy import ndimage
from skimage.morphology import medial_axis
import matplotlib.pyplot as plt

from skimage import io
from skimage import util

files = glob.glob('./tree_recon/**/skeleton.png', recursive=True)
for f in files:
	os.remove(f)
files = glob.glob('./tree_recon/**/*.png', recursive=True)
for f in files:
	image = io.imread(f)
	blue_channel = image[..., 1]
	# Compute the medial axis (skeleton) and the distance transform
	skel, distance = medial_axis(util.invert(blue_channel), return_distance=True)

	io.imsave(f.replace('target', 'skeleton'), skel * distance)