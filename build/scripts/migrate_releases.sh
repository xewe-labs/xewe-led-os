#!/usr/bin/env bash
set -euo pipefail

if [[ $# -eq 0 ]]; then
    echo "❌ Usage: $0 <path_to_release_version_folder>"
    echo "   Example: $0 ./static/firmware/releases/1.4.008"
    exit 1
fi

TARGET_DIR="$1"

if [[ ! -d "$TARGET_DIR" ]]; then
    echo "❌ Directory not found: $TARGET_DIR"
    exit 1
fi

echo "🔍 Scanning '$TARGET_DIR' for legacy folders..."

# Regex to safely extract parts from: 20260303-091130-1.4.008-ESP32-C3-xewe-led-os-pin-0
# Group 1: Timestamp (e.g., 20260303-091130)
# Group 2: Version (e.g., 1.4.008)
# Group 3: Chip Family (e.g., ESP32-C3)
# Group 4: Project Name (e.g., xewe-led-os)
# Group 5: Pin Number (e.g., 0)
LEGACY_REGEX="^([0-9]{8}-[0-9]{6})-([0-9]+\.[0-9]+\.[0-9]+)-(ESP32-[a-zA-Z0-9]+)-(.+)-pin-([0-9]+)$"

PROCESSED_COUNT=0

# Iterate through directories only
for d in "$TARGET_DIR"/*/; do
    # Strip trailing slash for cleaner basename
    d="${d%/}"
    DIR_NAME="$(basename "$d")"

    if [[ "$DIR_NAME" =~ $LEGACY_REGEX ]]; then
        TIMESTAMP="${BASH_REMATCH[1]}"
        VERSION="${BASH_REMATCH[2]}"
        CHIP_FAMILY="${BASH_REMATCH[3]}"
        PROJECT_NAME="${BASH_REMATCH[4]}"
        PIN="${BASH_REMATCH[5]}"

        # Extract short chip name (e.g., "c3" from "ESP32-C3")
        CHIP="$(echo "$CHIP_FAMILY" | cut -d'-' -f2 | tr '[:upper:]' '[:lower:]')"

        # Inject known defaults for the legacy format
        LED_STRIP_TYPE="WS2115"
        COLOR_ORDER="RGB"

        # Construct new underscore-delimited string
        NEW_NAME="${TIMESTAMP}_${VERSION}_${CHIP_FAMILY}_${PROJECT_NAME}_${CHIP}_pin${PIN}_${LED_STRIP_TYPE}_${COLOR_ORDER}"
        NEW_PATH="$(dirname "$d")/$NEW_NAME"

        echo "  📦 Converting: $DIR_NAME"
        echo "      ↳ To: $NEW_NAME"

        # Rename the folder
        mv "$d" "$NEW_PATH"
        PROCESSED_COUNT=$((PROCESSED_COUNT + 1))

        # Check for and rename the associated .bin file
        if [[ -d "${NEW_PATH}/binary" ]]; then
            for bin_file in "${NEW_PATH}/binary"/*.bin; do
                if [[ -f "$bin_file" ]]; then
                    bin_name="$(basename "$bin_file")"
                    # Replace the old folder string with the new string inside the binary name
                    new_bin_name="${bin_name/$DIR_NAME/$NEW_NAME}"
                    if [[ "$bin_name" != "$new_bin_name" ]]; then
                        mv "$bin_file" "${NEW_PATH}/binary/$new_bin_name"
                        echo "      ↳ Renamed binary: $new_bin_name"
                    fi
                fi
            done
        fi
    else
        # Silently skip file_name_key.csv and already-processed folders
        if [[ ! "$DIR_NAME" == *"_"* ]]; then
            echo "  ⏭️  Skipping: $DIR_NAME (Does not match legacy format)"
        fi
    fi
done

echo "✅ Processed $PROCESSED_COUNT legacy folders."

# ---------- Generate Schema Key ----------
SCHEMA_FILE="${TARGET_DIR}/file_name_key.csv"
echo "timestamp,version,chip_family,project_name,chip,pin,led_strip_type,color_order" > "$SCHEMA_FILE"
echo "📄 Generated schema key at: $SCHEMA_FILE"

echo "🎉 Migration complete!"