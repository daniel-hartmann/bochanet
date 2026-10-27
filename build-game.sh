#!/bin/bash
# Packages BochaNet for distribution on macOS (build + cook + pak + archive).
#
#   ./build-game.sh [Shipping|Development]
#
# Close the Unreal Editor first. The engine path can be overridden with UE_ROOT.
# Output: Packaged/Mac/ (the .app ready to zip and share).

set -euo pipefail

CONFIG="${1:-Shipping}"
UE_ROOT="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}"
REPO="$(cd "$(dirname "$0")" && pwd)"
PROJECT="$REPO/BochaNet/BochaNet.uproject"
OUT="$REPO/Packaged"
MAPS="/Game/ThirdPerson/Lvl_ThirdPerson+/Game/Bocha/Maps/Lvl_Cancha"

if [[ "$CONFIG" != "Shipping" && "$CONFIG" != "Development" ]]; then
	echo "Usage: $0 [Shipping|Development]" >&2
	exit 1
fi
if [[ ! -x "$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh" ]]; then
	echo "Unreal Engine not found at $UE_ROOT (set UE_ROOT)." >&2
	exit 1
fi
if pgrep -q UnrealEditor; then
	echo "Close the Unreal Editor before packaging." >&2
	exit 1
fi

# The cooker loads the project's editor modules, so they must be up to date.
"$UE_ROOT/Engine/Build/BatchFiles/Mac/Build.sh" BochaNetEditor Mac Development \
	-project="$PROJECT" -waitmutex

# Old .app bundles can carry stale cooked content; start clean.
APP="BochaNet.app"
[[ "$CONFIG" == "Shipping" ]] && APP="BochaNet-Mac-Shipping.app"
rm -rf "$REPO/BochaNet/Binaries/Mac/$APP" "$OUT/Mac/$APP"

"$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun \
	-project="$PROJECT" -platform=Mac -clientconfig="$CONFIG" \
	-map="$MAPS" -build -cook -stage -pak -package -archive -archivedirectory="$OUT" \
	-nocompileeditor -unattended -utf8output -nop4

echo
echo "Done: $OUT/Mac"
