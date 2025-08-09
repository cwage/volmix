#include <gtk/gtk.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include "pulse_client.h"

// Constants for PulseAudio operation timeouts and delays
#define MAX_REFRESH_TIMEOUT 50     // Maximum iterations to wait for refresh (0.25s)
#define DELAY_5_MS_USEC 5000       // 5ms delay in microseconds

typedef struct {
    GtkWidget *tray_icon;
    GtkWidget *popup_menu;
    GtkWidget *volmix_window;
    pulse_client_t pulse_client;
} volmix_app_t;

// Structure for asynchronous PulseAudio processing
typedef struct {
    volmix_app_t *app;
    int count;
} AsyncIterateData;

static volmix_app_t app_data;

// Forward declaration
static void update_slider_indexes(volmix_app_t *app);
static void update_sliders_recursive(GList *widgets, GList *apps);

// Asynchronous callback for PulseAudio processing
static gboolean async_iterate_callback(gpointer user_data)
{
    AsyncIterateData *data = (AsyncIterateData *)user_data;
    pulse_client_iterate(&data->app->pulse_client);
    data->count++;
    
    if (data->count < 5) {
        // Schedule next iteration after 10ms
        return G_SOURCE_CONTINUE;
    } else {
        g_free(data);
        return G_SOURCE_REMOVE;
    }
}

static void on_app_volume_changed(GtkRange *range, gpointer user_data)
{
    uint32_t *sink_input_index = (uint32_t *)user_data;
    double value = gtk_range_get_value(range);
    int volume = (int)value;
    
    printf("Setting volume for sink input %u to %d%%\n", *sink_input_index, volume);
    
    // Update the application volume
    if (pulse_client_set_app_volume(&app_data.pulse_client, *sink_input_index, volume)) {
        printf("Successfully set volume for app %u to %d%%\n", *sink_input_index, volume);
    } else {
        printf("Failed to set volume for app %u\n", *sink_input_index);
    }
}


