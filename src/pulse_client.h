#ifndef PULSE_CLIENT_H
#define PULSE_CLIENT_H

#include <pulse/pulseaudio.h>
#include <glib.h>

// Structure to represent an audio application (sink input)
typedef struct {
    uint32_t index;           // PulseAudio sink input index
    char *name;               // Application name
    char *process_name;       // Process name for icon lookup
    pa_cvolume volume;        // Current volume levels
    gboolean muted;           // Mute state
} app_audio_t;

typedef struct {
    pa_mainloop *mainloop;
    pa_mainloop_api *mainloop_api;
    pa_context *context;
    pa_operation *operation;
    gboolean connected;
    uint32_t default_sink_index;
    pa_cvolume default_sink_volume;
    gboolean default_sink_muted;
    GList *audio_apps;        // List of app_audio_t
} pulse_client_t;

// Initialize PulseAudio client
gboolean pulse_client_init(pulse_client_t *client);

// Cleanup PulseAudio client
void pulse_client_cleanup(pulse_client_t *client);

// Connect to PulseAudio server
gboolean pulse_client_connect(pulse_client_t *client);

// Disconnect from PulseAudio server
void pulse_client_disconnect(pulse_client_t *client);

// Get current master volume (0-100)
int pulse_client_get_master_volume(pulse_client_t *client);

// Set master volume (0-100)
gboolean pulse_client_set_master_volume(pulse_client_t *client, int volume);

// Increase master volume by percentage
gboolean pulse_client_increase_master_volume(pulse_client_t *client, int delta);

// Decrease master volume by percentage
gboolean pulse_client_decrease_master_volume(pulse_client_t *client, int delta);

// Toggle master mute
gboolean pulse_client_toggle_master_mute(pulse_client_t *client);

// Process PulseAudio events (call periodically)
void pulse_client_iterate(pulse_client_t *client);

// Application management functions
void pulse_client_refresh_apps(pulse_client_t *client);
GList* pulse_client_get_apps(pulse_client_t *client);
gboolean pulse_client_set_app_volume(pulse_client_t *client, uint32_t sink_input_index, int volume);
gboolean pulse_client_toggle_app_mute(pulse_client_t *client, uint32_t sink_input_index);

// Helper functions for app_audio_t
app_audio_t* app_audio_new(uint32_t index, const char *name, const char *process_name, 
                          const pa_cvolume *volume, gboolean muted);
void app_audio_free(app_audio_t *app);
int app_audio_get_volume_percent(const app_audio_t *app);

// Volume conversion helper
pa_volume_t pulse_client_percent_to_pa_volume(int volume_percent);

#endif // PULSE_CLIENT_H