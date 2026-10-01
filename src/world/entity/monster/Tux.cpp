#include "Tux.h"
#include "../../level/Level.h"
#include "../player/Player.h"
#include "../item/ItemEntity.h"
#include "../../item/ItemInstance.h"
#include "../../level/tile/Tile.h"
#include "../EntityRendererId.h"
#include "../EntityTypes.h"
#include "../ai/goal/GoalSelector.h"
#include "../ai/control/JumpControl.h"
#include "../ai/control/MoveControl.h"
#include "../ai/goal/RandomStrollGoal.h"
#include "../ai/PathNavigation.h"

#if !defined(ANDROID)
#include <AL/al.h>
#include <AL/alc.h>
#endif
#include <cmath>
#include <vector>

#if !defined(ANDROID)
static ALuint g_tuxSoundBuffers[TUX_SOUND_COUNT] = { 0 };
static bool g_tuxSoundBuffersLoaded = false;

static void initTuxAudioBuffers() {
    if (g_tuxSoundBuffersLoaded) return;
    for (int i = 0; i < TUX_SOUND_COUNT; i++) {
        const TuxAudioTrack& track = getTuxAudioTrack((TuxSoundId)i);
        if (track.pcmData && track.pcmSize > 0) {
            alGenBuffers(1, &g_tuxSoundBuffers[i]);
            alBufferData(g_tuxSoundBuffers[i], AL_FORMAT_MONO16, track.pcmData, (ALsizei)track.pcmSize, (ALsizei)track.sampleRate);
        }
    }
    g_tuxSoundBuffersLoaded = true;
}
#endif

Tux::Tux(Level* level)
:   super(level),
    alSource(0),
    heldFlowerId(0),
    splashCooldown(0),
    wanderTimer(0),
    lastIdleSound(-1),
    lastPickupMelody(-1),
    lastSoundId(-1)
{
    entityRendererId = ER_TUX_RENDERER;
    textureName = "mob/tux.png";
    setSize(0.75f, 1.3f);
    runSpeed = 0.35f;
    attackDamage = 0;

    idleSoundTimer = 200 + random.nextInt(300);

    targetSelector = new GoalSelector();

    goalSelector = new GoalSelector();
    goalSelector->addGoal(1, new RandomStrollGoal(this, 0.25f));

    moveControl = new MoveControl(this);
    jumpControl = new JumpControl(this);
}

Tux::~Tux() {
#if !defined(ANDROID)
    if (alSource != 0) {
        alSourceStop(alSource);
        alDeleteSources(1, &alSource);
        alSource = 0;
    }
#endif
    delete goalSelector;
    delete targetSelector;
    delete moveControl;
    delete jumpControl;
}

int Tux::getMaxHealth() {
    return 20;
}

int Tux::getEntityTypeId() const {
    return MobTypes::Tux;
}

Entity* Tux::findAttackTarget() {
    return NULL;
}

void Tux::die(Entity* source) {
#if !defined(ANDROID)
    if (alSource != 0) {
        alSourceStop(alSource);
        alDeleteSources(1, &alSource);
        alSource = 0;
    }
#endif
    super::die(source);
}

bool Tux::hurt(Entity* source, int damage) {
    bool res = super::hurt(source, damage);
    if (res) {
        playSound(TUX_SOUND_YETI_GNA, true);
    }
    return res;
}

std::string Tux::getHurtSound() {
    return "";
}

std::string Tux::getDeathSound() {
    return "";
}

void Tux::causeFallDamage(float distance) {
    super::causeFallDamage(distance);
    if (distance > 1.2f) {
        playSound(TUX_SOUND_FALL, false);
    }
}

void Tux::jumpFromGround() {
    super::jumpFromGround();
    if (random.nextInt(5) == 0) {
        playSound(TUX_SOUND_BIGJUMP, false);
    } else {
        playSound(TUX_SOUND_JUMP, false);
    }
}

void Tux::playPickupMelody() {
    TuxSoundId melody = (lastPickupMelody == TUX_SOUND_SPEECH_GOODDAY) ? TUX_SOUND_SINGSAGAIN : TUX_SOUND_SPEECH_GOODDAY;
    lastPickupMelody = melody;
    playSound(melody, true);
}

bool Tux::isAudioPlaying() {
#if !defined(ANDROID)
    if (alSource == 0) return false;
    ALint state = 0;
    alGetSourcei(alSource, AL_SOURCE_STATE, &state);
    return state == AL_PLAYING;
#else
    return false;
#endif
}

void Tux::playSound(TuxSoundId id, bool interrupt) {
#if !defined(ANDROID)
    if (!isAlive()) return;
    ALCcontext* ctx = alcGetCurrentContext();
    if (!ctx) return;

    initTuxAudioBuffers();
    if (g_tuxSoundBuffers[id] == 0) return;

    if (!interrupt && isAudioPlaying()) return;

    if (alSource == 0) {
        alGenSources(1, &alSource);
        if (alSource == 0) return;
    }

    alSourceStop(alSource);
    alSourcei(alSource, AL_BUFFER, g_tuxSoundBuffers[id]);
    alSourcei(alSource, AL_LOOPING, AL_FALSE);
    alSourcef(alSource, AL_GAIN, 1.0f);
    alSourcef(alSource, AL_PITCH, 1.0f);
    alSourcef(alSource, AL_REFERENCE_DISTANCE, 4.0f);
    alSourcef(alSource, AL_MAX_DISTANCE, 45.0f);
    alSourcef(alSource, AL_ROLLOFF_FACTOR, 1.0f);

    Player* p = level ? level->getNearestPlayer(this, 128.0f) : NULL;
    float rx = p ? (x - p->x) : 0.0f;
    float ry = p ? (y - p->y) : 0.0f;
    float rz = p ? (z - p->z) : 0.0f;
    alSource3f(alSource, AL_POSITION, rx, ry, rz);

    alSourcePlay(alSource);
    lastSoundId = id;
#endif
}

