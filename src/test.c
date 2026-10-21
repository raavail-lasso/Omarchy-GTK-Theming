/* Self-check for gtk-live-css.so. Run via ../test. */
/* Opens a GTK 4 window, switches "theme" like omarchy-theme-set does, and
 * checks the live colour. Exit 0 = pass. */
#include <gtk/gtk.h>
#include <stdlib.h>
static GtkWidget *win; static int step; static const char *base;
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
static char *color (void) { GdkRGBA c; if (!gtk_style_context_lookup_color (gtk_widget_get_style_context (win), "probe", &c)) return g_strdup ("none"); return gdk_rgba_to_string (&c); }
G_GNUC_END_IGNORE_DEPRECATIONS
static void sh (const char *cmd) { char *c = g_strdup_printf ("cd %s && %s", base, cmd); if (system (c)) exit (3); g_free (c); }
static gboolean tick (gpointer d) {
  char *c = color (); g_print ("step %d: %s\n", step, c);
  const char *want[] = { "rgb(255,0,0)", "rgb(0,0,255)", "rgb(0,255,0)" };
  if (strcmp (c, want[step])) { g_print ("FAIL want %s\n", want[step]); exit (1); }
  g_free (c);
  if (step == 0) sh ("mkdir next && echo '@define-color probe #0000ff;' > next/gtk.css && rm -rf theme && mv next theme");
  if (step == 1) sh ("echo '@define-color probe #00ff00;' > theme/gtk.css.tmp && mv theme/gtk.css.tmp theme/gtk.css");
  if (++step == 3) { g_print ("PASS\n"); exit (0); }
  return G_SOURCE_CONTINUE;
}
static void activate (GtkApplication *app) {
  if (g_getenv ("LD_PRELOAD")) { g_print ("FAIL LD_PRELOAD still set, child programs would inherit it\n"); exit (2); } win = gtk_application_window_new (app); gtk_window_present (GTK_WINDOW (win)); g_timeout_add (800, tick, NULL); }
int main (int argc, char **argv) {
  base = g_getenv ("TESTBASE");
  GtkApplication *app = gtk_application_new (NULL, G_APPLICATION_NON_UNIQUE);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  return g_application_run (G_APPLICATION (app), argc, argv);
}
