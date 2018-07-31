/* Definities voor audio aansturing */

#define PCM_DEVICE      "default"
#define PCM_LATENCY     200000
#define PCM_CHANNELS    2
#define PCM_RATE        44100

#define PCM_PATH        "/home/pi/shuttle-panels/audio/"

enum wav_sounds {
    WAV_HUM,
    WAV_ON,
    WAV_OFF,
    WAV_READY,
    WAV_BOOTING,
    WAV_SPARK,
    WAV_COUNT
};

#define WAV_AUDIOFILES {"hum.wav","on.wav","off.wav","ready.wav","booting.wav","spark.wav"}
#define WAV_CHANNELS 2

int init_audio(void);
int fini_audio(void);
void audio_mainloop(void);
int audio_play_file(int channel, enum wav_sounds sound);

/* vim: ai:si:expandtab:ts=4:sw=4
 */
