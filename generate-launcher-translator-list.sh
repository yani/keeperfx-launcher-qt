#!/usr/bin/env bash

set -euo pipefail

INPUT_FILE="$(pwd)/docs/translations.md"
OUTPUT_FILE="$(pwd)/res/launcher-translators.txt"

if [[ ! -f "$INPUT_FILE" ]]; then
    echo "Input file not found: $INPUT_FILE"
    exit 0
fi

# Remove existing output file if it exists
if [[ -f "$OUTPUT_FILE" ]]; then
    rm -f "$OUTPUT_FILE"
fi

awk -F'|' '
function trim(s) {
    gsub(/^[ \t\r\n]+|[ \t\r\n]+$/, "", s)
    return s
}

/^\|/ {
    # Skip header
    if ($2 ~ /^[[:space:]]*Language[[:space:]]*$/) next

    # Skip separator
    if ($0 ~ /^\|[-[:space:]]+\|/) next

    if (NF < 6) next

    translators = trim($6)
    if (translators == "") next

    n = split(translators, arr, ",")

    for (i = 1; i <= n; i++) {
        entry = trim(arr[i])
        if (entry == "") continue

        # Markdown link: [User](URL)
        if (match(entry, /^\[([^]]+)\]\(([^)]+)\)$/, m)) {
            output = m[1] "," m[2]
        } else {
            output = entry
        }

        if (!(output in seen)) {
            seen[output] = 1
            print output
        }
    }
}
' "$INPUT_FILE" > "$OUTPUT_FILE"

echo "Generated: $OUTPUT_FILE"