static void build_volume_window(volmix_app_t *app)
{
    // Destroy existing window if it exists
    // TODO: Optimization opportunity - update existing window content instead of destroying/rebuilding
    if (app->volmix_window) {
        gtk_widget_destroy(app->volmix_window);
        app->volmix_window = NULL;
    }
    
    // Refresh the list of audio applications
    pulse_client_refresh_apps(&app->pulse_client);
    
    // Process pending PulseAudio operations with minimal blocking
    if (app->pulse_client.operation) {
        // Wait for the refresh operation to complete (this is necessary for UI)
        int timeout_count = 0;
        
        while (pa_operation_get_state(app->pulse_client.operation) == PA_OPERATION_RUNNING && 
               timeout_count < MAX_REFRESH_TIMEOUT) {
            pulse_client_iterate(&app->pulse_client);
            g_usleep(DELAY_5_MS_USEC); // 5ms delay
            timeout_count++;
        }
        
        pa_operation_unref(app->pulse_client.operation);
        app->pulse_client.operation = NULL;
    }
    
    // Schedule additional processing time asynchronously for ongoing events
    AsyncIterateData *iterate_data = g_new0(AsyncIterateData, 1);
    iterate_data->app = app;
    iterate_data->count = 0;
    g_timeout_add(10, async_iterate_callback, iterate_data);
    
    // Get list of audio applications
    GList *apps = pulse_client_get_apps(&app->pulse_client);
    printf("Found %d applications when building window\n", g_list_length(apps));
    
    // Create popup window
    app->volmix_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(app->volmix_window), "Volume Control");
    gtk_window_set_decorated(GTK_WINDOW(app->volmix_window), TRUE);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(app->volmix_window), FALSE);
    gtk_window_set_skip_pager_hint(GTK_WINDOW(app->volmix_window), FALSE);
    gtk_window_set_type_hint(GTK_WINDOW(app->volmix_window), GDK_WINDOW_TYPE_HINT_DIALOG);
    gtk_window_set_resizable(GTK_WINDOW(app->volmix_window), FALSE);
    
    // Create main container with minimal spacing
    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_container_set_border_width(GTK_CONTAINER(main_box), 4);
    gtk_container_add(GTK_CONTAINER(app->volmix_window), main_box);
    
    if (apps == NULL) {
        // No applications playing audio
        GtkWidget *no_apps_label = gtk_label_new("No applications playing audio");
        gtk_box_pack_start(GTK_BOX(main_box), no_apps_label, FALSE, FALSE, 0);
    } else {
        // Add master volume header
        GtkWidget *master_label = gtk_label_new("Master Volume");
        gtk_label_set_markup(GTK_LABEL(master_label), "<b>Master Volume</b>");
        gtk_box_pack_start(GTK_BOX(main_box), master_label, FALSE, FALSE, 0);
        
        // Add separator
        GtkWidget *separator1 = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
        gtk_box_pack_start(GTK_BOX(main_box), separator1, FALSE, FALSE, 1);
        
        // Add application volume controls
        GtkWidget *apps_label = gtk_label_new("Applications");
        gtk_label_set_markup(GTK_LABEL(apps_label), "<b>Applications</b>");
        gtk_box_pack_start(GTK_BOX(main_box), apps_label, FALSE, FALSE, 0);
        
        // Add each application
        GList *item = apps;
        int app_count = 0;
        while (item) {
            app_audio_t *audio_app = (app_audio_t *)item->data;
            app_count++;
            printf("Adding app %d: %s\n", app_count, audio_app->name);
            
            // Create container for this app with minimal spacing
            GtkWidget *app_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 1);
            gtk_box_pack_start(GTK_BOX(main_box), app_box, FALSE, FALSE, 1);
            
            // Application name label
            char label_text[256];
            snprintf(label_text, sizeof(label_text), "%s (%d%%)", 
                    audio_app->name, app_audio_get_volume_percent(audio_app));
            GtkWidget *label = gtk_label_new(label_text);
            gtk_widget_set_halign(label, GTK_ALIGN_START);
            gtk_box_pack_start(GTK_BOX(app_box), label, FALSE, FALSE, 0);
            
            // Volume slider
            GtkWidget *slider = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.0, 100.0, 1.0);
            gtk_range_set_value(GTK_RANGE(slider), app_audio_get_volume_percent(audio_app));
            gtk_scale_set_draw_value(GTK_SCALE(slider), TRUE);
            gtk_scale_set_value_pos(GTK_SCALE(slider), GTK_POS_RIGHT);
            gtk_widget_set_size_request(slider, 160, -1);
            
            // Store the sink input index for the callback
            uint32_t *index_ptr = g_malloc(sizeof(uint32_t));
            *index_ptr = audio_app->index;
            
            // Store application name for index updates
            char *app_name = g_strdup(audio_app->name);
            
            // Use g_object_set_data_full to ensure memory is freed when widget is destroyed
            g_object_set_data_full(G_OBJECT(slider), "sink_input_index", index_ptr, g_free);
            g_object_set_data_full(G_OBJECT(slider), "app_name", app_name, g_free);
            
            // Connect slider signal using the stored data instead of direct pointer
            g_signal_connect(slider, "value-changed", G_CALLBACK(on_app_volume_changed), 
                           g_object_get_data(G_OBJECT(slider), "sink_input_index"));
            
            gtk_box_pack_start(GTK_BOX(app_box), slider, FALSE, FALSE, 0);
            
            item = item->next;
        }
        printf("Added %d applications to window\n", app_count);
        
    }
    
    // Window can be closed by clicking tray icon again or using window controls
}

static void position_window_near_cursor(GtkWindow *window)
{
    // Position the window near the mouse cursor
    GdkDisplay *display = gdk_display_get_default();
    GdkSeat *seat = gdk_display_get_default_seat(display);
    GdkDevice *pointer = gdk_seat_get_pointer(seat);
    GdkScreen *screen = gdk_display_get_default_screen(display);
    gint x, y;
    
    gdk_device_get_position(pointer, &screen, &x, &y);
    
    // Get window size to position it properly
    gint width, height;
    gtk_window_get_size(window, &width, &height);
    
    // Position window so it appears near the cursor but doesn't go off screen
    x = x - width / 2;
    y = y - height - 20; // Above the cursor
    
    // Make sure window stays on screen
    GdkRectangle screen_geometry;
    gdk_screen_get_monitor_geometry(screen, 
        gdk_screen_get_monitor_at_point(screen, x + width/2, y + height/2), 
        &screen_geometry);
    
    if (x < screen_geometry.x) x = screen_geometry.x;
    if (y < screen_geometry.y) y = screen_geometry.y;
    if (x + width > screen_geometry.x + screen_geometry.width) 
        x = screen_geometry.x + screen_geometry.width - width;
    if (y + height > screen_geometry.y + screen_geometry.height) 
        y = screen_geometry.y + screen_geometry.height - height;
    
    gtk_window_move(window, x, y);
}

