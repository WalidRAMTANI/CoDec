import os
import subprocess


input_folder = "../IMAGES_TESTS"
output_folder = "ENCODED_RESULTS" 
codec_path = "./main"

def run_codec():
    # Check if the input folder exists
    if not os.path.exists(input_folder):
        print(f"Error: Folder '{input_folder}' not found.")
        return

    
    if not os.path.exists(output_folder):
        os.makedirs(output_folder)
        print(f"Created output folder: {output_folder}")


    files = [f for f in os.listdir(input_folder) if os.path.isfile(os.path.join(input_folder, f))]

    if not files:
        print("No files found in the directory.")
        return

    print(f"Found {len(files)} files. Starting encoding...")

    for filename in files:
        input_path = os.path.join(input_folder, filename)
        
        # 1. Define the base name (yacht)
        base_name = os.path.splitext(filename)[0]
        output_path = os.path.join(output_folder, base_name + ".encoded")
        command = [codec_path, "-c", input_path, output_path]
        
        try:
            print(f"Encoding: {filename} -> {output_path}")
            subprocess.run(command, check=True)
        except subprocess.CalledProcessError as e:
            print(f"Failed to encode {filename}: {e}")
        except FileNotFoundError:
            print(f"Error: {codec_path} not found. Make sure it is in the current directory.")
            break

    print(f"Processing complete. {len(files)} files processed.")

if __name__ == "__main__":
    run_codec()