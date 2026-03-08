#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

# Check if a target directory was provided
if [ -z "$1" ]; then
    echo "Usage: $0 <path-to-src>"
    echo "Example: $0 src/"
    exit 1
fi

TARGET_DIR="$1"

# Ensure target directory exists
if [ ! -d "$TARGET_DIR" ]; then
    echo "Error: Directory '$TARGET_DIR' does not exist."
    exit 1
fi

# The exact PolyForm License block
LICENSE_BLOCK="/*********************************************************************************
 * SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI
 *
 * Licensed under PolyForm Noncommercial 1.0.0 + No AI Use Addendum v1.0.
 * See: LICENSE and LICENSE-NO-AI.md in the project root for full terms.
 *
 * Required Notice: Copyright 2025 Maxim Dokukin (https://maxdokukin.com)
 * https://github.com/maxdokukin/xewe-led-os
 *********************************************************************************/"

echo "Scanning for .h and .cpp files in '$TARGET_DIR'..."

# Find all .h and .cpp files recursively
find "$TARGET_DIR" -type f \( -name "*.h" -o -name "*.cpp" \) | while read -r FILE; do

    # 1. Calculate relative path (assumes script is run from project root)
    # This strips the leading './' if you pass a path like './src'
    REL_PATH=$(echo "$FILE" | sed 's|^./||')

    # 2. Enforce the 2-line minimum rule for empty functions
    # Using Perl to find cases of `) {}` or `){ }` (specifically targeting functions)
    # and expanding them to multiple lines. Perl is cross-platform for in-place edits.
    perl -pi -e 's/\)\s*\{\s*\}/\) {\n    \/\/ empty\n}/g' "$FILE"

    # 3. Check for the License Header
    # We look for the unique SPDX identifier. If missing, we prepend the license and path.
    if ! grep -q "SPDX-License-Identifier: LicenseRef-PolyForm-NC-1.0.0-NoAI" "$FILE"; then
        echo "Injecting license and path into: $FILE"

        TMP_FILE=$(mktemp)

        # Write the license block and the dynamic relative path
        echo "$LICENSE_BLOCK" > "$TMP_FILE"
        echo "// $REL_PATH" >> "$TMP_FILE"
        echo "" >> "$TMP_FILE" # Blank line for spacing

        # Append the original file's content
        cat "$FILE" >> "$TMP_FILE"

        # Overwrite the original file safely
        mv "$TMP_FILE" "$FILE"
    else
        # If the license already exists, we use awk to update/ensure the relative path
        # is correct immediately following the end of the comment block.
        # (This avoids duplicate paths if the file is moved).
        awk -v path="// $REL_PATH" '
            /^\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\*\/\s*$/ {
                print;
                print path;
                inserted=1;
                next
            }
            inserted && /^\/\/ / { inserted=0; next } # Skip the old path comment
            {print}
        ' "$FILE" > "$FILE.tmp" && mv "$FILE.tmp" "$FILE"
    fi

done

echo "Boilerplate formatting complete."