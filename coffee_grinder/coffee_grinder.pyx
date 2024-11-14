#cython: language_level=3
#cython: infer_types=True

from io import BufferedReader
import struct

import cython
from libc.stdint cimport int8_t, uint8_t, int16_t, uint16_t, uint32_t, int32_t

import numpy as np

cdef class DCTCoeffs:
    cdef public uint8_t plane
    cdef public uint16_t num_values
    cdef public list[int32_t] values
    def __init__(self, uint8_t plane, uint16_t num_values, list[int32_t] values):
        self.plane = plane
        self.num_values = num_values
        self.values = values

cdef class MotionVector:
    cdef public uint8_t mv_list
    cdef public uint8_t ref_idx
    cdef public int16_t ref_frame_id
    cdef public int16_t diff_x
    cdef public int16_t diff_y
    cdef public int16_t abs_x
    cdef public int16_t abs_y
    def __init__(self, uint8_t mv_list, uint8_t ref_idx, int16_t ref_frame_id, int16_t diff_x, int16_t diff_y, int16_t abs_x, int16_t abs_y):
        self.mv_list = mv_list
        self.ref_idx = ref_idx
        self.ref_frame_id = ref_frame_id
        self.diff_x = diff_x
        self.diff_y = diff_y
        self.abs_x = abs_x
        self.abs_y = abs_y

cdef class Macroblock:
    cdef public uint32_t num
    cdef public uint32_t x_coo
    cdef public uint32_t y_coo
    cdef public int8_t qp_y
    cdef public int8_t qp_u
    cdef public int8_t qp_v
    cdef public uint8_t mb_type

    cdef public list[MotionVector] motion_vectors

    cdef public tuple[DCTCoeffs, DCTCoeffs, DCTCoeffs] dct_coeffs

    def __init__(self, uint32_t num, uint32_t x_coo, uint32_t y_coo, int8_t qp_y, int8_t qp_u, int8_t qp_v, uint8_t mb_type,  list[MotionVector] motion_vectors, tuple[DCTCoeffs, DCTCoeffs, DCTCoeffs] dct_coeffs):
        self.num = num
        self.x_coo = x_coo
        self.y_coo = y_coo
        self.qp_y = qp_y
        self.qp_u = qp_u
        self.qp_v = qp_v
        self.mb_type = mb_type
        self.motion_vectors = motion_vectors
        self.dct_coeffs = dct_coeffs

cdef class Slice:
    cdef public uint16_t num
    cdef public uint8_t slice_type

    cdef public list[Macroblock] macroblocks

    def __init__(self, uint16_t num, uint8_t slice_type, list[Macroblock] macroblocks):
        self.num = num
        self.slice_type = slice_type
        self.macroblocks = macroblocks


cdef class SubPicture:
    cdef public uint8_t structure

    cdef public list[Slice] slices

    def __init__(self, uint8_t structure, list[Slice] slices):
        self.structure = structure
        self.slices = slices

cdef class Picture:
    cdef public uint32_t picture_id
    cdef public uint32_t poc
    cdef public uint32_t gop_num

    cdef public list[SubPicture] subpictures

    def __init__(self, uint32_t picture_id, uint32_t poc, uint32_t gop_num, list[SubPicture] subpictures):
        self.picture_id = picture_id
        self.poc = poc
        self.gop_num = gop_num
        self.subpictures = subpictures


cdef class Video:
    cdef public list[Picture] pictures

    def __init__(self, list[Picture] pictures):
        self.pictures = pictures


cdef DCTCoeffs read_dct(stream):
    cdef uint8_t plane
    cdef uint16_t num_values
    cdef int32_t coeff
    cdef list[int32_t] coeffs

    # unpack = struct.unpack
    read = stream.read

    cdef bytes data = read(3)
    cdef const uint8_t[::1] view1 = data
    plane = view1[0]
    num_values = view1[1] | (view1[2] << 8)
    #plane, num_values = unpack('<BH', read(3))

    coeffs = [0] * num_values

    if num_values == 0:
        if plane == 76:
            return DCTCoeffs(plane, 256, coeffs)
        else:
            return DCTCoeffs(plane, 64, coeffs)

    data = read(4 * num_values)
    cdef const uint8_t[::1] view2 = data

    for i in range(num_values):
        coeff = <int32_t> (view2[0 + i * 4] | \
                (view2[1 + i * 4] << 8) | \
                (view2[2 + i * 4] << 16) | \
                (view2[3] << 24))
        #coeff, = unpack('<i', read(4))
        coeffs[i] = coeff

    return DCTCoeffs(plane, num_values, coeffs)


