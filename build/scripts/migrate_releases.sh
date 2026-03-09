#!/usr/bin/env bash
set -euo pipefail

if [[ $# -eq 0 ]]; then
    echo "❌ Usage: $0 <path_to_release_version_folder>"
    exit 1
fi

TARGET_DIR="$1"

echo "🛠️  Fixing internal files in '$TARGET_DIR'..."

PROCESSED_COUNT=0

# Loop through folders that already use the new underscore format
for d in "$TARGET_DIR"/*/; do
    d="${d%/}"
    DIR_NAME="$(basename "$d")"

    # Only process directories that have underscores (the new format)
    if [[ "$DIR_NAME" == *"_"* ]]; then
        echo "📂 Checking: $DIR_NAME"

        # Split the directory name into parts using underscore as delimiter
        # Format: TIMESTAMP_VERSION_CHIPFAMILY_PROJECT_CHIP_PIN_TYPE_ORDER
        IFS='_' read -r TS VER FAMILY PROJ CHIP PIN TYPE ORDER <<< "$DIR_NAME"

        # Construct the target binary name
        # We omit the timestamp for the binary name to keep it clean
        NEW_BIN_NAME="${VER}_${FAMILY}_${PROJ}_${CHIP}_${PIN}_${TYPE}_${ORDER}.bin"

        BIN_DIR="${d}/binary"
        MANIFEST_FILE="${BIN_DIR}/manifest.json"

        if [[ -d "$BIN_DIR" ]]; then
            # 1. Rename the legacy .bin file
            # Looks for any bin that still has dashes and isn't firmware.bin
            for bin_path in "$BIN_DIR"/*.bin; do
                OLD_BIN_NAME="$(basename "$bin_path")"

                if [[ "$OLD_BIN_NAME" != "firmware.bin" && "$OLD_BIN_NAME" != "$NEW_BIN_NAME" ]]; then
                    echo "   ↳ 📄 Renaming $OLD_BIN_NAME -> $NEW_BIN_NAME"
                    mv "$bin_path" "${BIN_DIR}/${NEW_BIN_NAME}"

                    # 2. Patch manifest.json
                    if [[ -f "$MANIFEST_FILE" ]]; then
                        echo "   ↳ ⚙️  Patching manifest.json"

                        # Replace the path reference
                        sed -i.bak "s/${OLD_BIN_NAME}/${NEW_BIN_NAME}/g" "$MANIFEST_FILE"

                        # Replace the internal "name" field to match underscores
                        # Removes the 'pin' prefix if you want it clean, or keep it based on preference
                        NEW_MANIFEST_NAME="${PROJ}_${CHIP}_${PIN}_${TYPE}_${ORDER}"
                        sed -i.bak -E "s/\"name\": *\"[^\"]*\"/\"name\": \"${NEW_MANIFEST_NAME}\"/g" "$MANIFEST_FILE"

                        rm -f "${MANIFEST_FILE}.bak"
                    fi
                    PROCESSED_COUNT=$((PROCESSED_COUNT + 1))
                fi
            done
        fi
    fi
done

echo "✅ Successfully updated $PROCESSED_COUNT binaries and manifests."