/* Omarchy GTK Theming: GTK 3 stylesheet, rendered into the theme folder on
   every theme switch. ~/.config/gtk-3.0/gtk.css links to the result.
   GTK 3 loads this above whatever theme is active, so it also covers libhandy
   apps (Evince, Disks), which force GTK's built-in Adwaita, and light themes,
   whose "Adwaita" GTK 3 can't be overridden from ~/.local/share/themes.
   @DATA@ and @THEME_CSS@ are filled in by install. */
@import url("file://@DATA@/adw-gtk3-{{ mode }}.css");
@import url("file://@THEME_CSS@");