cdef MotionVector read_motion_vector(stream):
    mv_list, ref_idx, ref_frame_id, diff_x, diff_y, abs_x, abs_y = struct.unpack('<bbhhhhh', stream.read(12))

    return MotionVector(mv_list, ref_idx, ref_frame_id, diff_x, diff_y, abs_x, abs_y)


cdef Macroblock read_macroblock(stream):
    cdef uint32_t num
    cdef uint8_t marker
    cdef uint32_t x_coo
    cdef uint32_t y_coo
    cdef int8_t qp_y
    cdef int8_t qp_u
    cdef int8_t qp_b
    cdef uint8_t mb_type

    unpack = struct.unpack
    read = stream.read

    cdef list motion_vectors = []

    num, marker = unpack('<IB', read(5))

    while marker == 0:
        motion_vectors.append(read_motion_vector(stream))
        # count += 1
        marker, = unpack('<B', read(1))

    x_coo, y_coo, qp_y, qp_u, qp_v, mb_type = unpack('<IIbbbB', read(12))

    cdef tuple[DCTCoeffs, DCTCoeffs, DCTCoeffs] dct_coeffs = (
        read_dct(stream),
        read_dct(stream),
        read_dct(stream),
    )

    return Macroblock(num, x_coo, y_coo, qp_y, qp_u, qp_v, mb_type, motion_vectors, dct_coeffs)


cdef Slice read_slice(stream):
    num, slice_type = struct.unpack('<HB', stream.read(3))

    macroblocks = []

    marker, = struct.unpack('<B', stream.read(1))
    while marker == 0:
        macroblocks.append(read_macroblock(stream))
        marker, = struct.unpack('<B', stream.read(1))

    return Slice(num, slice_type, macroblocks)


cdef SubPicture read_subpicture(stream):
    structure, = struct.unpack('<B', stream.read(1))

    slices = []

    marker, = struct.unpack('<B', stream.read(1))
    while marker == 0:
        slices.append(read_slice(stream))
        marker, = struct.unpack('<B', stream.read(1))

    return SubPicture(structure, slices)


cpdef Picture read_picture(stream):
    picture_id, poc, gop_num = struct.unpack('<III', stream.read(4 * 3))

    subpictures = []

    marker, = struct.unpack('<B', stream.read(1))
    while marker == 0:
        subpictures.append(read_subpicture(stream))
        marker, = struct.unpack('<B', stream.read(1))

    return Picture(picture_id, poc, gop_num, subpictures)


cpdef Video read_video(stream):
    pictures = []
    while len(stream.peek()) > 0:
        pictures.append(read_picture(stream))
    return Video(pictures)


cpdef Video read_video_until_frameid(stream, frame_id: int):
    pictures = []
    print("Reading pictures...")
    pic = read_picture(stream)
    while pic.picture_id <= frame_id:
        pictures.append(pic)
        pic = read_picture(stream)
    return Video(pictures)


def get_subpicture_structure_type(structure):
    if structure == 70:
        return 'Frame'
    elif structure == 84:
        return 'TopField'
    elif structure == 66:
        return 'BottomField'
    else:
        raise ValueError('Invalid Structure Type!')


def get_slice_type(slice_type):
    if slice_type == 73 or slice_type == 76:
        return 'SliceI'
    elif slice_type == 80 or slice_type == 81:
        return 'SliceP'
    elif slice_type == 66:
        return 'SliceB'
    else:
        raise ValueError('Invalid Slice Type!')


