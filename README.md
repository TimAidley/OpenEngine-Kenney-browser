# Kenney Assets for OpenEngine

An OpenEngine editor package for finding assets on [kenney.nl](https://kenney.nl) and importing them into a project. Linux only for now.

You browse the real kenney.nl site, with Kenney's own pages, donation prompt and [All-in-1 bundle](https://kenney.itch.io/kenney-game-assets). The package doesn't scrape or mirror the site.

## Use it in a project

Add the package to the project's `Packages/manifest.json`:

```json
{
  "dependencies": {
    "kenney-assets": "git+https://github.com/TimAidley/OpenEngine-Kenney-browser.git#0.1.1&path=kenney-assets"
  }
}
```

Open the project, then open **Kenney Assets** from the panel menu at the right end of the top toolbar. The panel works like this:

- **Open kenney.nl** opens the site in your system browser.
- While the panel's tab is open, a finished `kenney_<pack>.zip` download in your download folder is imported into `Assets/Kenney/<pack>/`. The panel finds the download folder through `XDG_DOWNLOAD_DIR` or `xdg-user-dir`.
- Every downloaded zip is kept in `~/.cache/GameEngine/Kenney/`. The panel lists those packs with an **Import** button each, so another project can import one without downloading it again.

The import keeps one useful format per asset:

- 3D packs: glTF/GLB models and their textures, or FBX when a pack has no glTF.
- 2D, UI and icon packs: PNGs and atlases at the base size, and TTF fonts.
- Audio packs: OGG files.
- Every pack: its License and preview files.

It then collapses folders that hold only a single subfolder. Each imported file records its pack's kenney.nl page in the asset metadata (`kenney.sourcePage`).

Requires the `unzip` tool (`sudo dnf install unzip` on Fedora).

## Develop the package

Point the manifest at your checkout instead of the git URL. A git dependency is a pinned snapshot, so local edits don't show up through it:

```json
"kenney-assets": "file:/path/to/OpenEngine-Kenney-browser/kenney-assets"
```

The editor builds the package when the project opens. The pack-name parsing and the extraction rules have standalone tests:

```bash
cmake -S Tests -B Tests/build -DCMAKE_PREFIX_PATH=<OpenEngine>/build/vcpkg_installed/x64-linux
cmake --build Tests/build && Tests/build/KenneyAssetsTests
```

To also run the rules against every pack of an unzipped All-in-1 bundle, set `KENNEY_BUNDLE_DIR` to the bundle folder.

## Licence of the assets

Kenney's assets are CC0. See the `License.txt` in each imported pack.
