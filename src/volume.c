#include <gtk/gtk.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

typedef struct {
    GtkWidget *tray_icon;
    GtkWidget *popup_menu;
} volume_app_t;

static volume_app_t app_data;

static void on_tray_icon_activate(GtkStatusIcon *status_icon, gpointer user_data)
{
    volume_app_t *app = (volume_app_t *)user_data;
    
    // For now, just show a simple message
    printf("Tray icon clicked!\n");
    
    // TODO: Show popup menu with application list
}

static void on_tray_icon_popup_menu(GtkStatusIcon *status_icon, guint button, 
                                   guint activate_time, gpointer user_data)
{
    volume_app_t *app = (volume_app_t *)user_data;
    
    // Create a simple context menu for now
    GtkWidget *menu = gtk_menu_new();
    GtkWidget *quit_item = gtk_menu_item_new_with_label("Quit");
    
    g_signal_connect(quit_item, "activate", G_CALLBACK(gtk_main_quit), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), quit_item);
    
    gtk_widget_show_all(menu);
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, 
                   gtk_status_icon_position_menu, status_icon,
                   button, activate_time);
}

static gboolean on_scroll_event(GtkStatusIcon *status_icon, GdkEventScroll *event, 
                               gpointer user_data)
{
    // TODO: Implement master volume control with scroll wheel
    if (event->direction == GDK_SCROLL_UP) {
        printf("Scroll up - increase master volume\n");
    } else if (event->direction == GDK_SCROLL_DOWN) {
        printf("Scroll down - decrease master volume\n");
    }
    
    return TRUE;
}

static void setup_tray_icon(volume_app_t *app)
{
    app->tray_icon = gtk_status_icon_new();
    
    // Use a standard icon name for volume
    gtk_status_icon_set_from_icon_name(GTK_STATUS_ICON(app->tray_icon),
                                      "audio-volume-high");
    
    gtk_status_icon_set_tooltip_text(GTK_STATUS_ICON(app->tray_icon), 
                                    "Volume - Per-Application Audio Control");
    
    gtk_status_icon_set_visible(GTK_STATUS_ICON(app->tray_icon), TRUE);
    
    // Connect signals
    g_signal_connect(app->tray_icon, "activate", 
                    G_CALLBACK(on_tray_icon_activate), app);
    g_signal_connect(app->tray_icon, "popup-menu", 
                    G_CALLBACK(on_tray_icon_popup_menu), app);
    g_signal_connect(app->tray_icon, "scroll-event", 
                    G_CALLBACK(on_scroll_event), app);
}

static void cleanup_app(volume_app_t *app)
{
    if (app->tray_icon) {
        g_object_unref(app->tray_icon);
        app->tray_icon = NULL;
    }
}

static void signal_handler(int sig)
{
    printf("Received signal %d, cleaning up...\n", sig);
    cleanup_app(&app_data);
    gtk_main_quit();
}

int main(int argc, char *argv[])
{
    // Initialize GTK
    gtk_init(&argc, &argv);
    
    // Set up signal handlers for clean shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize application data
    memset(&app_data, 0, sizeof(volume_app_t));
    
    // Set up system tray icon
    setup_tray_icon(&app_data);
    
    printf("Volume application started. System tray icon should be visible.\n");
    printf("Left-click: Show menu, Right-click: Context menu, Scroll: Master volume\n");
    printf("Press Ctrl+C to quit.\n");
    
    // Run GTK main loop
    gtk_main();
    
    // Cleanup
    cleanup_app(&app_data);
    
    return 0;
}