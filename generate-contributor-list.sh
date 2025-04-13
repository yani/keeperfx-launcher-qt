#!/bin/bash

# Get output file
filePath="$(pwd)/res/contributors.txt"

# Download list of contributors from GitHub and write to file
curl -s "https://api.github.com/repos/dkfans/keeperfx/contributors" | jq -r '.[].login' > $filePath

# Show output
cat $filePath
echo ""
echo "Output saved to: $filePath"