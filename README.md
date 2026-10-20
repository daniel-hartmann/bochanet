# TESTANDO 123 BOM DIA

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