static inline bool isFlowerTile(int id) {
    return (Tile::flower && id == Tile::flower->id) || (Tile::rose && id == Tile::rose->id);
}

void Tux::updateAi() {
    noActionTime = 0;
    jumping = false;

    if (level && !level->isClientSide) {
        bool targetHandled = false;

        AABB searchBox = bb.grow(12.0f, 4.0f, 12.0f);
        std::vector<Entity*> nearby = level->getEntities(this, searchBox);
        ItemEntity* nearestFlower = NULL;
        float nearestDistSqr = 9999.0f;
        for (size_t i = 0; i < nearby.size(); ++i) {
            Entity* e = nearby[i];
            if (!e || e->removed) continue;
            if (e->isItemEntity()) {
                ItemEntity* ie = (ItemEntity*)e;
                if (isFlowerTile(ie->item.id)) {
                    float d = distanceToSqr(ie);
                    if (d < nearestDistSqr) {
                        nearestDistSqr = d;
                        nearestFlower = ie;
                    }
                }
            }
        }

        if (nearestFlower) {
            targetHandled = true;
            lookAt(nearestFlower, 30.0f, 30.0f);
            float dx = nearestFlower->x - x;
            float dz = nearestFlower->z - z;
            float distSqr = dx * dx + dz * dz;
            if (distSqr < 1.4f * 1.4f) {
                int pickedId = nearestFlower->item.id;
                nearestFlower->remove();
                setHoldingFlower(pickedId);
                playPickupMelody();
                yya = 0.0f;
            } else {
                float targetYaw = std::atan2(dz, dx) * Mth::RADDEG - 90.0f;
                yRot = targetYaw;
                yBodyRot = targetYaw;
                yya = 0.45f;
                if (horizontalCollision && onGround) jumping = true;
            }
        } else {
            Player* p = level->getNearestPlayer(this, 16.0f);
            if (p) {
                ItemInstance* carried = p->getCarriedItem();
                if (carried && isFlowerTile(carried->id)) {
                    targetHandled = true;
                    lookAt(p, 30.0f, 30.0f);
                    float dx = p->x - x;
                    float dz = p->z - z;
                    float distSqr = dx * dx + dz * dz;
                    if (distSqr > 2.2f * 2.2f) {
                        float targetYaw = std::atan2(dz, dx) * Mth::RADDEG - 90.0f;
                        yRot = targetYaw;
                        yBodyRot = targetYaw;
                        yya = 0.40f;
                        if (horizontalCollision && onGround) jumping = true;
                    } else {
                        yya = 0.0f;
                    }
                }
            }
        }

        if (!targetHandled) {
            if (wanderTimer > 0) {
                wanderTimer--;
                yya = 0.32f;
                if (horizontalCollision && onGround) jumping = true;
            } else {
                yya = 0.0f;
                if (random.nextInt(35) == 0) {
                    wanderTimer = 35 + random.nextInt(55);
                    yRot += (random.nextFloat() * 140.0f - 70.0f);
                    yBodyRot = yRot;
                }
            }
        }
    }
}

void Tux::aiStep() {
    super::aiStep();
    noActionTime = 0;

    if (isInWater()) {
        if (splashCooldown > 0) splashCooldown--;
        if (splashCooldown <= 0 && (std::abs(xd) > 0.015f || std::abs(zd) > 0.015f || std::abs(yd) > 0.015f)) {
            playSound(TUX_SOUND_SPLASH, false);
            splashCooldown = 40;
        }
    }

    if (idleSoundTimer > 0) {
        idleSoundTimer--;
    } else {
        static const TuxSoundId idleSounds[] = { TUX_SOUND_HQ, TUX_SOUND_SINGSAGAIN, TUX_SOUND_SPEECH_GOODDAY };
        int idx = random.nextInt(3);
        if (idleSounds[idx] == lastIdleSound) {
            idx = (idx + 1) % 3;
        }
        lastIdleSound = idleSounds[idx];
        playSound(idleSounds[idx], false);
        idleSoundTimer = 350 + random.nextInt(400);
    }
}

void Tux::updateAudio(float relX, float relY, float relZ) {
#if !defined(ANDROID)
    if (!isAlive()) {
        if (alSource != 0) {
            alSourceStop(alSource);
            alDeleteSources(1, &alSource);
            alSource = 0;
        }
        return;
    }
    if (alSource != 0) {
        alSource3f(alSource, AL_POSITION, relX, relY, relZ);
    }
#endif
}

void Tux::tick() {
    Mob::tick();

    Player* p = level ? level->getNearestPlayer(this, 128.0f) : NULL;
    float rx = p ? (x - p->x) : 0.0f;
    float ry = p ? (y - p->y) : 0.0f;
    float rz = p ? (z - p->z) : 0.0f;
    updateAudio(rx, ry, rz);
}
