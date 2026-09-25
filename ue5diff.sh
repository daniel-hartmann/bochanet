#!/bin/bash
# Visual diff driver for Unreal Engine binary assets (.uasset/.umap), using
# the engine's own built-in asset diff tool.
#
# Works as both:
#   - a git diff driver  (git calls it with 7 args: path old-file old-hex old-mode new-file new-hex new-mode)
#   - a git difftool cmd (git calls it with 2 args: $LOCAL $REMOTE)
#
# Configuration (see setup-ue5-diff.sh for the one-time per-clone install):
#   UE_EDITOR_PATH   optional override: full path to the Unreal Editor binary
#   UPROJECT_PATH    optional override: full path to the .uproject file
set -uo pipefail

die() {
    echo "ue5diff: $*" >&2
    exit 1
}

REPO_ROOT="$(git rev-parse --show-toplevel 2>/dev/null)" || die "not inside a git repository"

# --- Resolve the .uproject file -------------------------------------------
UPROJECT_PATH="${UPROJECT_PATH:-}"
if [ -z "$UPROJECT_PATH" ]; then
    UPROJECT_PATH="$(find "$REPO_ROOT" -maxdepth 4 -iname '*.uproject' -print -quit)"
fi
[ -n "$UPROJECT_PATH" ] && [ -f "$UPROJECT_PATH" ] || die "couldn't find a .uproject file under $REPO_ROOT (set UPROJECT_PATH to override)"

ENGINE_VERSION="$(grep -o '"EngineAssociation"[[:space:]]*:[[:space:]]*"[^"]*"' "$UPROJECT_PATH" | sed -E 's/.*:[[:space:]]*"([^"]*)"/\1/')"

# --- Resolve the Unreal Editor binary ---------------------------------------
UE_EDITOR_PATH="${UE_EDITOR_PATH:-}"

detect_os() {
    case "$(uname -s)" in
        Darwin) echo mac ;;
        Linux) echo linux ;;
        MINGW*|MSYS*|CYGWIN*) echo windows ;;
        *) echo unknown ;;
    esac
}
OS="$(detect_os)"

editor_binary_for_install() {
    # $1 = engine install root
    case "$OS" in
        mac)     echo "$1/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" ;;
        windows) echo "$1/Engine/Binaries/Win64/UnrealEditor.exe" ;;
        linux)   echo "$1/Engine/Binaries/Linux/UnrealEditor" ;;
    esac
}

launcher_installed_dat() {
    case "$OS" in
        mac)     echo "$HOME/Library/Application Support/Epic/UnrealEngineLauncher/LauncherInstalled.dat" ;;
        windows) echo "${PROGRAMDATA:-C:/ProgramData}/Epic/UnrealEngineLauncher/LauncherInstalled.dat" ;;
        linux)   echo "$HOME/.config/Epic/UnrealEngineLauncher/LauncherInstalled.dat" ;;
    esac
}

find_install_via_launcher() {
    # Prints the InstallLocation for AppName "UE_<version>", if found.
    [ -n "$ENGINE_VERSION" ] || return 1
    local dat app
    dat="$(launcher_installed_dat)"
    [ -f "$dat" ] || return 1
    app="UE_$ENGINE_VERSION"

    if command -v jq >/dev/null 2>&1; then
        jq -r --arg app "$app" '.InstallationList[]? | select(.AppName==$app) | .InstallLocation' "$dat" 2>/dev/null | head -n1
    elif command -v python3 >/dev/null 2>&1; then
        python3 - "$dat" "$app" <<'PY' 2>/dev/null
import json, sys
try:
    data = json.load(open(sys.argv[1]))
    for entry in data.get("InstallationList", []):
        if entry.get("AppName") == sys.argv[2]:
            print(entry.get("InstallLocation", ""))
            break
except Exception:
    pass
PY
    fi
}

default_install_guesses() {
    [ -n "$ENGINE_VERSION" ] || return 0
    case "$OS" in
        mac)     echo "/Users/Shared/Epic Games/UE_$ENGINE_VERSION" ;;
        windows) echo "C:/Program Files/Epic Games/UE_$ENGINE_VERSION" ;;
        linux)   echo "$HOME/UnrealEngine-$ENGINE_VERSION" ;;
    esac
}

if [ -z "$UE_EDITOR_PATH" ]; then
    install_dir="$(find_install_via_launcher)"
    if [ -z "$install_dir" ]; then
        while IFS= read -r guess; do
            [ -n "$guess" ] && [ -d "$guess" ] && { install_dir="$guess"; break; }
        done <<< "$(default_install_guesses)"
    fi
    [ -n "$install_dir" ] && UE_EDITOR_PATH="$(editor_binary_for_install "$install_dir")"
fi

[ -n "$UE_EDITOR_PATH" ] && [ -e "$UE_EDITOR_PATH" ] || die "couldn't find UnrealEditor for engine version '${ENGINE_VERSION:-unknown}'. Set UE_EDITOR_PATH to the editor binary and re-run."

# --- Resolve the two files to compare ---------------------------------------
if [ "$#" -eq 7 ]; then
    # git diff driver convention: path old-file old-hex old-mode new-file new-hex new-mode
    LOCAL_FILE="$2"
    REMOTE_FILE="$5"
elif [ "$#" -eq 2 ]; then
    # git difftool convention
    LOCAL_FILE="$1"
    REMOTE_FILE="$2"
else
    die "expected 2 args (difftool) or 7 args (diff driver), got $#"
fi

for f in "$LOCAL_FILE" "$REMOTE_FILE"; do
    if [ "$f" = "/dev/null" ]; then
        echo "ue5diff: file was added or deleted, skipping visual diff"
        exit 0
    fi
done

# UnrealEditor changes its own working directory on launch, so relative
# paths (e.g. the working-tree file git hands us) would stop resolving.
abspath() {
    case "$1" in
        /*) echo "$1" ;;
        *) echo "$(cd "$(dirname "$1")" && pwd)/$(basename "$1")" ;;
    esac
}
LOCAL_FILE="$(abspath "$LOCAL_FILE")"
REMOTE_FILE="$(abspath "$REMOTE_FILE")"

"$UE_EDITOR_PATH" "$UPROJECT_PATH" -diff "$LOCAL_FILE" "$REMOTE_FILE" -NoSourceControl
