#ifndef NET_MINECRAFT_WORLD_ENTITY_MONSTER__TuxAssets_H__
#define NET_MINECRAFT_WORLD_ENTITY_MONSTER__TuxAssets_H__

#include <cstddef>
#include <cstdint>

enum TuxSoundId {
    TUX_SOUND_BIGJUMP = 0,
    TUX_SOUND_FALL,
    TUX_SOUND_JUMP,
    TUX_SOUND_SPEECH_GOODDAY,
    TUX_SOUND_SPLASH,
    TUX_SOUND_SINGSAGAIN,
    TUX_SOUND_HQ,
    TUX_SOUND_YETI_GNA,
    TUX_SOUND_COUNT
};

struct TuxAudioTrack {
    const void* pcmData;
    size_t pcmSize;
    uint32_t sampleRate;
};

const TuxAudioTrack& getTuxAudioTrack(TuxSoundId id);
const void* getTuxModelData(size_t* outSize);
const void* getTuxTextureRgba(int* outW, int* outH);
const void* getTuxIconRgba(int* outW, int* outH);
const void* getTuxButtonRgba(const char* name, int* outW, int* outH);

#endif
