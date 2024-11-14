import os
import coffee_grinder as cg

stream = open("binary_output.bin", 'rb')

# Read the entire stream video
video = cg.read_video(stream)

# Access the list of pictures
pictures = video.pictures

# Get DCT coefficients of frame 2 in macroblock 3
dct_coeffs =  pictures[2].subpictures[0].slices[0].macroblocks[2].dct_coeffs

# Reorganize the extracted DCT of luma component as a matrix 
dct_coeffs_y_matrix = cf.unpack_dct_coefficients(dct_coeffs[0].plane, dct_coeffs[0].values)