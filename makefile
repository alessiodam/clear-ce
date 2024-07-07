# ----------------------------
# Makefile Options
# ----------------------------

NAME = CLEAR
ICON = icon.png
DESCRIPTION = Clear. a web browser.

APP_NAME = Clear
APP_VERSION = 0

CFLAGS = -Wall -Wextra -Oz -I src/include
CXXFLAGS = -Wall -Wextra -Oz -Isrc/include
OUTPUT_MAP = YES

# ----------------------------

include app_tools/makefile