static void on_tray_icon_activate(GtkStatusIcon *status_icon, gpointer user_data)
{
    volmix_app_t *app = (volmix_app_t *)user_data;
    
    // Check if window exists
    if (app->volmix_window) {
        if (gtk_widget_get_visible(app->volmix_window)) {
            printf("Tray icon clicked! Hiding volume control window...\n");
            gtk_widget_hide(app->volmix_window);
            return;
        } else {
            printf("Tray icon clicked! Showing volume control window...\n");
            gtk_widget_show_all(app->volmix_window);
            position_window_near_cursor(GTK_WINDOW(app->volmix_window));
            gtk_window_present(GTK_WINDOW(app->volmix_window));
            return;
        }
    }
    
    printf("Tray icon clicked! Building volume control window...\n");
    
    // Build the volume control window
    build_volume_window(app);
    
    // Show the window first so GTK can calculate its size
    gtk_widget_show_all(app->volmix_window);
    
    // Position the window near the mouse cursor after showing
    position_window_near_cursor(GTK_WINDOW(app->volmix_window));
    gtk_window_present(GTK_WINDOW(app->volmix_window));
}

static void on_tray_icon_popup_menu(GtkStatusIcon *status_icon, guint button, 
                                   guint activate_time, gpointer user_data)
{
    volmix_app_t *app = (volmix_app_t *)user_data;
    
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
    volmix_app_t *app = (volmix_app_t *)user_data;
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

static void setup_tray_icon(volmix_app_t *app)
{
    app->tray_icon = gtk_status_icon_new();
    
    // Load the inverted sound icon PNG (white speaker)
    // Try development location first, then installed location
    gchar *dev_icon_path = g_build_filename("data", "icons", "sound-icon-inverted.png", NULL);
    gchar *installed_icon_path = g_build_filename(DATADIR, "volmix", "icons", "sound-icon-inverted.png", NULL);
    gchar *icon_path = NULL;
    GError *error = NULL;
    GdkPixbuf *icon_pixbuf = NULL;
    
    // Check development location first
    if (g_file_test(dev_icon_path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR)) {
        icon_pixbuf = gdk_pixbuf_new_from_file_at_size(dev_icon_path, 22, 22, &error);
        icon_path = g_strdup(dev_icon_path);  // Keep track for error reporting
    }
    // Check installed location if development location failed
    else if (g_file_test(installed_icon_path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR)) {
        icon_pixbuf = gdk_pixbuf_new_from_file_at_size(installed_icon_path, 22, 22, &error);
        icon_path = g_strdup(installed_icon_path);  // Keep track for error reporting
    }
    
    g_free(dev_icon_path);
    g_free(installed_icon_path);
    
    if (icon_pixbuf) {
        gtk_status_icon_set_from_pixbuf(GTK_STATUS_ICON(app->tray_icon), icon_pixbuf);
        g_object_unref(icon_pixbuf);
        g_print("Loaded inverted sound icon PNG\n");
    } else {
        // Fallback to system icon if file not found
        g_warning("Could not load sound icon (%s), using system icon",
                  error ? error->message : "file not found");
        if (error) g_error_free(error);
        gtk_status_icon_set_from_icon_name(GTK_STATUS_ICON(app->tray_icon),
                                          "audio-volume-high");
    }
    
    g_free(icon_path);
    
    gtk_status_icon_set_tooltip_text(GTK_STATUS_ICON(app->tray_icon), 
                                    "volmix - Per-Application Audio Control");
    
    gtk_status_icon_set_visible(GTK_STATUS_ICON(app->tray_icon), TRUE);
    
    // Connect signals
    g_signal_connect(app->tray_icon, "activate", 
                    G_CALLBACK(on_tray_icon_activate), app);
    g_signal_connect(app->tray_icon, "popup-menu", 
                    G_CALLBACK(on_tray_icon_popup_menu), app);
    g_signal_connect(app->tray_icon, "scroll-event", 
                    G_CALLBACK(on_scroll_event), app);
}

static void cleanup_app(volmix_app_t *app)
{
    if (app->tray_icon) {
        gtk_status_icon_set_visible(GTK_STATUS_ICON(app->tray_icon), FALSE);
        app->tray_icon = NULL;
    }
    
    if (app->volmix_window) {
        gtk_widget_destroy(app->volmix_window);
        app->volmix_window = NULL;
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
    volmix_app_t *app = (volmix_app_t *)user_data;
    
    // Process PulseAudio events
    pulse_client_iterate(&app->pulse_client);
    
    // Check if sink inputs have changed and window is visible
    if (pulse_client_sink_inputs_changed(&app->pulse_client) && 
        app->volmix_window && gtk_widget_get_visible(app->volmix_window)) {
        printf("Sink inputs changed, updating slider indexes...\n");
        update_slider_indexes(app);
    }
    
    return G_SOURCE_CONTINUE; // Keep the timer running
}

// Function to update slider widget data with current sink input indexes
static void update_slider_indexes(volmix_app_t *app)
{
    if (!app->volmix_window || !gtk_widget_get_visible(app->volmix_window)) {
        return;
    }
    
    // Refresh the sink input list
    pulse_client_refresh_apps(&app->pulse_client);
    
    // Process the refresh operation
    if (app->pulse_client.operation) {
        int timeout_count = 0;
        while (pa_operation_get_state(app->pulse_client.operation) == PA_OPERATION_RUNNING && 
               timeout_count < MAX_REFRESH_TIMEOUT) {
            pulse_client_iterate(&app->pulse_client);
            g_usleep(DELAY_5_MS_USEC);
            timeout_count++;
        }
        pa_operation_unref(app->pulse_client.operation);
        app->pulse_client.operation = NULL;
    }
    
    // Get current apps list
    GList *apps = pulse_client_get_apps(&app->pulse_client);
    
    // Find all slider widgets and update their stored sink input indexes
    GList *widgets = gtk_container_get_children(GTK_CONTAINER(app->volmix_window));
    update_sliders_recursive(widgets, apps);
    g_list_free(widgets);
}

// Recursive function to find and update slider widgets
static void update_sliders_recursive(GList *widgets, GList *apps)
{
    while (widgets) {
        GtkWidget *widget = GTK_WIDGET(widgets->data);
        
        if (GTK_IS_SCALE(widget)) {
            // This is a volume slider - check if it has stored app data
            uint32_t *stored_index = (uint32_t *)g_object_get_data(G_OBJECT(widget), "sink_input_index");
            char *stored_app_name = (char *)g_object_get_data(G_OBJECT(widget), "app_name");
            
            if (stored_index && stored_app_name) {
                // Find the current index for this app name
                GList *app_item = apps;
                while (app_item) {
                    app_audio_t *app = (app_audio_t *)app_item->data;
                    if (strcmp(app->name, stored_app_name) == 0) {
                        if (*stored_index != app->index) {
                            printf("Updating slider for '%s': index %u -> %u\n", 
                                   stored_app_name, *stored_index, app->index);
                            *stored_index = app->index;
                        }
                        break;
                    }
                    app_item = app_item->next;
                }
            }
        } else if (GTK_IS_CONTAINER(widget)) {
            // Recursively check container widgets
            GList *children = gtk_container_get_children(GTK_CONTAINER(widget));
            update_sliders_recursive(children, apps);
            g_list_free(children);
        }
        
        widgets = widgets->next;
    }
}

int main(int argc, char *argv[])
{
    // Initialize GTK
    gtk_init(&argc, &argv);
    
    // Set up signal handlers for clean shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize application data
    memset(&app_data, 0, sizeof(volmix_app_t));
    
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
    
    printf("volmix application started. System tray icon should be visible.\n");
    printf("Left-click: Show menu, Right-click: Context menu, Scroll: Master volume\n");
    printf("Current volume: %d%%\n", pulse_client_get_master_volume(&app_data.pulse_client));
    printf("Press Ctrl+C to quit.\n");
    
    // Run GTK main loop
    gtk_main();
    
    // Cleanup
    cleanup_app(&app_data);
    
    return 0;
}