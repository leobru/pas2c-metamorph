#!/bin/sh
# Shared environment for dispak-based build/test scripts.
# Source from other scripts:  . "$(dirname "$0")/env.sh"
#
# When a script sources this file, $0 is still that script's path, so
# dirname $0 is the dispak/ directory.

DISPAK_DIR="$(CDPATH= cd -- "$(dirname "$0")" && pwd)"
ROOT="$(CDPATH= cd -- "$DISPAK_DIR/.." && pwd)"
cd "$ROOT" || exit 1

# BESM6_PATH: cwd, then ~/.besm6 if present, then ../share/besm6 next to dispak.
besm6_path='.'
if [ -d "$HOME/.besm6" ]; then
    besm6_path="$besm6_path:$HOME/.besm6"
fi
_dispak_bin=$(command -v dispak 2>/dev/null || true)
if [ -n "$_dispak_bin" ]; then
    _share="$(CDPATH= cd -- "$(dirname "$_dispak_bin")/../share/besm6" 2>/dev/null && pwd || true)"
    if [ -n "$_share" ] && [ -d "$_share" ]; then
        besm6_path="$besm6_path:$_share"
    fi
fi
BESM6_PATH="$besm6_path"
export BESM6_PATH

# Volume numbers: 3000 + Dubna LUN (octal LUN written in decimal here).
VOL_BASE=3000
VOL_WORK=$((VOL_BASE + 41))    # 3041 -- work / tmpbin (LUN 41)
VOL_CCOM=$((VOL_BASE + 42))    # 3042 -- ccom          (LUN 42)
VOL_LIBC=$((VOL_BASE + 43))    # 3043 -- libc          (LUN 43)
VOL_SRC=$((VOL_BASE + 44))     # 3044 -- tmpsrc/wrksrc (LUN 44)
VOL_OUT=$((VOL_BASE + 67))     # 3067 -- writable out  (LUN 67)

# Passport cipher / КОНЕЦ trailer (Monitor-80 batch, UTF-8).
passport_header() {
    python3 "$DISPAK_DIR/passport.py" header "$@"
}

job_trailer() {
    python3 "$DISPAK_DIR/passport.py" trailer
}

# Ensure an empty volume image exists, then fill it from a host file.
write_vol() {
    # write_vol <volume> <host-file> [--length=N]
    _vol=$1
    _file=$2
    shift 2
    if [ ! -f "$_file" ]; then
        echo "write_vol: missing $_file" >&2
        return 1
    fi
    rm -f "$_vol"
    touch "$_vol"
    # Size the empty image to at least the source (zones of 6144).
    _bytes=$(wc -c < "$_file")
    _zones=$(( (_bytes + 6143) / 6144 ))
    [ "$_zones" -lt 1 ] && _zones=1
    # Leave headroom for writable volumes.
    _pad=$_zones
    case " $* " in
        *" --length="*) ;;
        *) _pad=$((_zones + 4)) ;;
    esac
    besmtool zero "$_vol" --length="$_pad" >/dev/null
    besmtool write "$_vol" --from-file="$_file" "$@" >/dev/null
}

# Dump a volume back to a host file (after a writable run).
dump_vol() {
    # dump_vol <volume> <host-file> [--length=N]
    _vol=$1
    _file=$2
    shift 2
    besmtool dump "$_vol" --to-file="$_file" "$@" >/dev/null
}

# Run a job file under dispak with UTF-8 input and a CPU cap.
# Usage: run_dispak <seconds> <jobfile> [output-file]
run_dispak() {
    _secs=$1
    _job=$2
    _out=${3:-}
    if [ -n "$_out" ]; then
        ( ulimit -t "$_secs"; exec dispak --input-encoding=utf8 -l "$_job" ) > "$_out"
        return $?
    fi
    ( ulimit -t "$_secs"; exec dispak --input-encoding=utf8 -l "$_job" )
}
