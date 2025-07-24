#!/bin/bash

# Stop execution on any error
set -e

# Check required gettext utilities
for tool in msgmerge msgfmt; do
    if ! command -v "$tool" &>/dev/null; then
        echo "[-] Required tool '$tool' is not installed or not in PATH."
        exit 1
    fi
done

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

        # Merge POT file into temporary PO file so we can get the current stats
        tmpMergedPo="$(mktemp)"
        msgmerge --quiet --no-fuzzy-matching "$poFile" "$translationTemplateFile" -o "$tmpMergedPo"

        # Use msgfmt to get translation statistics
        # The output of msgfmt --statistics goes to stderr, so capture it
        stats=$(msgfmt --statistics -o /dev/null "$tmpMergedPo" 2>&1)

        # Extract the number of translated messages from the output
        if [[ "$stats" =~ ([0-9]+)\ translated ]]; then
            translated="${BASH_REMATCH[1]}"
        else
            translated=0
        fi

        # Remove leftover temp file
        rm -f "$tmpMergedPo"

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
