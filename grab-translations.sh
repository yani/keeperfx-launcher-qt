#!/bin/bash

# Make sure $(pwd) is populated
if [[ -n "$(pwd)" ]]; then
    echo "Current directory: $(pwd)"
else
    echo "Failed to retrieve the current directory"
    exit 1
fi

# Grab translations
find "$(pwd)/ui/" -name "*.ui" -exec sh -c 'sed -n "/notr=\"true\"/!s/.*<string>\(.*\)<\/string>.*/_\(\"\1\"\);/p" {} > {}.tmp.cpp' \;
xgettext --omit-header -c++ -k_ --from-code=UTF-8 --escape -o "$(pwd)/i18n/ui.pot" $(find "$(pwd)/ui/" -name "*.ui.tmp.cpp")
find $(pwd)/ui/ -name "*.ui.tmp.cpp" -exec rm {} \;
xgettext --omit-header --qt -c++ -ktr --from-code=UTF-8 --escape -o "$(pwd)/i18n/translations.pot" "$(pwd)/i18n/qt.pot" $(find "$(pwd)/src/" -name "*.cpp") "$(pwd)/i18n/ui.pot"
rm $(pwd)/i18n/ui.pot

# Remove information about temporary files
sed -i 's|\.ui\.tmp\.cpp:[0-9]\+|.ui|g' "$(pwd)/i18n/translations.pot"

# Remove personal path from translation file
sed -i 's|'"$(pwd)"'||g' "$(pwd)/i18n/translations.pot"

# Count translations
count=$(grep -c 'msgid "' "$(pwd)/i18n/translations.pot")
echo "Translation strings: $count"

# Done
echo "Done"