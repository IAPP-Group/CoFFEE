import os
import argparse
import glob
import shutil
import subprocess
import tempfile
import zipfile
import json
from colorama import Fore
from tqdm import tqdm 

def get_parser():
    parser = argparse.ArgumentParser()
    parser.add_argument('-j', '--jm_bin_path', type=str, default='bin', help="Location of the directory containing ldecod.exe and decoder.cfg files of JM")
    parser.add_argument('-i', '--videos_directory', required=True, type=str, help='Location of the directory containing the videos in .mp4, .MOV, etc. format')
    parser.add_argument('-o', '--output_path', type=str, default='outputs', help='Location to store the processed data')
    parser.add_argument('-c', '--uncompressed_bin', action='store_false', default=True, help='If True, binary_output.bin will be stored as a .zip file.')
    parser.add_argument('-n', '--json_filename', type=str, default='video_paths.json', help='Name of the json file to store the video to process.')
    return parser


def generate_file_video_path(videos_directory, json_filename):
    videos = []
    for root, _, files in os.walk(videos_directory):
        for file in files:
            if file.lower().endswith('.mp4') or file.lower().endswith('.mov'): 
                video_class = os.path.basename(root)  
                video_path = os.path.join(root, file)
                videos.append([video_path, video_class])
                
    with open(json_filename, 'w') as file:
        json.dump(videos, file, indent=2)
    

def generate_h264(video_path, video_class, output_path):
    # check if some video in the directory are already in H.264 format
    existing_h264 = glob.glob(os.path.join(output_path, video_class, "*.h264"))
    
    video_name = os.path.splitext(os.path.basename(video_path))[0]
    output_video = os.path.join(output_path, video_class, video_name+".h264")
    
    if output_video in existing_h264:
        print(Fore.BLUE + f"{output_video} already exists. Skipping h264 generation." + Fore.RESET)
        return output_video

    cmd = ["ffmpeg", "-hide_banner", "-loglevel", "error", "-i", video_path, "-vcodec", "copy", "-an", "-bsf:v", "h264_mp4toannexb", output_video]
    subprocess.run(cmd)
        
    return output_video

def generate_binary_file(video_h264_path, video_class, output_path, compress_bin):
     # check if some videos in the directory have already been decoded
    os.makedirs(os.path.join(output_path, video_class, 'BINARY_OUTPUTS'), exist_ok=True)
    existing_zip = glob.glob(os.path.join(output_path, video_class, "BINARY_OUTPUTS", "*.zip"))
    existing_bin = glob.glob(os.path.join(output_path, video_class, "BINARY_OUTPUTS", "*.bin"))
    
    video_name = os.path.splitext(os.path.basename(video_h264_path))[0]

    if os.path.join(output_path, video_class, 'BINARY_OUTPUTS', f'{video_name}_binary_output.zip') in existing_zip or os.path.join(output_path, video_class, 'BINARY_OUTPUTS', f'{video_name}_binary_output.bin') in existing_bin:
        print(Fore.BLUE + f"{os.path.join(output_path, video_class, 'BINARY_OUTPUTS', f'{video_name}_binary_output.zip')} already exists. Skipping binary generation." + Fore.RESET)
        return

    # DECODE VIDEOS

    with tempfile.TemporaryDirectory(dir=os.path.join(os.getcwd(), 'Temp')) as temp_path:
        shutil.copy(os.path.join(os.path.abspath(args.jm_bin_path), "decoder.cfg"), temp_path)

        # generating binary file
        cmd =[os.path.join(os.path.abspath(args.jm_bin_path), 'ldecod.exe'), '-i', video_h264_path]
        subprocess.run(cmd, cwd=temp_path, shell=False)

        if compress_bin:
            zip_path = compress_file(os.path.join(temp_path, "binary_output.bin"), video_name=video_name)
            shutil.move(zip_path, os.path.join(output_path, video_class, "BINARY_OUTPUTS"))
            
        else:
            file_path = os.path.join(temp_path, "binary_output.bin")
            new_file_path = os.path.join(temp_path, f"{video_name}_binary_output.bin")
            os.rename(file_path, new_file_path)
            shutil.move(new_file_path, os.path.join(output_path, video_class, "BINARY_OUTPUTS"))
                

def compress_file(file_path, video_name):
    temp_path, binary_filename = os.path.split(file_path)
    zip_path = os.path.join(temp_path, f"{video_name}_binary_output.zip")
    with zipfile.ZipFile(zip_path, 'w', compression=zipfile.ZIP_DEFLATED) as zip_file:
        zip_file.write(file_path, arcname=os.path.basename(file_path))
    return zip_path


if __name__ == '__main__':
    parser = get_parser()
    args = parser.parse_args()
    
    # Generate a .txt file containing the video paths to be processed
    generate_file_video_path(args.videos_directory, args.json_filename)
    
    with open(f'{os.path.join(os.getcwd(), args.json_filename)}', 'r') as file:
        videos = json.load(file)
    
    os.makedirs(args.output_path, exist_ok=True)
    class_set = list({item[1] for item in videos})

    for video_class in class_set:
        os.makedirs(os.path.join(args.output_path, video_class), exist_ok=True)
        
    for video in tqdm(videos, desc="Processing videos", bar_format=f'\033[36m{{l_bar}}{{bar}}\033[0m{{r_bar}}'):
        video_path = video[0]
        video_class = video[1]
        video_h264_path = generate_h264(video_path, video_class, args.output_path)
        video_bin_path = generate_binary_file(os.path.abspath(video_h264_path), video_class, args.output_path, args.uncompressed_bin)
        
        