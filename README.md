# BochaNet

## Diffing Unreal assets (`.uasset` / `.umap`)

After cloning, run once:

```bash
./setup-ue5-diff.sh
```

This wires `git diff`/`git difftool` on `.uasset`/`.umap` files to open Unreal Editor's
own asset diff (via `ue5diff.sh`), using **local** repo config only — it doesn't touch
your global `~/.gitconfig`. Blueprints and similar assets get Unreal's native graph
diff window; Levels and most other asset types fall back to a text dump of the
asset's properties opened in FileMerge (Mac) — that's an Unreal limitation, not
something this script can improve on.
## C++ (VS Code)

The gameplay code lives in `BochaNet/Source`. Binaries and IDE project files are not
in git; each machine builds and generates its own.

**Requirements**
- Mac: Xcode (provides the compiler).
- Windows: Visual Studio 2022 or the *Build Tools for Visual Studio 2022*, with the
  "Game development with C++" workload (provides MSVC, even if you edit in VS Code).
- VS Code with the Microsoft **C/C++** extension.

**First time after cloning**: open `BochaNet/BochaNet.uproject` and answer *Yes* when
the editor asks to rebuild the missing modules.

**Generate the VS Code workspace** (again whenever C++ files are added or removed):
in the editor, *Tools → Refresh Visual Studio Code Project*, or from the repo root:

- Mac:
  ```bash
  "/Users/Shared/Epic Games/UE_5.8/Engine/Build/BatchFiles/Mac/GenerateProjectFiles.sh" -project="$PWD/BochaNet/BochaNet.uproject" -game -vscode
  ```
- Windows:
  ```bat
  "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\GenerateProjectFiles.bat" -project="%CD%\BochaNet\BochaNet.uproject" -game -vscode
  ```

Then open `BochaNet/BochaNet.code-workspace` in VS Code (*File → Open Workspace from File…*).
In VS Code, build with the `BochaNetEditor … Development Build` task. On Mac, building the
game target (`BochaNet …`) regenerates an Xcode project and makes the editor open a missing
`BochaNet_Mac.code-workspace`; if that happens, generate the VS Code workspace again.
The editor is set to open C++ classes in VS Code (`Config/DefaultEditorPerProjectUserSettings.ini`).

**Build without the editor** (close the editor first; with it open, the build produces
hot-reload binaries the editor may not pick up):

- Mac:
  ```bash
  "/Users/Shared/Epic Games/UE_5.8/Engine/Build/BatchFiles/Mac/Build.sh" BochaNetEditor Mac Development -project="$PWD/BochaNet/BochaNet.uproject" -waitmutex
  ```
- Windows:
  ```bat
  "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" BochaNetEditor Win64 Development -project="%CD%\BochaNet\BochaNet.uproject" -waitmutex
  ```

## Packaging the game

Close the editor, then from the repo root:

- Mac:
  ```bash
  ./build-game.sh
  ```
- Windows:
  ```bat
  build-game.bat
  ```

Pass `Development` as the first argument to keep the in-game console (`~`); the default is
`Shipping`. The result goes to `Packaged/Mac` or `Packaged/Windows` (zip that folder to share).
Each platform has to be packaged on its own OS. Set `UE_ROOT` if the engine is not in the
default location.

**Playing over the network** (Listen Server, UDP port 7777 must be reachable): one player hosts,
the others join by IP. Shipping builds ignore a map or address on the command line, so the game
reads its own `-host` and `-join=<IP>` (`UBochaGameInstance`).

- Windows: `BochaNet.exe -host` (host) and `BochaNet.exe -join=<host IP>` (join).
- Mac: `open BochaNet-Mac-Shipping.app --args -host` (host) and
  `open -n BochaNet-Mac-Shipping.app --args -join=<host IP>` (join; `-n` opens a second copy).

Add `-windowed -ResX=1280 -ResY=720` to play in a window.
