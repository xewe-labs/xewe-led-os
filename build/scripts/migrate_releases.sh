#!/usr/bin/env bash
set -euo pipefail

if [[ $# -eq 0 ]]; then
    echo "❌ Usage: $0 <path_to_releases_root>"
    echo "   Example: $0 ./static/firmware/releases"
    exit 1
fi

RELEASES_ROOT="$1"

echo "🔄 Migrating CSV schemas to meta.json..."

# Process each version directory (e.g., 1.4.008)
for version_dir in "$RELEASES_ROOT"/*/; do
    version_dir="${version_dir%/}"
    [[ ! -d "$version_dir" ]] && continue

    VERSION_BASE="$(basename "$version_dir")"
    echo "📂 Processing Version: $VERSION_BASE"

    # 1. Delete the legacy CSV schema if it exists
    SCHEMA_CSV="${version_dir}/file_name_key.csv"
    if [[ -f "$SCHEMA_CSV" ]]; then
        rm "$SCHEMA_CSV"
        echo "   🗑️  Removed legacy file_name_key.csv"
    fi

    # 2. Process each build folder within this version
    for build_dir in "$version_dir"/*/; do
        build_dir="${build_dir%/}"
        DIR_NAME="$(basename "$build_dir")"

        # Split the directory name into an array using underscore
        IFS='_' read -r -a parts <<< "$DIR_NAME"

        # Check if it matches our 7-field clean schema
        if [[ ${#parts[@]} -eq 7 ]]; then
            TS_SHORT="${parts[0]}"
            VER="${parts[1]}"
            FAMILY="${parts[2]}"
            PROJ="${parts[3]}"
            PIN_RAW="${parts[4]}"
            TYPE="${parts[5]}"
            ORDER="${parts[6]}"

            # Clean the "pin" prefix if it exists (e.g., "pin0" -> "0")
            PIN="${PIN_RAW#pin}"

            # Reconstruct an ISO-like timestamp from TS_SHORT (YYYYMMDD-HHMMSS)
            # Format: YYYY-MM-DDTHH:MM:SSZ
            if [[ "$TS_SHORT" =~ ^([0-9]{4})([0-9]{2})([0-9]{2})-([0-9]{2})([0-9]{2})([0-9]{2})$ ]]; then
                TS_ISO="${BASH_REMATCH[1]}-${BASH_REMATCH[2]}-${BASH_REMATCH[3]}T${BASH_REMATCH[4]}:${BASH_REMATCH[5]}:${BASH_REMATCH[6]}Z"
            else
                TS_ISO=""
            fi

            BIN_DIR="${build_dir}/binary"
            if [[ -d "$BIN_DIR" ]]; then
                META_JSON="${BIN_DIR}/meta.json"

                # Generate the JSON file
                cat > "$META_JSON" <<EOF
{
  "timestamp": "${TS_SHORT}",
  "timestamp_iso": "${TS_ISO}",
  "version": "${VER}",
  "chip_family": "${FAMILY}",
  "project_name": "${PROJ}",
  "pin": "${PIN}",
  "led_strip_type": "${TYPE}",
  "color_order": "${ORDER}"
}
EOF
                echo "   ✅ Created meta.json for: $DIR_NAME"
            fi
        else
            # Skip folders that don't match the expected field count (like the release notes or unknown dirs)
            if [[ "$DIR_NAME" != "binary" ]]; then
                echo "   ⏭️  Skipping non-conforming directory: $DIR_NAME"
            fi
        fi
    done
done

echo "🎉 Migration to meta.json complete!"