#include "pulse_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void context_state_callback(pa_context *c, void *userdata);
static void sink_info_callback(pa_context *c, const pa_sink_info *info, int eol, void *userdata);
static void server_info_callback(pa_context *c, const pa_server_info *info, void *userdata);
static void sink_input_info_callback(pa_context *c, const pa_sink_input_info *info, int eol, void *userdata);

gboolean pulse_client_init(pulse_client_t *client)
{
    if (!client) {
        return FALSE;
    }
    
    memset(client, 0, sizeof(pulse_client_t));
    client->audio_apps = NULL;
    
    // Create mainloop
    client->mainloop = pa_mainloop_new();
    if (!client->mainloop) {
        printf("Failed to create PulseAudio mainloop\n");
        return FALSE;
    }
    
    client->mainloop_api = pa_mainloop_get_api(client->mainloop);
    if (!client->mainloop_api) {
        printf("Failed to get PulseAudio mainloop API\n");
        pulse_client_cleanup(client);
        return FALSE;
    }
    
    // Create context
    client->context = pa_context_new(client->mainloop_api, "Volume Control");
    if (!client->context) {
        printf("Failed to create PulseAudio context\n");
        pulse_client_cleanup(client);
        return FALSE;
    }
    
    // Set state callback
    pa_context_set_state_callback(client->context, context_state_callback, client);
    
    return TRUE;
}

void pulse_client_cleanup(pulse_client_t *client)
{
    if (!client) {
        return;
    }
    
    // Cleanup audio apps list
    if (client->audio_apps) {
        g_list_free_full(client->audio_apps, (GDestroyNotify)app_audio_free);
        client->audio_apps = NULL;
    }
    
    if (client->operation) {
        pa_operation_unref(client->operation);
        client->operation = NULL;
    }
    
    if (client->context) {
        pa_context_unref(client->context);
        client->context = NULL;
    }
    
    if (client->mainloop) {
        pa_mainloop_free(client->mainloop);
        client->mainloop = NULL;
    }
    
    client->mainloop_api = NULL;
    client->connected = FALSE;
}

gboolean pulse_client_connect(pulse_client_t *client)
{
    if (!client || !client->context) {
        return FALSE;
    }
    
    // Connect to PulseAudio server
    if (pa_context_connect(client->context, NULL, PA_CONTEXT_NOFLAGS, NULL) < 0) {
        printf("Failed to connect to PulseAudio server: %s\n", 
               pa_strerror(pa_context_errno(client->context)));
        return FALSE;
    }
    
    // Wait for connection with timeout
    int retval;
    const int TIMEOUT_SECONDS = 5;
    time_t start_time = time(NULL);
    
    while (TRUE) {
        // Check for timeout
        if (time(NULL) - start_time > TIMEOUT_SECONDS) {
            printf("PulseAudio connection timeout after %d seconds\n", TIMEOUT_SECONDS);
            return FALSE;
        }
        
        if (pa_mainloop_iterate(client->mainloop, 1, &retval) < 0) {
            printf("PulseAudio mainloop iteration failed\n");
            return FALSE;
        }
        
        pa_context_state_t state = pa_context_get_state(client->context);
        if (state == PA_CONTEXT_READY) {
            client->connected = TRUE;
            printf("Connected to PulseAudio server\n");
            
            // Get server info to find default sink
            client->operation = pa_context_get_server_info(client->context, 
                                                         server_info_callback, client);
            if (!client->operation) {
                printf("Failed to get server info from PulseAudio\n");
                return FALSE;
            }
            
            while (pa_operation_get_state(client->operation) == PA_OPERATION_RUNNING) {
                pa_mainloop_iterate(client->mainloop, 1, NULL);
            }
            pa_operation_unref(client->operation);
            client->operation = NULL;
            
            return TRUE;
        } else if (state == PA_CONTEXT_FAILED || state == PA_CONTEXT_TERMINATED) {
            printf("PulseAudio connection failed\n");
            return FALSE;
        }
    }
}

void pulse_client_disconnect(pulse_client_t *client)
{
    if (!client || !client->context) {
        return;
    }
    
    if (client->operation) {
        pa_operation_unref(client->operation);
        client->operation = NULL;
    }
    
    pa_context_disconnect(client->context);
    client->connected = FALSE;
}

int pulse_client_get_master_volume(pulse_client_t *client)
{
    if (!client || !client->connected) {
        return -1;
    }
    
    // Return average volume as percentage
    return (int)((pa_cvolume_avg(&client->default_sink_volume) * 100) / PA_VOLUME_NORM);
}

