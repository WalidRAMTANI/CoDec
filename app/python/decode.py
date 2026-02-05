import os
import subprocess


input_folder = "../IMAGES_DIFS"     
output_folder = "DECODED_OUTPUT"   
codec_path = "./main"

def run_decoder():
    
    if not os.path.exists(input_folder):
        print(f"Error: Folder '{input_folder}' not found.")
        return

    if not os.path.exists(output_folder):
        os.makedirs(output_folder)
        print(f"Created output folder: {output_folder}")
    files = [f for f in os.listdir(input_folder) if f.endswith(".dif")]

    if not files:
        print(f"No .encoded files found in '{input_folder}'.")
        return

    print(f"Found {len(files)} encoded files. Starting decoding...")

    for filename in files:
        input_path = os.path.join(input_folder, filename)
        
        base_name = os.path.splitext(filename)[0]
        output_path = os.path.join(output_folder, base_name + ".pnm")
        command = [codec_path, "-d", input_path, output_path]
        
        try:
            print(f"Decoding: {input_path} -> {output_path}")
            subprocess.run(command, check=True)
        except subprocess.CalledProcessError as e:
            print(f"Failed to decode {filename}: {e}")
        except FileNotFoundError:
            print(f"Error: {codec_path} binary not found in the current directory.")
            break

    print(f"\nProcessing complete. Restored {len(files)} images to '{output_folder}'.")

if __name__ == "__main__":
    run_decoder()