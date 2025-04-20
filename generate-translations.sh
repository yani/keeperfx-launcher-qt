#!/bin/bash

# Make sure $(pwd) is populated
if [[ -n "$(pwd)" ]]; then
    echo "Current directory: $(pwd)"
else
    echo "Failed to retrieve the current directory"
    exit 1
fi

# Get binaries
lupdate_bin="/usr/lib/qt6/bin/lupdate"
lconvert_bin="/usr/lib/qt6/bin/lconvert"

# Check if "lupdate" bin exists
if [[ ! -x "$lupdate_bin" ]]; then
    echo "Error: $lupdate_bin not found or not executable."
    echo "Install it using: sudo apt install qt6-tools-dev-tools"
    exit 1
fi

# Check if "lconvert" bin exists
if [[ ! -x "$lconvert_bin" ]]; then
    echo "Error: $lconvert_bin not found or not executable."
    echo "Install it using: sudo apt install qt6-tools-dev-tools"
    exit 1
fi

# Grab translations
$lupdate_bin "$(pwd)/src" "$(pwd)/ui" -ts "$(pwd)/i18n/translations.ts"

# Convert translations to POT
$lconvert_bin -i "$(pwd)/i18n/translations.ts" -o "$(pwd)/i18n/translations.pot"

# Remove leftover translation file
rm "$(pwd)/i18n/translations.ts"

# Done
echo
echo "POT file: $(pwd)/i18n/translations.pot"
echo
echo "Done"