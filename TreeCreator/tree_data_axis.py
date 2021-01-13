import os
import glob
import numpy as np
from scipy import ndimage
from skimage.morphology import medial_axis
import matplotlib.pyplot as plt
from skimage.transform import resize
from skimage import io
from skimage import util

files = glob.glob('./tree_data/mask_val/*.png', recursive=True)
for f in files:
	image = io.imread(f)
	blue_channel = image[..., 1]
	blue_channel = resize(blue_channel, (320, 320), anti_aliasing=False)
	# Compute the medial axis (skeleton) and the distance transform
	skel, distance = medial_axis(util.invert(blue_channel), return_distance=True)
	io.imsave(f.replace('mask_val', 'recon_skeleton_val'), skel * distance)