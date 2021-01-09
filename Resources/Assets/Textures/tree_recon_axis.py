import os
import glob
import numpy as np
from scipy import ndimage
from skimage.morphology import medial_axis
import matplotlib.pyplot as plt

from skimage import io
from skimage import util

files = glob.glob('./StreetView/*_0.jpg', recursive=True)
for f in files:
	os.remove(f)
files = glob.glob('./StreetView/*_1.jpg', recursive=True)
for f in files:
	os.remove(f)
files = glob.glob('./StreetView/*_3.jpg', recursive=True)
for f in files:
	os.remove(f)
files = glob.glob('./StreetView/*_4.jpg', recursive=True)
for f in files:
	os.remove(f)
files = glob.glob('./StreetView/*_5.jpg', recursive=True)
for f in files:
	os.remove(f)
