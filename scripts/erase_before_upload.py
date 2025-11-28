Import("env")

# Run a full flash erase before uploading firmware to the board.
# This script will be invoked by PlatformIO as a pre-action for upload.

def do_erase(source, target, env):
    print("\n[erase_before_upload] Running full flash erase (pio run -t erase)...")
    # Use the PlatformIO CLI to run erase. This spawns a separate process.
    # It's safe and typically available in the user's environment.
    env.Execute("pio run -t erase")

# Attach the erase action before the 'upload' target
env.AddPreAction("upload", do_erase)
