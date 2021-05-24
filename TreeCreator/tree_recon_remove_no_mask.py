import os
import glob
import numpy as np
from scipy import ndimage
from skimage.morphology import medial_axis
import matplotlib.pyplot as plt
import shutil
from skimage import io
from skimage import util

files = glob.glob('./tree_recon/**/no_mask', recursive=True)
for f in files:
	shutil.rmtree(f)