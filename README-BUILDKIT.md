# Getting your BLOODMOON plugin file (no coding)

I could not build the final `.op1p` inside my sandbox (it can't download the Blackfin compiler the OP-1 needs). This kit builds it for you automatically on GitHub's free servers. You only click buttons. It takes about an hour, mostly waiting.

The result is a **`.op1p`** file. Per the Hundekraft Workshop page, "Your plugins" accepts `.knl` or `.op1p`, and the op1r page only mentions `.op1p`, so `.op1p` is the right format.

## A. Build it (about 10 minutes of clicking, then ~1 hour of waiting)
1. Make a free account at github.com (or sign in).
2. Click **+** (top right) → **New repository**. Name it `bloodmoon-build`. **Private is fine.** Click **Create repository**.
3. On the new page click the link **uploading an existing file**.
4. Unzip `bloodmoon-build-kit.zip`. Open the unzipped folder and drag **everything inside it** (the `synth_sdk` folder, the `toolchain` folder, `LICENSE`, `NOTICE`) into the browser window. Click **Commit changes**.
   (Your computer may hide a folder called `.github`; that's fine, you add that part in the next step.)
5. Click **Add file → Create new file**. In the name box type exactly `.github/workflows/build.yml` (typing the slashes makes the folders). Open `build.yml` from the kit's `.github/workflows` folder if you can see it, otherwise copy the text from the end of this page, and paste it into the big box. Click **Commit changes**.
6. Click the **Actions** tab (if it asks, click the green button to enable workflows). Click **Build BLOODMOON plugin** on the left, then **Run workflow** → **Run workflow**.
7. Wait. A yellow dot means working; green tick = done. It starts with a quick PC sound test, then builds the compiler (the slow part), then your plugin.
8. When it's green, click the finished run, scroll to **Artifacts**, and download **bloodmoon-op1p**. Unzip it: inside is **`bloodmoon.op1p`**. That's your plugin.

**If it turns red:** click the red step, copy the last ~30 lines of the log and send them to me. I'll fix it.

## B. Install it on the OP-1 (from the Workshop and op1r pages; I haven't tested this on hardware)
1. **Back up your OP-1 first** (copy tapes/synth/drum folders to your computer through disk mode, Shift + COM, then T3).
2. Go to the Hundekraft Workshop (hundekraft.github.io/op1-workshop). Step 1 there needs your original, unmodified `op1_246.op1`.
3. In the plugin list, add `bloodmoon.op1p` under **"Your plugins"**, then **Build my firmware** and install it as the page says (hold COM while switching on, press T1, copy the `.op1` file to the OP-1, eject).
4. On the OP-1 choose the **bloodmoon** synth engine.

**Recovery if something goes wrong:** switch the OP-1 off, hold **COM** while switching on, choose firmware update and install a known-good build (the Workshop page says tapes, album and presets stay). The op1r page says a plugin that locks the OP-1 at boot is skipped on the next boot.

## C. Be aware
- Everything I tested was on a PC (sound, pitch, the four knobs, the screen drawing). Nothing has run on a real OP-1 yet, so how it sounds and looks there is unknown.
- A plugin runs at the lowest level of the OP-1. Only install files you trust; this one is built from the source I wrote and tested here.

## build.yml (only needed if step 5 can't find the file)
```yaml
name: Build BLOODMOON plugin
on: workflow_dispatch
jobs:
  build:
    runs-on: ubuntu-latest
    timeout-minutes: 240
    steps:
      - uses: actions/checkout@v4
      - uses: actions/setup-python@v5
        with:
          python-version: '3.12'
      - name: Install the packaging helper
        run: pip install pyelftools
      - name: PC sound test (checks the engine runs and makes sound)
        run: cd synth_sdk && python3 harness/audio.py bloodmoon
      - name: Build the Blackfin compiler (slow, about 40 to 60 minutes)
        run: docker build -t bfin-elf-toolchain toolchain
      - name: Build the plugin file
        run: cd synth_sdk && python3 make_plugin.py plugins/bloodmoon && ls -l build/plugins
      - name: Save the plugin file
        uses: actions/upload-artifact@v4
        with:
          name: bloodmoon-op1p
          path: synth_sdk/build/plugins/bloodmoon.op1p
          if-no-files-found: error
```
