# Installation

Everything users need is in [`download/bloodmoon-1.5.2.zip`](../download/bloodmoon-1.5.2.zip). Its
[`INSTALL.md`](../download/INSTALL.md) gives the steps for each firmware mod. Back up the OP-1 (tapes and the `synth`
folder) in disk mode first, start with the volume low, and install only one plugin file.

| Your OP-1 runs | File | Where it goes |
|---|---|---|
| Opus 2, core 2.6+ | `opus/bloodmoon.opmod` | the disk's **mods** folder, then restart |
| Hundekraft 3.0+ | `hundekraft/bloodmoon.knl` | the Hundekraft Workshop's **Your plugins**, then **Update OP-1** |
| op1r with the plugin loader | `releases/v1.3/bloodmoon.op1p` (not in the zip) | the Workshop, or `/yaffs2/plugins/` |

**Presets:** in disk mode, copy the folder `presets/bloodmoon/` into the OP-1's `synth/` folder. The same eight files
work with every plugin file above. On an OP-1 without the plugin, they load as the stock iter engine.

**Recovery:** on Opus, hold **Stop** while switching on to start with no plugins. On Hundekraft, remove the plugin
in the Workshop and update again.

Per-platform details: [OP-1](../platforms/op1/README.md), [Opus](../platforms/opus/README.md),
[Hundekraft](../platforms/hundekraft/README.md).
