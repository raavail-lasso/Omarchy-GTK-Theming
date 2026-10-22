#!/bin/bash
# Omarchy GTK4 Theming: point GTK at the Omarchy-GTK theme (adw-gtk3 plus the
# current Omarchy colours) so GTK 3 apps and non-libadwaita GTK 4 apps match.
# omarchy-theme-set-gnome resets gtk-theme to Adwaita on every switch; Omarchy
# runs theme-set hooks after it.

[[ -n ${DBUS_SESSION_BUS_ADDRESS:-} ]] || exit 0

mode=$(omarchy-theme-color --file "$HOME/.local/state/omarchy/current/theme/colors.toml" mode)
theme=Omarchy-GTK-dark
[[ $mode == light ]] && theme=Omarchy-GTK

# GTK only re-reads a theme from disk when its name changes, which is what
# makes running apps pick up the new colours. Omarchy's reset normally does
# that; if it didn't, change the name ourselves.
if [[ $(gsettings get org.gnome.desktop.interface gtk-theme) == "'$theme'" ]]; then
  gsettings set org.gnome.desktop.interface gtk-theme Adwaita-dark
  sleep 0.3
fi
gsettings set org.gnome.desktop.interface gtk-theme "$theme"
