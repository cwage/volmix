#include "pulse_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void context_state_callback(pa_context *c, void *userdata);
static void sink_info_callback(pa_context *c, const pa_sink_info *info, int eol, void *userdata);
static void server_info_callback(pa_context *c, const pa_server_info *info, void *userdata);

gboolean pulse_client_init(pulse_client_t *client)
{
    if (!client) {
        return FALSE;
    }
    
    memset(client, 0, sizeof(pulse_client_t));
    
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
    
    // Wait for connection
    int retval;
    while (TRUE) {
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
            if (client->operation) {
                while (pa_operation_get_state(client->operation) == PA_OPERATION_RUNNING) {
                    pa_mainloop_iterate(client->mainloop, 1, NULL);
                }
                pa_operation_unref(client->operation);
                client->operation = NULL;
            }
            
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