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

# Check if xgettext is installed
if ! command -v xgettext &> /dev/null; then
    echo "Error: xgettext not found."
    echo "Install it using: sudo apt install gettext"
    exit 1
fi

# Check if msgfmt is installed
if ! command -v msgfmt &> /dev/null; then
    echo "Error: msgfmt not found."
    echo "Install it using: sudo apt install gettext"
    exit 1
fi

# Grab translations
$lupdate_bin -locations none "$(pwd)/src" "$(pwd)/ui" -ts "$(pwd)/i18n/translations_temp.ts" > /dev/null

# Convert translations to POT
$lconvert_bin -i "$(pwd)/i18n/translations_temp.ts" -o "$(pwd)/i18n/translations_temp.pot" > /dev/null

# Combine POT files into one translations file
xgettext -o "$(pwd)/i18n/translations.pot" "$(pwd)/i18n/qt.pot" "$(pwd)/i18n/translations_temp.pot" > /dev/null

# Remove leftover translation files
rm "$(pwd)/i18n/translations_temp.ts" > /dev/null
rm "$(pwd)/i18n/translations_temp.pot" > /dev/null

# Count the number of translation strings
translation_count=$(grep -c '^msgid "' "$(pwd)/i18n/translations.pot")
translation_count=$((translation_count - 1))

# Done
echo "Total translation strings: $translation_count"
echo "Output POT file: $(pwd)/i18n/translations.pot"
