# CoFFEE: A Codec-based Forensic Feature Extraction and Evaluation Software for H. 264 Videos
This is the official repository of the paper: [CoFFEE: A Codec-based Forensic Feature Extraction and Evaluation Software for H. 264 Videos](https://link.springer.com/article/10.1186/s13635-024-00181-4)

Giulia Bertazzini, Daniele Baracchi, Dasara Shullani, Massimo Iuliani, and Alessandro Piva.

<p align="center">
<img src="images/coffee.png" alt="coffee pipeline" width="40%"/>
</p>

## Abstract
The forensic analysis of digital videos is becoming increasingly relevant to deal with forensic cases, propaganda, and fake news. The research community has developed numerous forensic tools to address various challenges, such as integrity verification, manipulation detection, and source characterization. Each tool exploits characteristic traces to reconstruct the video life-cycle. Among these traces, a significant source of information is provided by the specific way in which the video has been encoded. While several tools are available to analyze codec-related information for images, a similar approach has been overlooked for videos, since video codecs are extremely complex and involve the analysis of a huge amount of data.
To this end, we present **CoFFEE**, a new tool designed for extracting and parsing a plethora of video compression information from H.264 encoded files. 
It consists of two main modules:
- **CoFFEE roaster**: this module extracts codec information from H.264 bitstreams. It optimizes the JM reference software by storing the extracted features in a binary format, which significantly reduce both runtime and storage requirements.
- **CoFFEE grinder**: this module allows easy access to the extracted features for further analysis.

<p align="center">
<img src="images/coffee_pipeline.png" alt="coffee pipeline" width="90%"/>
</p>

## Requirements
```
conda create -n coffee python
conda activate coffee
pip install -r requirements.txt
```

Then, install [FFmpeg](https://www.ffmpeg.org/download.html).

## Usage
### 1. JM 19.0 Setup
To compile JM 19.0, navigate to the ``ldecod`` folder of JM and run the ``make`` command. 
This will generate a bin folder containing the **ldecod.exe** file. Please, move this bin folder into the ldecod directory. Additionally, ensure that the configuration file **decoder.cfg** is placed inside the ldecod/bin folder for proper setup.

The JM folder structure should be as follows:
```
jm_19/ldecod
├── bin
│   ├── ldecod.exe
│   ├── decoder.cfg
├── ...
```

### 2. CoFFEE Roaster
Please, make sure to have **FFMPEG** installed. 

To extract binary files from a video dataset, run the following command:
`python coffee_roaster/extract_data.py --jm_bin_path --videos_directory --output_path --uncompressed_bin --json_filename`

where ``--jm_bin_path`` specifies the directory containing the ldecod.exe and decoder.cfg files from JM. The default is the bin directory; ``--videos_directory`` specifies the directory where the videos (in formats such as .mp4, .MOV, etc.) are located; ``--output_path`` defines the location to store the processed data. The default is the ``outputs`` directory; ``--uncompressed_bin`` is a flag that controls whether the binary output (binary_output.bin) should be stored as a .zip file. If set to False, the output will be uncompressed. By default, the value is True;  ``--json_filename`` specifies the name of the JSON file that will store the list of videos to be processed. The default is ``video_paths.json``.

Example usage:
```
python coffee_roaster/extract_data.py -i /path/to/videos/directory -o /path/to/outputs/directory -j /path/to/jm/bin -n my_videos.json
```
### 3. CoFFEE Grinder
Build the library using ``python setup.py build_ext --inplace`` if you want to compile the library locally for development and testing without installing it globally, or ``python setup.py install`` to install the library into your environment.

Once the build is complete, you can create a Python file where you can import the coffee_grinder library and use its functionality. 

Example of usage:

```
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
dct_coeffs_y_matrix = cg.unpack_dct_coefficients(dct_coeffs[0].plane, dct_coeffs[0].values)
```

For all the available methods of the library, please consult our [paper](https://link.springer.com/article/10.1186/s13635-024-00181-4).

## License
CoFFEE code (which includes coffee_roaster and coffee_grinder modules) is licensed under the GNU Affero General Public License v3.0 - see the LICENSE file for details.

JM 19.0 code is under its original license. 

## Citation
If you find this work useful for your research, please cite our paper:
```
@article{bertazzini2024coffee,
  title={CoFFEE: a codec-based forensic feature extraction and evaluation software for H. 264 videos},
  author={Bertazzini, Giulia and Baracchi, Daniele and Shullani, Dasara and Iuliani, Massimo and Piva, Alessandro},
  journal={EURASIP Journal on Information Security},
  volume={2024},
  number={1},
  pages={34},
  year={2024},
  publisher={Springer}
}
```