gboolean pulse_client_set_master_volume(pulse_client_t *client, int volume)
{
    if (!client || !client->connected || volume < 0 || volume > 100) {
        return FALSE;
    }
    
    // Convert percentage to PulseAudio volume
    pa_volume_t pa_volume = (pa_volume_t)((volume * PA_VOLUME_NORM) / 100);
    
    // Set all channels to same volume
    pa_cvolume new_volume = client->default_sink_volume;
    pa_cvolume_set(&new_volume, new_volume.channels, pa_volume);
    
    // Send volume change to PulseAudio
    if (client->operation) {
        pa_operation_unref(client->operation);
    }
    client->operation = pa_context_set_sink_volume_by_index(client->context,
                                                           client->default_sink_index,
                                                           &new_volume,
                                                           NULL, NULL);
    
    if (client->operation) {
        client->default_sink_volume = new_volume;
        return TRUE;
    }
    
    return FALSE;
}

gboolean pulse_client_increase_master_volume(pulse_client_t *client, int delta)
{
    int current_volume = pulse_client_get_master_volume(client);
    if (current_volume < 0) {
        return FALSE;
    }
    
    int new_volume = current_volume + delta;
    if (new_volume > 100) {
        new_volume = 100;
    }
    
    return pulse_client_set_master_volume(client, new_volume);
}

gboolean pulse_client_decrease_master_volume(pulse_client_t *client, int delta)
{
    int current_volume = pulse_client_get_master_volume(client);
    if (current_volume < 0) {
        return FALSE;
    }
    
    int new_volume = current_volume - delta;
    if (new_volume < 0) {
        new_volume = 0;
    }
    
    return pulse_client_set_master_volume(client, new_volume);
}

gboolean pulse_client_toggle_master_mute(pulse_client_t *client)
{
    if (!client || !client->connected) {
        return FALSE;
    }
    
    // Toggle mute state
    gboolean new_mute_state = !client->default_sink_muted;
    
    // Clean up previous operation to prevent memory leak
    if (client->operation) {
        pa_operation_unref(client->operation);
        client->operation = NULL;
    }
    client->operation = pa_context_set_sink_mute_by_index(client->context,
                                                         client->default_sink_index,
                                                         new_mute_state ? 1 : 0,
                                                         NULL, NULL);
    
    if (client->operation) {
        client->default_sink_muted = new_mute_state;
        return TRUE;
    }
    
    return FALSE;
}

void pulse_client_iterate(pulse_client_t *client)
{
    if (!client || !client->mainloop) {
        return;
    }
    
    // Process pending PulseAudio events (non-blocking)
    int retval;
    pa_mainloop_iterate(client->mainloop, 0, &retval);
}

// Callback functions
static void context_state_callback(pa_context *c, void *userdata)
{
    pulse_client_t *client = (pulse_client_t *)userdata;
    
    switch (pa_context_get_state(c)) {
        case PA_CONTEXT_CONNECTING:
            printf("PulseAudio: Connecting...\n");
            break;
        case PA_CONTEXT_AUTHORIZING:
            printf("PulseAudio: Authorizing...\n");
            break;
        case PA_CONTEXT_SETTING_NAME:
            printf("PulseAudio: Setting name...\n");
            break;
        case PA_CONTEXT_READY:
            printf("PulseAudio: Ready\n");
            break;
        case PA_CONTEXT_FAILED:
            printf("PulseAudio: Connection failed\n");
            break;
        case PA_CONTEXT_TERMINATED:
            printf("PulseAudio: Connection terminated\n");
            break;
        default:
            break;
    }
}

static void sink_info_callback(pa_context *c, const pa_sink_info *info, int eol, void *userdata)
{
    pulse_client_t *client = (pulse_client_t *)userdata;
    
    if (eol > 0) {
        return;
    }
    
    if (!info) {
        return;
    }
    
    // Store default sink information
    client->default_sink_index = info->index;
    client->default_sink_volume = info->volume;
    client->default_sink_muted = info->mute ? TRUE : FALSE;
    
    printf("Default sink: %s (index=%u, volume=%d%%, muted=%s)\n",
           info->name, info->index, 
           (int)((pa_cvolume_avg(&info->volume) * 100) / PA_VOLUME_NORM),
           info->mute ? "yes" : "no");
}

static void server_info_callback(pa_context *c, const pa_server_info *info, void *userdata)
{
    pulse_client_t *client = (pulse_client_t *)userdata;
    
    if (!info || !info->default_sink_name) {
        return;
    }
    
    printf("Default sink name: %s\n", info->default_sink_name);
    
    // Get information about the default sink
    client->operation = pa_context_get_sink_info_by_name(c, info->default_sink_name,
                                                        sink_info_callback, client);
}

// Application management functions
void pulse_client_refresh_apps(pulse_client_t *client)
{
    if (!client || !client->connected) {
        return;
    }
    
    // Clear existing apps list
    if (client->audio_apps) {
        g_list_free_full(client->audio_apps, (GDestroyNotify)app_audio_free);
        client->audio_apps = NULL;
    }
    
    // Get all sink inputs (applications with audio streams)
    if (client->operation) {
        pa_operation_unref(client->operation);
    }
    client->operation = pa_context_get_sink_input_info_list(client->context,
                                                           sink_input_info_callback,
                                                           client);
}

