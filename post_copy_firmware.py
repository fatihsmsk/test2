import shutil
import os

def copy_firmware(source, target, env):
    build_dir = os.path.join(env['PROJECT_DIR'], 'build')
    if not os.path.exists(build_dir):
        os.makedirs(build_dir)

    firmware_path = target[0].get_abspath()
    dest_path = os.path.join(build_dir, 'firmware.bin')

    if os.path.exists(firmware_path):
        shutil.copy(firmware_path, dest_path)
        print(f"[Post Action] Copied firmware to {dest_path}")
    else:
        print(f"[Post Action] firmware.bin not found at {firmware_path}")

Import("env")
env.AddPostAction("$BUILD_DIR/firmware.bin", copy_firmware)
