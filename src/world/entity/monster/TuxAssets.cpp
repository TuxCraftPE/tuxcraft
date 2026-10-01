#include "TuxAssets.h"
#include <cstring>

extern "C" {
    extern const unsigned char tux_mc3d_start[];
    extern const unsigned char tux_mc3d_end[];

    extern const unsigned char tux_rgba_start[];
    extern const unsigned char tux_rgba_end[];

    extern const unsigned char tux_snd_bigjump_start[];
    extern const unsigned char tux_snd_bigjump_end[];

    extern const unsigned char tux_snd_fall_start[];
    extern const unsigned char tux_snd_fall_end[];

    extern const unsigned char tux_snd_jump_start[];
    extern const unsigned char tux_snd_jump_end[];

    extern const unsigned char tux_snd_speech_goodday_start[];
    extern const unsigned char tux_snd_speech_goodday_end[];

    extern const unsigned char tux_snd_splash_start[];
    extern const unsigned char tux_snd_splash_end[];

    extern const unsigned char tux_snd_singsagain_start[];
    extern const unsigned char tux_snd_singsagain_end[];

    extern const unsigned char tux_snd_hq_start[];
    extern const unsigned char tux_snd_hq_end[];

    extern const unsigned char tux_snd_yeti_gna_start[];
    extern const unsigned char tux_snd_yeti_gna_end[];

    extern const unsigned char tux_icon_rgba_start[];
    extern const unsigned char tux_btn_multi_start[];
    extern const unsigned char tux_btn_options_start[];
    extern const unsigned char tux_btn_play_start[];
}

const TuxAudioTrack& getTuxAudioTrack(TuxSoundId id) {
    static TuxAudioTrack tracks[TUX_SOUND_COUNT];
    static bool inited = false;
    if (!inited) {
        tracks[TUX_SOUND_BIGJUMP] = { tux_snd_bigjump_start, (size_t)(tux_snd_bigjump_end - tux_snd_bigjump_start), 44100 };
        tracks[TUX_SOUND_FALL] = { tux_snd_fall_start, (size_t)(tux_snd_fall_end - tux_snd_fall_start), 22050 };
        tracks[TUX_SOUND_JUMP] = { tux_snd_jump_start, (size_t)(tux_snd_jump_end - tux_snd_jump_start), 44100 };
        tracks[TUX_SOUND_SPEECH_GOODDAY] = { tux_snd_speech_goodday_start, (size_t)(tux_snd_speech_goodday_end - tux_snd_speech_goodday_start), 44100 };
        tracks[TUX_SOUND_SPLASH] = { tux_snd_splash_start, (size_t)(tux_snd_splash_end - tux_snd_splash_start), 44100 };
        tracks[TUX_SOUND_SINGSAGAIN] = { tux_snd_singsagain_start, (size_t)(tux_snd_singsagain_end - tux_snd_singsagain_start), 44100 };
        tracks[TUX_SOUND_HQ] = { tux_snd_hq_start, (size_t)(tux_snd_hq_end - tux_snd_hq_start), 44100 };
        tracks[TUX_SOUND_YETI_GNA] = { tux_snd_yeti_gna_start, (size_t)(tux_snd_yeti_gna_end - tux_snd_yeti_gna_start), 44100 };
        inited = true;
    }
    if (id < 0 || id >= TUX_SOUND_COUNT) return tracks[0];
    return tracks[id];
}

const void* getTuxModelData(size_t* outSize) {
    if (outSize) *outSize = (size_t)(tux_mc3d_end - tux_mc3d_start);
    return tux_mc3d_start;
}

const void* getTuxTextureRgba(int* outW, int* outH) {
    if (outW) *outW = 256;
    if (outH) *outH = 256;
    return tux_rgba_start;
}

const void* getTuxIconRgba(int* outW, int* outH) {
    if (outW) *outW = 250;
    if (outH) *outH = 250;
    return tux_icon_rgba_start;
}

const void* getTuxButtonRgba(const char* name, int* outW, int* outH) {
    if (outW) *outW = 64;
    if (outH) *outH = 64;
    if (!name) return NULL;
    if (strstr(name, "multi") || strstr(name, "Multi")) return tux_btn_multi_start;
    if (strstr(name, "options") || strstr(name, "Options")) return tux_btn_options_start;
    if (strstr(name, "play") || strstr(name, "Play")) return tux_btn_play_start;
    return NULL;
}
