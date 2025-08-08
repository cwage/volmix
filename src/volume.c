#include <gtk/gtk.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include "pulse_client.h"

typedef struct {
    GtkWidget *tray_icon;
    GtkWidget *popup_menu;
    pulse_client_t pulse_client;
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
    gtk_menu_popup_at_pointer(GTK_MENU(menu), NULL);
}

static gboolean on_scroll_event(GtkStatusIcon *status_icon, GdkEventScroll *event, 
                               gpointer user_data)
{
    volume_app_t *app = (volume_app_t *)user_data;
    const int volume_step = 5; // 5% volume steps
    
    if (event->direction == GDK_SCROLL_UP) {
        printf("Scroll up - increase master volume\n");
        if (pulse_client_increase_master_volume(&app->pulse_client, volume_step)) {
            int current_volume = pulse_client_get_master_volume(&app->pulse_client);
            printf("Volume increased to %d%%\n", current_volume);
        } else {
            printf("Failed to increase volume\n");
        }
    } else if (event->direction == GDK_SCROLL_DOWN) {
        printf("Scroll down - decrease master volume\n");
        if (pulse_client_decrease_master_volume(&app->pulse_client, volume_step)) {
            int current_volume = pulse_client_get_master_volume(&app->pulse_client);
            printf("Volume decreased to %d%%\n", current_volume);
        } else {
            printf("Failed to decrease volume\n");
        }
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
        gtk_status_icon_set_visible(GTK_STATUS_ICON(app->tray_icon), FALSE);
        app->tray_icon = NULL;
    }
    
    // Cleanup PulseAudio client
    pulse_client_disconnect(&app->pulse_client);
    pulse_client_cleanup(&app->pulse_client);
}

static void signal_handler(int sig)
{
    printf("Received signal %d, cleaning up...\n", sig);
    cleanup_app(&app_data);
    gtk_main_quit();
}

static gboolean pulse_client_timer_callback(gpointer user_data)
{
    volume_app_t *app = (volume_app_t *)user_data;
    
    // Process PulseAudio events
    pulse_client_iterate(&app->pulse_client);
    
    return G_SOURCE_CONTINUE; // Keep the timer running
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
    
    // Initialize and connect PulseAudio client
    if (!pulse_client_init(&app_data.pulse_client)) {
        printf("Failed to initialize PulseAudio client\n");
        return 1;
    }
    
    if (!pulse_client_connect(&app_data.pulse_client)) {
        printf("Failed to connect to PulseAudio server\n");
        cleanup_app(&app_data);
        return 1;
    }
    
    // Set up system tray icon
    setup_tray_icon(&app_data);
    
    // Set up timer to process PulseAudio events regularly
    g_timeout_add(100, pulse_client_timer_callback, &app_data);
    
    printf("Volume application started. System tray icon should be visible.\n");
    printf("Left-click: Show menu, Right-click: Context menu, Scroll: Master volume\n");
    printf("Current volume: %d%%\n", pulse_client_get_master_volume(&app_data.pulse_client));
    printf("Press Ctrl+C to quit.\n");
    
    // Run GTK main loop
    gtk_main();
    
    // Cleanup
    cleanup_app(&app_data);
    
    return 0;
}