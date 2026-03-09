#!/usr/bin/env bash
set -euo pipefail

if [[ $# -eq 0 ]]; then
    echo "❌ Usage: $0 <path_to_releases_root>"
    echo "   Example: $0 ./static/firmware/releases"
    exit 1
fi

RELEASES_ROOT="$1"

echo "🧹 Starting Redundancy Cleanup (Removing redundant 'chip' field)..."

# 1. New Global Schema
NEW_SCHEMA="timestamp,version,chip_family,project_name,pin,led_strip_type,color_order"

# Iterate through each version folder (e.g., 1.4.008, 1.4.095)
for version_dir in "$RELEASES_ROOT"/*/; do
    version_dir="${version_dir%/}"
    [[ ! -d "$version_dir" ]] && continue

    VERSION_BASE="$(basename "$version_dir")"
    echo "📁 Processing Version: $VERSION_BASE"

    # --- Step A: Update Schema File ---
    SCHEMA_FILE="${version_dir}/file_name_key.csv"
    echo "$NEW_SCHEMA" > "$SCHEMA_FILE"
    echo "   📄 Updated schema key."

    # --- Step B: Process Build Folders ---
    for build_dir in "$version_dir"/*/; do
        build_dir="${build_dir%/}"
        DIR_NAME="$(basename "$build_dir")"

        # Check if this folder follows the 8-field underscore pattern
        # We count underscores: an 8-field string has 7 underscores
        IFS='_' read -r -a parts <<< "$DIR_NAME"

        if [[ ${#parts[@]} -eq 8 ]]; then
            # Extract parts, skipping the 5th element (index 4: chip)
            TS="${parts[0]}"
            VER="${parts[1]}"
            FAMILY="${parts[2]}"
            PROJ="${parts[3]}"
            # Skipping parts[4] (the redundant 'c3'/'s3' field)
            PIN="${parts[5]}"
            TYPE="${parts[6]}"
            ORDER="${parts[7]}"

            # Construct New Folder Name
            NEW_DIR_NAME="${TS}_${VER}_${FAMILY}_${PROJ}_${PIN}_${TYPE}_${ORDER}"
            NEW_BUILD_PATH="${version_dir}/${NEW_DIR_NAME}"

            echo "   📦 Migrating: $DIR_NAME"
            mv "$build_dir" "$NEW_BUILD_PATH"

            # --- Step C: Rename .bin and Update Manifest ---
            BIN_DIR="${NEW_BUILD_PATH}/binary"
            if [[ -d "$BIN_DIR" ]]; then

                # Binary naming usually omits the timestamp for brevity
                NEW_BASE_NAME="${VER}_${FAMILY}_${PROJ}_${PIN}_${TYPE}_${ORDER}"
                NEW_BIN_NAME="${NEW_BASE_NAME}.bin"

                for bin_file in "$BIN_DIR"/*.bin; do
                    if [[ -f "$bin_file" ]]; then
                        OLD_BIN_NAME="$(basename "$bin_file")"

                        # Only target the specific build binary, not firmware.bin
                        if [[ "$OLD_BIN_NAME" != "firmware.bin" && "$OLD_BIN_NAME" != "$NEW_BIN_NAME" ]]; then
                            mv "$bin_file" "${BIN_DIR}/${NEW_BIN_NAME}"
                            echo "      ↳ Renamed .bin to: $NEW_BIN_NAME"

                            # Patch manifest.json
                            MANIFEST="${BIN_DIR}/manifest.json"
                            if [[ -f "$MANIFEST" ]]; then
                                # 1. Update the binary path reference
                                sed -i.bak "s/${OLD_BIN_NAME}/${NEW_BIN_NAME}/g" "$MANIFEST"

                                # 2. Update the "name" field (Version_Family_Project_Pin_Type_Order)
                                sed -i.bak -E "s/\"name\": *\"[^\"]*\"/\"name\": \"${NEW_BASE_NAME}\"/g" "$MANIFEST"

                                rm -f "${MANIFEST}.bak"
                                echo "      ↳ Updated manifest.json"
                            fi
                        fi
                    fi
                done
            fi
        else
            # Skip files (like schema.csv) or folders already processed/not matching
            continue
        fi
    done
done

echo "✅ Redundancy cleanup complete for all releases."