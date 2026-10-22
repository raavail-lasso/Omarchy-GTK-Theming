# Omarchy GTK4 Theming

GTK 4 apps (Nautilus and friends) that use your Omarchy theme's colours, and
**repaint live when you switch themes**, without replacing Arch's `gtk4`.

## How it works

Three parts.

**Colours.** libadwaita ignores GTK themes (it pins the theme name to
`Adwaita-empty`). The one thing it does honour is `~/.config/gtk-4.0/gtk.css`.
A template in `themed/` maps Omarchy's `colors.toml` onto libadwaita's named
colours. Omarchy renders it into the theme folder on every switch, and
`~/.config/gtk-4.0/gtk.css` links there.

**GTK 3 and non-libadwaita apps.** Apps like Transmission (GTK 4 without
libadwaita) and Evince (GTK 3) use GTK's built-in theme, which hard-codes its
colours, so colour names do nothing there. `install` creates two small GTK
themes in `~/.local/share/themes` that import
[adw-gtk3](https://github.com/lassekongo83/adw-gtk3) (which looks colours up by
name like libadwaita) and then the same `gtk.css`. They're named `Adwaita` and
`Adwaita-dark`. GTK finds those before its built-in copies, so Omarchy's own
reset to Adwaita on every theme switch picks ours, with no hook to wait for.

**Live reload.** GTK reads that file once, at startup. A tiny library
(`src/gtk-live-css.c`), loaded into the apps listed in `install` with
`LD_PRELOAD`, watches the
file and reloads it when it changes, so open windows repaint about 150 ms
after a switch. It's the same idea as the
[gtk4-live-css](https://github.com/saneaspect/gtk4-live-css) patch, but done
from outside GTK using only public API, so GTK updates are very unlikely to
break it.

The library is defensive: it isn't linked against GTK or GLib, does nothing
outside GTK 4 apps, and doesn't pass `LD_PRELOAD` on to programs those apps
open. `GTK_LIVE_CSS=0` turns it off for one run.

Full write-up, including the options considered and why:
[docs/gtk-live-theming.html](docs/gtk-live-theming.html)

## Install

```bash
git clone git@github.com:raavail-lasso/Omarchy-GTK4-Theming.git
cd Omarchy-GTK4-Theming && ./install
nautilus -q          # then reopen with Super+Shift+F
```

Requires `gcc`, `pkgconf`, `gtk4` (headers ship with it), `adw-gtk-theme`
and Omarchy:

```bash
sudo pacman -S adw-gtk-theme
```

Check it loaded (should print a number above 0):

```bash
grep -c gtk-live-css /proc/$(pgrep -x nautilus)/maps
```

Re-run `./install` after an app update to refresh its override files, and
after editing `themed/gtk.css.tpl`.

## Adding apps

Live reload is enabled per app. The list is at the top of `install`:

```bash
APPS=(
  org.gnome.Nautilus
  org.gnome.Papers
  transmission-gtk
  dev.tensaku.Tensaku
)
```

Each entry is a `.desktop` file name from `/usr/share/applications`, without
`.desktop`. Add one, re-run `./install`, and restart the app. For each app,
`install` writes a copy of its `.desktop` file (and its D-Bus service file, if
it has one) with `LD_PRELOAD` in front of every `Exec=` line. Taking an app off
the list and re-running removes its override. If you already have your own
override of the same name, `install` skips that app rather than overwrite it.

To find GTK 4 apps on your system:

```bash
for f in /usr/share/applications/*.desktop; do
  grep -q '^NoDisplay=true' "$f" && continue
  exe=$(sed -n 's/^Exec=//p' "$f" | head -1 | awk '{print $1}')
  p=$(command -v "$exe") && ldd "$p" 2>/dev/null | grep -q libgtk-4 && basename "$f" .desktop
done
```

Only apps compiled against GTK 4 work. Python and JavaScript apps load GTK
after they start, too late for the library; they still get the theme colours,
just not live.

## Test

```bash
./test
```

Opens a test GTK 4 window, switches "themes" the way `omarchy-theme-set` does,
and checks the open window follows. Run it after GTK updates.

## Uninstall

```bash
./uninstall
nautilus -q
```

## Layout

```
install / uninstall / test
src/gtk-live-css.c      the live reload library
src/test.c              test GTK 4 program used by ./test
themed/gtk.css.tpl      Omarchy colours -> libadwaita named colours
docs/                   design notes and decisions
```

## Limits

- **Live reload is per app** (see Adding apps). GTK 4 apps not on the list get
  the theme colours when they start, but don't repaint live.
- **GTK 3 apps don't repaint live.** They get the theme colours when they
  start, but open GTK 3 windows don't react to theme changes at all on this
  setup (tested with Evince). Restart them after a switch.
- **Electron and Flutter apps** (1Password, LocalSend) only use GTK for
  things like file dialogs, so only those change.
- **"Adwaita" now means adw-gtk3 with your colours** for GTK 3 and
  non-libadwaita GTK 4 apps, including anything that asks for Adwaita by name.
  `uninstall` removes the two themes and the stock Adwaita comes back.
- **Flatpak apps** don't see the library (sandbox), so they won't repaint live.
- **An app started some other way won't have it**, e.g. a custom keybinding
  or script that runs the binary directly. Nautilus's Omarchy keybindings are
  handled by `install`.

## License

MIT
