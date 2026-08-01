import shutil
import os

from SCons.Script import Import

Import("env")  # Import the build environment from PlatformIO

# This function will be called after the build is complete
def copy_firmware(source, target, env):
    # Get the path to the built firmware file
    firmware_path = str(target[0])
    
    # Define your destination directory
    dest_dir = os.path.join(env["PROJECT_DIR"], "firmware")
    os.makedirs(dest_dir, exist_ok=True)
    
    # Destination file path
    dest_file = os.path.join(dest_dir, os.path.basename(firmware_path))
    
    try:
        shutil.copy(firmware_path, dest_file)
        print(f"[Post-Build] Copied firmware to: {dest_file}")
    except Exception as e:
        print(f"[Post-Build] Error copying firmware: {e}")

# Register the post-build action
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", copy_firmware)