GList* pulse_client_get_apps(pulse_client_t *client)
{
    if (!client) {
        return NULL;
    }
    return client->audio_apps;
}

gboolean pulse_client_set_app_volume(pulse_client_t *client, uint32_t sink_input_index, int volume)
{
    if (!client || !client->connected || volume < 0 || volume > 100) {
        return FALSE;
    }
    
    // Convert percentage to PulseAudio volume
    pa_volume_t pa_volume = (pa_volume_t)((volume * PA_VOLUME_NORM) / 100);
    
    // Find the app to get current volume structure
    GList *item = client->audio_apps;
    pa_cvolume new_volume;
    gboolean found = FALSE;
    
    while (item) {
        app_audio_t *app = (app_audio_t *)item->data;
        if (app->index == sink_input_index) {
            new_volume = app->volume;
            pa_cvolume_set(&new_volume, new_volume.channels, pa_volume);
            found = TRUE;
            break;
        }
        item = item->next;
    }
    
    if (!found) {
        // Default to stereo if app not found
        pa_cvolume_init(&new_volume);
        pa_cvolume_set(&new_volume, 2, pa_volume);
    }
    
    // Send volume change to PulseAudio
    if (client->operation) {
        pa_operation_unref(client->operation);
    }
    client->operation = pa_context_set_sink_input_volume(client->context,
                                                        sink_input_index,
                                                        &new_volume,
                                                        NULL, NULL);
    
    return client->operation != NULL;
}

gboolean pulse_client_toggle_app_mute(pulse_client_t *client, uint32_t sink_input_index)
{
    if (!client || !client->connected) {
        return FALSE;
    }
    
    // Find the app to get current mute state
    GList *item = client->audio_apps;
    gboolean current_muted = FALSE;
    
    while (item) {
        app_audio_t *app = (app_audio_t *)item->data;
        if (app->index == sink_input_index) {
            current_muted = app->muted;
            break;
        }
        item = item->next;
    }
    
    // Toggle mute state
    gboolean new_mute_state = !current_muted;
    
    // Clean up previous operation
    if (client->operation) {
        pa_operation_unref(client->operation);
        client->operation = NULL;
    }
    client->operation = pa_context_set_sink_input_mute(client->context,
                                                      sink_input_index,
                                                      new_mute_state ? 1 : 0,
                                                      NULL, NULL);
    
    return client->operation != NULL;
}

// Helper functions for app_audio_t
app_audio_t* app_audio_new(uint32_t index, const char *name, const char *process_name,
                          const pa_cvolume *volume, gboolean muted)
{
    app_audio_t *app = g_malloc0(sizeof(app_audio_t));
    app->index = index;
    app->name = g_strdup(name ? name : "Unknown Application");
    app->process_name = g_strdup(process_name ? process_name : "unknown");
    if (volume) {
        app->volume = *volume;
    } else {
        pa_cvolume_init(&app->volume);
    }
    app->muted = muted;
    return app;
}

void app_audio_free(app_audio_t *app)
{
    if (app) {
        g_free(app->name);
        g_free(app->process_name);
        g_free(app);
    }
}

int app_audio_get_volume_percent(const app_audio_t *app)
{
    if (!app) {
        return 0;
    }
    return (int)((pa_cvolume_avg(&app->volume) * 100) / PA_VOLUME_NORM);
}

static void sink_input_info_callback(pa_context *c, const pa_sink_input_info *info, int eol, void *userdata)
{
    pulse_client_t *client = (pulse_client_t *)userdata;
    
    if (eol > 0) {
        return;
    }
    
    if (!info) {
        return;
    }
    
    // Extract application name from properties
    const char *app_name = pa_proplist_gets(info->proplist, PA_PROP_APPLICATION_NAME);
    const char *process_name = pa_proplist_gets(info->proplist, PA_PROP_APPLICATION_PROCESS_BINARY);
    
    if (!app_name) {
        app_name = pa_proplist_gets(info->proplist, "application.name");
    }
    
    if (!process_name) {
        process_name = pa_proplist_gets(info->proplist, "application.process.binary");
    }
    
    // Create new app audio entry
    app_audio_t *app = app_audio_new(info->index, app_name, process_name, &info->volume, info->mute);
    
    // Add to list
    client->audio_apps = g_list_append(client->audio_apps, app);
    
    printf("Found audio app: %s (process: %s, index=%u, volume=%d%%, muted=%s)\n",
           app->name, app->process_name, app->index, 
           app_audio_get_volume_percent(app),
           app->muted ? "yes" : "no");
}