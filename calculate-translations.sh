#!/bin/bash

# Stop execution on any error
set -e

# Variables
translationTemplateFile="$(dirname "$0")/i18n/translations.pot"
translationPoFileMask="$(dirname "$0")/i18n/translations_*.po"
translationDocumentOutputFile="$(dirname "$0")/docs/translations.md"

# Calculate total strings
totalStrings=$(msgattrib --no-obsolete "$translationTemplateFile" | grep -c '^msgid ')
totalStrings=$((totalStrings - 1)) # Remove 1 because of the metadata msgid

# Make sure we got some strings
if [ "$totalStrings" -le 0 ]; then echo "[-] No strings found in POT file."; exit 1; fi

# Show total strings
echo "[+] Total translation strings in POT file: $totalStrings"

# Declare arrays for temporary storing the translation statistics
declare -A poStats_done poStats_percent

# Loop trough PO files
for poFile in $translationPoFileMask; do
    [[ ! -f "$poFile" ]] && continue
    if [[ "$(basename "$poFile")" =~ translations_([a-zA-Z0-9\-]+)\.po$ ]]; then

        # Get language code
        code="${BASH_REMATCH[1]}"; code="${code^^}"

        # Count translated strings
        translated=$(msgattrib --no-fuzzy --translated --no-obsolete "$poFile" | grep -c '^msgid ')
        [[ "$translated" -gt 0 ]] && translated=$((translated - 1)) # Remove 1 because of metadata msgid

        # Get formatted percentage and remove decimals on round numbers
        percent=$(awk "BEGIN { p=($translated/$totalStrings)*100; printf(\"%.1f\", p) }")
        percent_int=$(awk "BEGIN { printf(\"%d\", $percent) }")
        [[ "$percent" = "$percent_int.0" ]] && percent="$percent_int"

        # Remember translation statistics
        poStats_done["$code"]="$translated"
        poStats_percent["$code"]="$percent"
        echo "[+] $(printf '%-7s' "$code:")	$percent%	$translated/$totalStrings"
    fi
done

# Make sure we actually did something
if [ "${#poStats_done[@]}" -eq 0 ]; then echo "[-] No calculations done"; exit 1; fi

# Start updating the translation document
echo "[>] Updating translation document..."

# Create a tmpfile
# We do this so any sed errors don't break the original file
tmpfile="$(mktemp)"
cp "$translationDocumentOutputFile" "$tmpfile"

# Loop trough translations
for code in "${!poStats_done[@]}"; do

    # Get strings
    percent_padded=$(printf "%-10s" "${poStats_percent[$code]}%")
    count_padded=$(printf "%-15s" "${poStats_done[$code]}/$totalStrings")

    # Update the lines of the translation table
    # Use # as the sed delimiter to avoid conflict with / in $count
    sed -i -E \
      "s#^(\|[^\|]*\|\s*)$code(\s*\|)[^\|]*(\|)[^\|]*#\1$code\2 $percent_padded\3 $count_padded#" \
      "$tmpfile"
done

# Replace original file with tmpfile
mv "$tmpfile" "$translationDocumentOutputFile"

# Success
echo "[+] Done!"