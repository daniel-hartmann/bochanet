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