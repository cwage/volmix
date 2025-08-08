#ifndef PULSE_CLIENT_H
#define PULSE_CLIENT_H

#include <pulse/pulseaudio.h>
#include <glib.h>

typedef struct {
    pa_mainloop *mainloop;
    pa_mainloop_api *mainloop_api;
    pa_context *context;
    pa_operation *operation;
    gboolean connected;
    uint32_t default_sink_index;
    pa_cvolume default_sink_volume;
    gboolean default_sink_muted;
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

#endif // PULSE_CLIENT_H