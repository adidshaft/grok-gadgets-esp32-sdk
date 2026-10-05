# Point the esp32s3-led-button environment at this example. PlatformIO 6.1
# ignores src_dir inside an [env:*] section, so the pre-script sets it.
import os

Import("env")

env.Replace(
    PROJECT_SRC_DIR=os.path.join(env.subst("$PROJECT_DIR"), "examples", "led-button")
)
