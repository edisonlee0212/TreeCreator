import os
import glob
import numpy as np
from scipy import ndimage
from skimage.morphology import medial_axis
import matplotlib.pyplot as plt

from skimage import io
from skimage import util

files = glob.glob('./tree_real/skeleton_recon/*.png', recursive=True)
for f in files:
	os.remove(f)
files = glob.glob('./tree_real/mask_real_small/*.png', recursive=True)
for f in files:
	image = io.imread(f)
	blue_channel = image[..., 1]
	# Compute the medial axis (skeleton) and the distance transform
	skel, distance = medial_axis(util.invert(blue_channel), return_distance=True)

	io.imsave(f.replace('mask_real_small', 'skeleton_recon'), skel * distance)