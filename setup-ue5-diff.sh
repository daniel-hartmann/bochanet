#!/bin/bash
# One-time setup: wires ue5diff.sh into this clone's *local* git config
# (.git/config), so it works out of the box for anyone who clones the repo
# without touching their global ~/.gitconfig.
set -euo pipefail

REPO_ROOT="$(git rev-parse --show-toplevel)"
DIFF_SCRIPT="$REPO_ROOT/ue5diff.sh"

chmod +x "$DIFF_SCRIPT"

git config --local diff.unreal.command "\"$DIFF_SCRIPT\""
git config --local difftool.ue5.cmd "\"$DIFF_SCRIPT\" \"\$LOCAL\" \"\$REMOTE\""
git config --local difftool.prompt false
git config --local diff.tool ue5

echo "Configured git diff.unreal and difftool.ue5 for this repo (local config only)."
echo "Usage:"
echo "  git diff <file>.uasset          # uses UnrealEditor's asset diff automatically"
echo "  git difftool -t ue5 <file>.umap # same, via difftool"