def get_macroblock_type(slice_type, mb_type):
    if slice_type == 73 or slice_type == 76 or slice_type == 'SliceI':
        if mb_type == 48:
            return 'i_4x4', '-'
        elif mb_type == 49:
            return 'i_8x8', '-'
        elif mb_type == 0:
            return 'si', '-'
        elif mb_type == 1:
            return 'i_16x16', '0_0_0'
        elif mb_type == 2:
            return 'i_16x16', '1_0_0'
        elif mb_type == 3:
            return 'i_16x16', '2_0_0'
        elif mb_type == 4:
            return 'i_16x16', '3_0_0'
        elif mb_type == 5:
            return 'i_16x16', '0_1_0'
        elif mb_type == 6:
            return 'i_16x16', '1_1_0'
        elif mb_type == 7:
            return 'i_16x16', '2_1_0'
        elif mb_type == 8:
            return 'i_16x16', '3_1_0'
        elif mb_type == 9:
            return 'i_16x16', '0_2_0'
        elif mb_type == 10:
            return 'i_16x16', '1_2_0'
        elif mb_type == 11:
            return 'i_16x16', '2_2_0'
        elif mb_type == 12:
            return 'i_16x16', '3_2_0'
        elif mb_type == 13:
            return 'i_16x16', '0_0_1'
        elif mb_type == 14:
            return 'i_16x16', '1_0_1'
        elif mb_type == 15:
            return 'i_16x16', '2_0_1'
        elif mb_type == 16:
            return 'i_16x16', '3_0_1'
        elif mb_type == 17:
            return 'i_16x16', '0_1_1'
        elif mb_type == 18:
            return 'i_16x16', '1_1_1'
        elif mb_type == 19:
            return 'i_16x16', '2_1_1'
        elif mb_type == 20:
            return 'i_16x16', '3_1_1'
        elif mb_type == 21:
            return 'i_16x16', '0_2_1'
        elif mb_type == 22:
            return 'i_16x16', '1_2_1'
        elif mb_type == 23:
            return 'i_16x16', '2_2_1'
        elif mb_type == 24:
            return 'i_16x16', '3_2_1'
        elif mb_type == 25:
            return 'i_pcm', '-'
        else:
            raise ValueError('Invalid Macroblock Type!')

    elif slice_type == 80 or slice_type == 81 or slice_type == 'SliceP':
        if mb_type == 0:
            return 'p_16x16', 'l0'
        elif mb_type == 1:
            return 'p_16x8', 'l0_l0'
        elif mb_type == 2:
            return 'p_8x16', 'l0_l0'
        elif mb_type == 3 or mb_type == 4:
            return 'p_8x8'
        elif mb_type == 5:
            return 'si', '-'
        elif mb_type == 6:
            return 'i_16x16', '0_0_0'
        elif mb_type == 7:
            return 'i_16x16', '1_0_0'
        elif mb_type == 8:
            return 'i_16x16', '2_0_0'
        elif mb_type == 9:
            return 'i_16x16', '3_0_0'
        elif mb_type == 10:
            return 'i_16x16', '0_1_0'
        elif mb_type == 11:
            return 'i_16x16', '1_1_0'
        elif mb_type == 12:
            return 'i_16x16', '2_1_0'
        elif mb_type == 13:
            return 'i_16x16', '3_1_0'
        elif mb_type == 14:
            return 'i_16x16', '0_2_0'
        elif mb_type == 15:
            return 'i_16x16', '1_2_0'
        elif mb_type == 16:
            return 'i_16x16', '2_2_0'
        elif mb_type == 17:
            return 'i_16x16', '3_2_0'
        elif mb_type == 18:
            return 'i_16x16', '0_0_1'
        elif mb_type == 19:
            return 'i_16x16', '1_0_1'
        elif mb_type == 20:
            return 'i_16x16', '2_0_1'
        elif mb_type == 21:
            return 'i_16x16', '3_0_1'
        elif mb_type == 22:
            return 'i_16x16', '0_1_1'
        elif mb_type == 23:
            return 'i_16x16', '1_1_1'
        elif mb_type == 24:
            return 'i_16x16', '2_1_1'
        elif mb_type == 25:
            return 'i_16x16', '3_1_1'
        elif mb_type == 26:
            return 'i_16x16', '0_2_1'
        elif mb_type == 27:
            return 'i_16x16', '1_2_1'
        elif mb_type == 28:
            return 'i_16x16', '2_2_1'
        elif mb_type == 29:
            return 'i_16x16', '3_2_1'
        elif mb_type == 30:
            return 'i_pcm', '-'
        elif mb_type == 48:
            return 'i_4x4', '-'
        elif mb_type == 49:
            return 'i_8x8', '-'
        elif mb_type == 50:
            return 'p_skip', '-'
        else:
            raise ValueError('Invalid Macroblock Type!')

    elif slice_type == 66 or slice_type == 'SliceB':
        if mb_type == 0:
            return 'b_16x16', 'direct'
        elif mb_type == 1:
            return 'b_16x16', 'l0'
        elif mb_type == 2:
            return 'b_16x16', 'l1'
        elif mb_type == 3:
            return 'b_16x16', 'bi'
        elif mb_type == 4:
            return 'b_16x8', 'l0_l0'
        elif mb_type == 6:
            return 'b_16x8', 'l1_l1'
        elif mb_type == 8:
            return 'b_16x8', 'l0_l1'
        elif mb_type == 10:
            return 'b_16x8', 'l1_l0'
        elif mb_type == 5:
            return 'b_8x16', 'l0_l0'
        elif mb_type == 7:
            return 'b_8x16', 'l1_l1'
        elif mb_type == 9:
            return 'b_8x16', 'l0_l1'
        elif mb_type == 11:
            return 'b_8x16', 'l1_l0'
        elif mb_type == 12:
            return 'b_16x8', 'l0_bi'
        elif mb_type == 14:
            return 'b_16x8', 'l1_bi'
        elif mb_type == 13:
            return 'b_8x16', 'l0_bi'
        elif mb_type == 15:
            return 'b_8x16', 'l1_bi'
        elif mb_type == 16:
            return 'b_16x8', 'bi_l0'
        elif mb_type == 18:
            return 'b_16x8', 'bi_l1'
        elif mb_type == 17:
            return 'b_8x16', 'bi_l0'
        elif mb_type == 19:
            return 'b_8x16', 'bi_l1'
        elif mb_type == 20:
            return 'b_16x8', 'bi_bi'
        elif mb_type == 21:
            return 'b_8x16', 'bi_bi'
        elif mb_type == 22:
            return 'b_8x8', '-'
        elif mb_type == 23:
            return 'si', '-'
        elif mb_type == 24:
            return 'i_16x16', '0_0_0'
        elif mb_type == 25:
            return 'i_16x16', '1_0_0'
        elif mb_type == 26:
            return 'i_16x16', '2_0_0'
        elif mb_type == 27:
            return 'i_16x16', '3_0_0'
        elif mb_type == 28:
            return 'i_16x16', '0_1_0'
        elif mb_type == 29:
            return 'i_16x16', '1_1_0'
        elif mb_type == 30:
            return 'i_16x16', '2_1_0'
        elif mb_type == 31:
            return 'i_16x16', '3_1_0'
        elif mb_type == 32:
            return 'i_16x16', '0_2_0'
        elif mb_type == 33:
            return 'i_16x16', '1_2_0'
        elif mb_type == 34:
            return 'i_16x16', '2_2_0'
        elif mb_type == 35:
            return 'i_16x16', '3_2_0'
        elif mb_type == 36:
            return 'i_16x16', '0_0_1'
        elif mb_type == 37:
            return 'i_16x16', '1_0_1'
        elif mb_type == 38:
            return 'i_16x16', '2_0_1'
        elif mb_type == 39:
            return 'i_16x16', '3_0_1'
        elif mb_type == 40:
            return 'i_16x16', '0_1_1'
        elif mb_type == 41:
            return 'i_16x16', '1_1_1'
        elif mb_type == 42:
            return 'i_16x16', '2_1_1'
        elif mb_type == 43:
            return 'i_16x16', '3_1_1'
        elif mb_type == 44:
            return 'i_16x16', '0_2_1'
        elif mb_type == 46:
            return 'i_16x16', '1_2_1'
        elif mb_type == 47:
            return 'i_16x16', '2_2_1'
        elif mb_type == 48:
            return 'i_16x16', '3_2_1'
        elif mb_type == 49:
            return 'i_pcm', '-'
        elif mb_type == 51:
            return 'i_4x4', '-'
        elif mb_type == 52:
            return 'i_8x8', '-'
        elif mb_type == 53:
            return 'b_skip', '-'
        else:
            raise ValueError('Invalid Macroblock Type!')

    else:
        raise ValueError('Invalid Slice Type!')



def get_dct_plane(plane):
    if plane == 76:
        return 'luma'
    elif plane == 66:
        return 'chroma_blue'
    elif plane == 82:
        return 'chroma_red'
    else:
        raise ValueError('Invalid Plane Type!')

def unpack_dct_coefficients(plane, values):
    unpacked_values = []
    if len(values) == 0:
        if plane == 76:
            return np.zeros((16,16))
        elif plane == 66 or plane == 82:
            return np.zeros((8,8))
        else:
            raise ValueError("Invalid Plane Type!")
    i = 0
    while i < len(values):
        if values[i] == 0:
            zeros = [0] * values[i + 1]
            unpacked_values += zeros
            i += 2
        else:
            unpacked_values.append(values[i])
            i += 1
    if plane == 76:
        return np.array(unpacked_values).reshape((16, 16))
    elif plane == 66 or plane == 82:
        return np.array(unpacked_values).reshape((8, 8))
    else:
        raise ValueError("Invalid Plane Type!")
