/* gtk-live-css: reload ~/.config/gtk-4.0/gtk.css in running GTK 4 apps.
 *
 * Loaded with LD_PRELOAD. GTK reads the user gtk.css once at startup; this adds
 * a second provider just above GTK's own (USER + 1), loads the same file, and
 * reloads it whenever the file or its symlink target changes. Same idea as the
 * gtk4-live-css patch, but using only public API from outside GTK.
 *
 * Safety: not linked against GTK or GLib. Every function is looked up with
 * dlsym in the already-loaded process; if anything is missing, or this isn't a
 * GTK 4 app, it does nothing. GTK_LIVE_CSS=0 turns it off for one run.
 *
 * Built by ../install. Design notes: ../docs/gtk-live-theming.html
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

/* __typeof__ only borrows the declaration, so nothing here links to GTK. */
#define FN(name) static __typeof__(name) *p_##name
FN(gtk_get_major_version);
FN(gdk_display_get_default);
FN(gtk_css_provider_new);
FN(gtk_css_provider_load_from_path);
FN(gtk_css_provider_load_from_string);
FN(gtk_style_context_add_provider_for_display);
FN(g_timeout_add);
FN(g_source_remove);
FN(g_file_new_for_path);
FN(g_file_monitor_file);
FN(g_signal_connect_data);
FN(g_object_unref);
FN(g_file_test);
FN(g_get_user_config_dir);
FN(g_build_filename);

static GtkCssProvider *provider;
static char *css_path;
static char *css_target;
static GFileMonitor *path_monitor, *target_monitor;
static guint reload_id;
static int display_tries, missing_tries;

static void on_changed (GFileMonitor *m, GFile *f, GFile *o, GFileMonitorEvent e, gpointer d);

static GFileMonitor *
watch (const char *path)
{
  GFile *file = p_g_file_new_for_path (path);
  GFileMonitor *monitor = p_g_file_monitor_file (file, G_FILE_MONITOR_WATCH_MOVES, NULL, NULL);

  if (monitor)
    p_g_signal_connect_data (monitor, "changed", G_CALLBACK (on_changed), NULL, NULL, 0);
  p_g_object_unref (file);
  return monitor;
}

/* Re-arm the watch on the symlink target every time. Omarchy deletes and
 * recreates the theme folder, so an old watch can point at a dead directory.
 * Start the new watch before dropping the old one (the patch found GLib loses
 * the kernel watch otherwise). */
static void
watch_target (void)
{
  char *target = realpath (css_path, NULL);
  GFileMonitor *old = target_monitor;

  if (target && strcmp (target, css_path) == 0)
    {
      free (target);
      target = NULL;
    }
  target_monitor = target ? watch (target) : NULL;
  if (old)
    p_g_object_unref (old);
  free (css_target);
  css_target = target;
}

static gboolean
reload (gpointer data)
{
  reload_id = 0;

  if (p_g_file_test (css_path, G_FILE_TEST_IS_REGULAR))
    {
      missing_tries = 0;
      p_gtk_css_provider_load_from_path (provider, css_path);
      watch_target ();
    }
  else if (++missing_tries < 20)
    /* mid theme switch: keep current colours and look again shortly */
    reload_id = p_g_timeout_add (150, reload, NULL);
  else
    {
      missing_tries = 0;
      p_gtk_css_provider_load_from_string (provider, "");
      watch_target ();
    }

  return G_SOURCE_REMOVE;
}

static void
on_changed (GFileMonitor *m, GFile *f, GFile *o, GFileMonitorEvent event, gpointer d)
{
  switch (event)
    {
    case G_FILE_MONITOR_EVENT_CHANGES_DONE_HINT:
    case G_FILE_MONITOR_EVENT_CREATED:
    case G_FILE_MONITOR_EVENT_DELETED:
    case G_FILE_MONITOR_EVENT_RENAMED:
    case G_FILE_MONITOR_EVENT_MOVED_IN:
    case G_FILE_MONITOR_EVENT_MOVED_OUT:
      break;
    default:
      return;
    }

  /* theme switchers write several files in a row; wait until they stop */
  if (reload_id)
    p_g_source_remove (reload_id);
  missing_tries = 0;
  reload_id = p_g_timeout_add (150, reload, NULL);
}

/* Runs on GTK's main loop. Polls until GTK has opened its display. */
static gboolean
setup (gpointer data)
{
  GdkDisplay *display = p_gdk_display_get_default ();

  if (!display)
    return ++display_tries < 100 ? G_SOURCE_CONTINUE : G_SOURCE_REMOVE;

  css_path = p_g_build_filename (p_g_get_user_config_dir (), "gtk-4.0", "gtk.css", NULL);
  provider = p_gtk_css_provider_new ();
  p_gtk_style_context_add_provider_for_display (display, (GtkStyleProvider *) provider,
                                                GTK_STYLE_PROVIDER_PRIORITY_USER + 1);
  path_monitor = watch (css_path);
  reload (NULL);
  return G_SOURCE_REMOVE;
}

__attribute__ ((constructor)) static void
init (void)
{
  const char *preload = getenv ("LD_PRELOAD");
  const char *off = getenv ("GTK_LIVE_CSS");

#define LOAD(name) if (!(p_##name = dlsym (RTLD_DEFAULT, #name))) return
  /* Not a GTK 4 app (env, timeout, a shell wrapper...): leave everything,
   * including LD_PRELOAD, untouched so it reaches the real app. */
  LOAD (gtk_get_major_version);
  if (p_gtk_get_major_version () != 4)
    return;

  /* Don't hand ourselves to programs this app launches (files opened from
   * Nautilus, terminals, ...). Only when we are the sole preload entry. */
  if (preload && strstr (preload, "gtk-live-css") && !strpbrk (preload, ": "))
    unsetenv ("LD_PRELOAD");

  if (off && strcmp (off, "0") == 0)
    return;

  LOAD (gdk_display_get_default);
  LOAD (gtk_css_provider_new);
  LOAD (gtk_css_provider_load_from_path);
  LOAD (gtk_css_provider_load_from_string);
  LOAD (gtk_style_context_add_provider_for_display);
  LOAD (g_timeout_add);
  LOAD (g_source_remove);
  LOAD (g_file_new_for_path);
  LOAD (g_file_monitor_file);
  LOAD (g_signal_connect_data);
  LOAD (g_object_unref);
  LOAD (g_file_test);
  LOAD (g_get_user_config_dir);
  LOAD (g_build_filename);

  p_g_timeout_add (100, setup, NULL);
}
