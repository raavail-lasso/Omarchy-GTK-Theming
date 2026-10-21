# Omarchy GTK4 Theming

GTK 4 apps (Nautilus and friends) that use your Omarchy theme's colours, and
**repaint live when you switch themes**, without replacing Arch's `gtk4`.

## How it works

Two parts.

**Colours.** libadwaita ignores GTK themes (it pins the theme name to
`Adwaita-empty`). The one thing it does honour is `~/.config/gtk-4.0/gtk.css`.
A template in `themed/` maps Omarchy's `colors.toml` onto libadwaita's named
colours. Omarchy renders it into the theme folder on every switch, and
`~/.config/gtk-4.0/gtk.css` links there.

**Live reload.** GTK reads that file once, at startup. A tiny library
(`src/gtk-live-css.c`), loaded into Nautilus with `LD_PRELOAD`, watches the
file and reloads it when it changes, so open windows repaint about 150 ms
after a switch. It's the same idea as the
[gtk4-live-css](https://github.com/saneaspect/gtk4-live-css) patch, but done
from outside GTK using only public API, so GTK updates are very unlikely to
break it.

The library is defensive: it isn't linked against GTK or GLib, does nothing
outside GTK 4 apps, and doesn't pass `LD_PRELOAD` on to programs Nautilus
opens. `GTK_LIVE_CSS=0` turns it off for one run.

Full write-up, including the options considered and why:
[docs/gtk-live-theming.html](docs/gtk-live-theming.html)

## Install

```bash
./install
nautilus -q          # then reopen with Super+Shift+F
```

Requires `gcc`, `pkgconf`, `gtk4` (headers ship with it) and Omarchy.

Check it loaded (should print a number above 0):

```bash
grep -c gtk-live-css /proc/$(pgrep -x nautilus)/maps
```

Re-run `./install` after a Nautilus update to refresh its override files, and
after editing `themed/gtk.css.tpl`.

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

- **Live reload is Nautilus-only for now.** Other GTK 4 apps get the theme
  colours when they start, but need their own override to repaint live.
- **Flatpak apps** don't see the library (sandbox), so they won't repaint live.
- **Ways of starting Nautilus:** Super+Shift+F is confirmed. The app launcher
  and "Show in folder" go through the `.desktop` and D-Bus overrides, which
  aren't confirmed yet.
