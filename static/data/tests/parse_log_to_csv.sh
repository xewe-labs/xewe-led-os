#!/bin/sh
set -eu

if [ "$#" -ne 1 ]; then
  echo "Usage: $0 path/to/input.txt" >&2
  exit 1
fi

infile=$1
case "$infile" in
  /*) ;;
  *) infile=$(cd "$(dirname "$infile")" && pwd)/$(basename "$infile") ;;
esac

if [ ! -f "$infile" ]; then
  echo "File not found: $infile" >&2
  exit 1
fi

indir=$(dirname "$infile")
base=$(basename "$infile")
name=${base%.*}
outfile="$indir/${name}_cleaned.csv"

tmpfile=$(mktemp)
trap 'rm -f "$tmpfile"' EXIT HUP INT TERM

awk -F',' '
BEGIN {
  OFS=",";
  mode="";
  saw_header=0;
  test_id=""; meta_chipset=""; meta_protocol=""; meta_spi_mhz=""; meta_leds=""; meta_run="";
}

function trim(s) {
  sub(/^[[:space:]]+/, "", s);
  sub(/[[:space:]]+$/, "", s);
  return s;
}

function reset_meta() {
  test_id=""; meta_chipset=""; meta_protocol=""; meta_spi_mhz=""; meta_leds=""; meta_run="";
}

function is_uint(s) { return s ~ /^[0-9]+$/ }
function is_num(s)  { return s ~ /^[0-9]+(\.[0-9]+)?$/ }

function print_header_v1() {
  print "test_id","chipset","protocol","leds","run","frames","elapsed_us","avg_render_us","avg_show_us","avg_total_us","fps";
}

function print_header_v2() {
  print "test_id","chipset","protocol","spi_mhz","leds","run","frames","elapsed_us","avg_render_us","avg_show_us","avg_show_us_per_led","avg_total_us","fps";
}

function maybe_print_header(new_mode) {
  if (!saw_header) {
    if (new_mode == "v1") print_header_v1();
    else if (new_mode == "v2") print_header_v2();
    saw_header=1;
    mode=new_mode;
  }
}

{
  line=$0;
  sub(/\r$/, "", line);

  if (line ~ /^[[:space:]]*SUMMARY([[:space:]]|$)/) next;
  if (line ~ /^[[:space:]]*total_frames:/) next;
  if (line ~ /^[[:space:]]*mean_render_us:/) next;
  if (line ~ /^[[:space:]]*mean_show_us:/) next;
  if (line ~ /^[[:space:]]*mean_show_us_per_led:/) next;
  if (line ~ /^[[:space:]]*mean_total_us:/) next;
  if (line ~ /^[[:space:]]*effective_fps:/) next;
  if (line ~ /^[[:space:]]*Next test id:/) next;

  if (line ~ /^[[:space:]]*test_id:[[:space:]]*[0-9]+[[:space:]]*$/) {
    split(line, a, ":");
    test_id=trim(a[2]);
    next;
  }
  if (line ~ /^[[:space:]]*chipset:[[:space:]]*/) {
    sub(/^[[:space:]]*chipset:[[:space:]]*/, "", line);
    meta_chipset=trim(line);
    next;
  }
  if (line ~ /^[[:space:]]*protocol:[[:space:]]*/) {
    sub(/^[[:space:]]*protocol:[[:space:]]*/, "", line);
    meta_protocol=trim(line);
    next;
  }
  if (line ~ /^[[:space:]]*spi_mhz:[[:space:]]*[0-9]+[[:space:]]*$/) {
    split(line, a, ":");
    meta_spi_mhz=trim(a[2]);
    next;
  }
  if (line ~ /^[[:space:]]*leds:[[:space:]]*[0-9]+[[:space:]]*$/) {
    split(line, a, ":");
    meta_leds=trim(a[2]);
    next;
  }
  if (line ~ /^[[:space:]]*run:[[:space:]]*[0-9]+\/[0-9]+[[:space:]]*$/) {
    split(line, a, ":");
    split(trim(a[2]), b, "/");
    meta_run=b[1];
    next;
  }

  n = split(line, f, ",");
  for (i = 1; i <= n; i++) f[i]=trim(f[i]);

  if (n == 10 \
      && f[1] != "chipset" \
      && is_uint(f[3]) && is_uint(f[4]) && is_uint(f[5]) && is_uint(f[6]) \
      && is_num(f[7]) && is_num(f[8]) && is_num(f[9]) && is_num(f[10])) {

    maybe_print_header("v1");
    if (mode != "v1") next;
    if (test_id != "" && meta_chipset != "" && meta_protocol != "" && meta_leds != "" && meta_run != "") {
      if (f[1] == meta_chipset && f[2] == meta_protocol && f[3] == meta_leds && f[4] == meta_run) {
        print test_id, f[1], f[2], f[3], f[4], f[5], f[6], f[7], f[8], f[9], f[10];
      }
    } else {
      print "", f[1], f[2], f[3], f[4], f[5], f[6], f[7], f[8], f[9], f[10];
    }
    reset_meta();
    next;
  }

  if (n == 12 \
      && f[1] != "chipset" \
      && is_uint(f[3]) && is_uint(f[4]) && is_uint(f[5]) && is_uint(f[6]) && is_uint(f[7]) \
      && is_num(f[8]) && is_num(f[9]) && is_num(f[10]) && is_num(f[11]) && is_num(f[12])) {

    maybe_print_header("v2");
    if (mode != "v2") next;
    if (test_id != "" && meta_chipset != "" && meta_protocol != "" && meta_spi_mhz != "" && meta_leds != "" && meta_run != "") {
      if (f[1] == meta_chipset && f[2] == meta_protocol && f[3] == meta_spi_mhz && f[4] == meta_leds && f[5] == meta_run) {
        print test_id, f[1], f[2], f[3], f[4], f[5], f[6], f[7], f[8], f[9], f[10], f[11], f[12];
      }
    } else {
      print "", f[1], f[2], f[3], f[4], f[5], f[6], f[7], f[8], f[9], f[10], f[11], f[12];
    }
    reset_meta();
    next;
  }
}
' "$infile" > "$tmpfile"

if [ ! -s "$tmpfile" ]; then
  echo "No valid data rows found in: $infile" >&2
  exit 1
fi

mv "$tmpfile" "$outfile"
printf '%s\n' "$outfile"
