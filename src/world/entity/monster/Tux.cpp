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
#include "../projectile/Arrow.h"
#include "../../../nbt/CompoundTag.h"

#include "../../level/material/Material.h"

#if !defined(ANDROID) && !defined(__ANDROID__)
#include <AL/al.h>
#include <AL/alc.h>
#endif
#include <cmath>
#include <vector>

#if !defined(ANDROID) && !defined(__ANDROID__)
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

#if defined(ANDROID) || defined(__ANDROID__)
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

static SLObjectItf g_slEngineObj = NULL;
static SLEngineItf g_slEngine = NULL;
static SLObjectItf g_slOutputMix = NULL;
static bool g_slInitialized = false;

static void initTuxOpenSL() {
    if (g_slInitialized) return;
    g_slInitialized = true;
    if (slCreateEngine(&g_slEngineObj, 0, NULL, 0, NULL, NULL) != SL_RESULT_SUCCESS) return;
    if ((*g_slEngineObj)->Realize(g_slEngineObj, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS) return;
    if ((*g_slEngineObj)->GetInterface(g_slEngineObj, SL_IID_ENGINE, &g_slEngine) != SL_RESULT_SUCCESS) return;
    if ((*g_slEngine)->CreateOutputMix(g_slEngine, &g_slOutputMix, 0, NULL, NULL) != SL_RESULT_SUCCESS) return;
    if ((*g_slOutputMix)->Realize(g_slOutputMix, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS) return;
}
#endif

Tux::Tux(Level* level)
:   super(level),
    alSource(0),
    soundTicksRemaining(0),
    slPlayer(NULL),
    slPlay(NULL),
    slBufferQueue(NULL),
    slVolume(NULL),
    slCurrentRate(0),
    heldFlowerId(0),
    splashCooldown(0),
    wanderTimer(0),
    lastIdleSound(-1),
    lastPickupMelody(-1),
    lastSoundId(-1),
    playerStareDuration(0),
    disdainLookTimer(0),
    disdainCooldown(0),
    admireTimer(0),
    admireCooldown(100),
    admireTargetX(0),
    admireTargetY(0),
    admireTargetZ(0),
    admiringHeldFlower(false),
    tuxScale(1.0f),
    holdingSword(false),
    attackCooldown(0),
    pendingDropFlowerId(0),
    leftHanded(random.nextInt(2) == 0),
    mutualAdmireTimer(0),
    mutualAdmireCooldown(0)
{
    entityRendererId = ER_TUX_RENDERER;
    textureName = "mob/tux.png";
    tuxScale = 0.60f + random.nextFloat() * 0.40f;
    setSize(0.75f * tuxScale, 1.3f * tuxScale);
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
#if !defined(ANDROID) && !defined(__ANDROID__)
    if (alSource != 0) {
        alSourceStop(alSource);
        alDeleteSources(1, &alSource);
        alSource = 0;
    }
#else
    if (slPlayer != NULL) {
        SLObjectItf player = (SLObjectItf)slPlayer;
        (*player)->Destroy(player);
        slPlayer = NULL;
        slPlay = NULL;
        slBufferQueue = NULL;
        slVolume = NULL;
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
#if !defined(ANDROID) && !defined(__ANDROID__)
    if (alSource != 0) {
        alSourceStop(alSource);
        alDeleteSources(1, &alSource);
        alSource = 0;
    }
#else
    if (slPlayer != NULL) {
        SLObjectItf player = (SLObjectItf)slPlayer;
        (*player)->Destroy(player);
        slPlayer = NULL;
        slPlay = NULL;
        slBufferQueue = NULL;
        slVolume = NULL;
    }
#endif
    holdingSword = false;
    super::die(source);
}

void Tux::alertAttack(Entity* target) {
    if (!target) return;
    setAttackTarget(target);
    holdingSword = true;
    attackDamage = 3;
    admireTimer = 0;
    admiringHeldFlower = false;
    disdainLookTimer = 0;
    playerStareDuration = 0;
    wanderTimer = 0;
    attackCooldown = 0;
}

bool Tux::hurt(Entity* source, int damage) {
    bool res = super::hurt(source, damage);
    if (res) {
        playSound(TUX_SOUND_YETI_GNA, true);
        Entity* attacker = NULL;
        if (source != NULL) {
            if (source->isPlayer()) {
                attacker = source;
            } else if (source->isEntityType(EntityTypes::IdArrow)) {
                Arrow* arrow = (Arrow*)source;
                if (arrow->ownerId != 0 && level) {
                    attacker = level->getEntity(arrow->ownerId);
                    if (attacker && !attacker->isPlayer())
                        attacker = NULL;
                }
            }
        }
        if (attacker != NULL) {
            alertAttack(attacker);
            if (level) {
                std::vector<Entity*> nearby = level->getEntities(this, bb.grow(24.0f, 12.0f, 24.0f));
                for (size_t i = 0; i < nearby.size(); ++i) {
                    Entity* e = nearby[i];
                    if (e && !e->removed && e->isEntityType(MobTypes::Tux)) {
                        ((Tux*)e)->alertAttack(attacker);
                    }
                }
            }
        }
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
#if !defined(ANDROID) && !defined(__ANDROID__)
    if (alSource == 0) return false;
    ALint state = 0;
    alGetSourcei(alSource, AL_SOURCE_STATE, &state);
    return state == AL_PLAYING;
#else
    if (soundTicksRemaining > 0) return true;
    if (slBufferQueue == NULL) return false;
    SLAndroidSimpleBufferQueueItf bq = (SLAndroidSimpleBufferQueueItf)slBufferQueue;
    SLAndroidSimpleBufferQueueState qState;
    if ((*bq)->GetState(bq, &qState) == SL_RESULT_SUCCESS) {
        return qState.count > 0;
    }
    return false;
#endif
}

void Tux::playSound(TuxSoundId id, bool interrupt) {
#if !defined(ANDROID) && !defined(__ANDROID__)
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
#else
    if (!isAlive()) return;

    initTuxOpenSL();
    if (!g_slEngine || !g_slOutputMix) return;

    if (!interrupt && isAudioPlaying()) return;

    const TuxAudioTrack& track = getTuxAudioTrack(id);
    if (!track.pcmData || track.pcmSize == 0) return;

    SLObjectItf player = (SLObjectItf)slPlayer;
    SLPlayItf play = (SLPlayItf)slPlay;
    SLAndroidSimpleBufferQueueItf bq = (SLAndroidSimpleBufferQueueItf)slBufferQueue;

    if (player != NULL && slCurrentRate != track.sampleRate) {
        (*player)->Destroy(player);
        player = NULL;
        play = NULL;
        bq = NULL;
        slPlayer = NULL;
        slPlay = NULL;
        slBufferQueue = NULL;
        slVolume = NULL;
    }

    if (player == NULL) {
        SLDataLocator_AndroidSimpleBufferQueue loc_bq = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2 };
        SLDataFormat_PCM mime = {
            SL_DATAFORMAT_PCM,
            (SLuint32)1,
            (SLuint32)(track.sampleRate * 1000),
            (SLuint32)SL_PCMSAMPLEFORMAT_FIXED_16,
            (SLuint32)SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_SPEAKER_FRONT_CENTER,
            SL_BYTEORDER_LITTLEENDIAN
        };
        SLDataSource audioSource = { &loc_bq, &mime };
        SLDataLocator_OutputMix loc_outmix = { SL_DATALOCATOR_OUTPUTMIX, g_slOutputMix };
        SLDataSink audioSink = { &loc_outmix, NULL };

        const SLInterfaceID iids[2] = { SL_IID_BUFFERQUEUE, SL_IID_VOLUME };
        const SLboolean req[2] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };

        if ((*g_slEngine)->CreateAudioPlayer(g_slEngine, &player, &audioSource, &audioSink, 2, iids, req) != SL_RESULT_SUCCESS) {
            return;
        }
        if ((*player)->Realize(player, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS) {
            (*player)->Destroy(player);
            return;
        }
        (*player)->GetInterface(player, SL_IID_PLAY, &play);
        (*player)->GetInterface(player, SL_IID_BUFFERQUEUE, &bq);
        (*player)->GetInterface(player, SL_IID_VOLUME, &slVolume);

        slPlayer = (void*)player;
        slPlay = (void*)play;
        slBufferQueue = (void*)bq;
        slCurrentRate = track.sampleRate;
    }

    if (play && bq) {
        (*play)->SetPlayState(play, SL_PLAYSTATE_STOPPED);
        (*bq)->Clear(bq);
        (*bq)->Enqueue(bq, track.pcmData, track.pcmSize);

        Player* p = level ? level->getNearestPlayer(this, 128.0f) : NULL;
        float rx = p ? (x - p->x) : 0.0f;
        float ry = p ? (y - p->y) : 0.0f;
        float rz = p ? (z - p->z) : 0.0f;
        updateAudio(rx, ry, rz);

        (*play)->SetPlayState(play, SL_PLAYSTATE_PLAYING);
        soundTicksRemaining = (int)((track.pcmSize / (float)(track.sampleRate * 2)) * 20.0f) + 2;
        lastSoundId = id;
    }
#endif
}

static inline bool isAdmirableTile(int id) {
    return (Tile::flower && id == Tile::flower->id) ||
           (Tile::rose && id == Tile::rose->id) ||
           (Tile::mushroom1 && id == Tile::mushroom1->id) ||
           (Tile::mushroom2 && id == Tile::mushroom2->id) ||
           (Tile::sapling && id == Tile::sapling->id) ||
           id == 37 || id == 38 || id == 39 || id == 40 || id == 6;
}

static inline bool isFlowerTile(int id) {
    return id == 37 || id == 38 ||
           (Tile::flower && id == Tile::flower->id) ||
           (Tile::rose && id == Tile::rose->id);
}

void Tux::travel(float xa, float ya) {
    if (isInWater()) {
        float yo = y;
        moveRelative(xa, ya, 0.02f);
        move(xd, yd, zd);

        xd *= 0.80f;
        yd *= 0.80f;
        zd *= 0.80f;

        yd += 0.035f;
        if (yd > 0.15f) yd = 0.15f;

        if (horizontalCollision) {
            float forwardX = -std::sin(yRot * Mth::DEGRAD);
            float forwardZ = std::cos(yRot * Mth::DEGRAD);
            AABB stepBox = bb.cloneMove(forwardX * 0.35f, 0.7f, forwardZ * 0.35f);
            if (level->getCubes(this, stepBox).empty()) {
                yd = 0.38f;
                xd += forwardX * 0.15f;
                zd += forwardZ * 0.15f;
            }
        }
    } else {
        super::travel(xa, ya);
    }
}

void Tux::updateAi() {
    noActionTime = 0;
    jumping = isInWater();

    if (level && !level->isClientSide) {
        bool targetHandled = false;

        Player* p = level->getNearestPlayer(this, 16.0f);
        Entity* currentTarget = getAttackTarget();

        if (holdingSword) {
            Player* checkPlayer = (currentTarget && currentTarget->isPlayer()) ? (Player*)currentTarget : p;
            bool flowerShown = false;
            if (checkPlayer) {
                ItemInstance* carried = checkPlayer->getCarriedItem();
                if (carried && isFlowerTile(carried->id)) {
                    flowerShown = true;
                }
            }
            if (!flowerShown && p) {
                ItemInstance* carried = p->getCarriedItem();
                if (carried && isFlowerTile(carried->id)) {
                    flowerShown = true;
                }
            }
            if (flowerShown) {
                holdingSword = false;
                setAttackTarget(NULL);
                attackDamage = 0;
                currentTarget = NULL;
            }
        }

        if (holdingSword) {
            if (!currentTarget || !currentTarget->isAlive() || currentTarget->removed) {
                setAttackTarget(NULL);
                holdingSword = false;
                attackDamage = 0;
            } else {
                targetHandled = true;
                wanderTimer = 0;
                lookAt(currentTarget, 30.0f, 30.0f);
                float dx = currentTarget->x - x;
                float dy = (currentTarget->bb.y0 + currentTarget->bb.y1) * 0.5f - (bb.y0 + bb.y1) * 0.5f;
                float dz = currentTarget->z - z;
                float distSqr = dx * dx + dz * dz;

                float targetYaw = std::atan2(dz, dx) * Mth::RADDEG - 90.0f;
                yRot = targetYaw;
                yBodyRot = targetYaw;

                if (distSqr > 1.3f * 1.3f) {
                    yya = 0.55f;
                    if (horizontalCollision) jumping = true;
                } else {
                    yya = 0.0f;
                }

                if (attackCooldown > 0) {
                    attackCooldown--;
                } else {
                    if (distSqr <= 2.2f * 2.2f && std::abs(dy) <= 2.0f) {
                        doHurtTarget(currentTarget);
                        attackCooldown = 20;
                    }
                }
            }
        }

        bool playerStaringAtMe = false;
        if (!targetHandled && p && p->isAlive()) {
            float px = p->x;
            float py = p->y + p->getHeadHeight();
            float pz = p->z;

            float tx = x;
            float ty = (bb.y0 + bb.y1) * 0.5f;
            float tz = z;

            float dx = tx - px;
            float dy = ty - py;
            float dz = tz - pz;
            float dist = std::sqrt(dx * dx + dy * dy + dz * dz);

            if (dist > 0.1f && dist < 10.0f) {
                Vec3 pLook = p->getViewVector(1.0f);
                float ndx = dx / dist;
                float ndy = dy / dist;
                float ndz = dz / dist;
                float dot = pLook.x * ndx + pLook.y * ndy + pLook.z * ndz;

                float hdy = (y + getHeadHeight()) - py;
                float hdist = std::sqrt(dx * dx + hdy * hdy + dz * dz);
                float hdot = hdist > 0.1f ? (pLook.x * (dx / hdist) + pLook.y * (hdy / hdist) + pLook.z * (dz / hdist)) : dot;
                float maxDot = dot > hdot ? dot : hdot;

                if (maxDot > (1.0f - 0.09f / dist) && maxDot > 0.92f && p->canSee(this)) {
                    playerStaringAtMe = true;
                }
            }
        }

        if (disdainCooldown > 0) {
            disdainCooldown--;
            playerStareDuration = 0;
        } else if (playerStaringAtMe) {
            playerStareDuration++;
            if (playerStareDuration >= 70) {
                disdainLookTimer = 60;
                playerStareDuration = 0;
            }
        } else {
            if (playerStareDuration > 0) {
                playerStareDuration--;
            }
        }

        if (disdainLookTimer > 0 && p && p->isAlive()) {
            disdainLookTimer--;
            targetHandled = true;
            wanderTimer = 0;
            yya = 0.0f;

            float tdx = p->x - x;
            float tdz = p->z - z;
            float targetYaw = std::atan2(tdz, tdx) * Mth::RADDEG - 90.0f;
            yRot = targetYaw;
            yBodyRot = targetYaw;

            float pEyeY = p->y + p->getHeadHeight();
            float eyeY = y + getHeadHeight();
            float pyd = pEyeY - eyeY;
            float psd = std::sqrt(tdx * tdx + tdz * tdz);
            xRot = -(float)(std::atan2(pyd, psd) * 180.0f / Mth::PI) - 5.0f;

            if (disdainLookTimer == 0) {
                yRot += (random.nextBoolean() ? 70.0f : -70.0f);
                yBodyRot = yRot;
                xRot = 0.0f;
                disdainCooldown = 240;
                targetHandled = false;
            }
        }

        if (!targetHandled && isHoldingFlower()) {
            if (mutualAdmireCooldown > 0) {
                mutualAdmireCooldown--;
            } else {
                AABB tuxSearchBox = bb.grow(16.0f, 6.0f, 16.0f);
                std::vector<Entity*> nearbyTuxes = level->getEntities(this, tuxSearchBox);
                Tux* nearestFlowerTux = NULL;
                float nearestDistSqr = 9999.0f;

                for (size_t i = 0; i < nearbyTuxes.size(); ++i) {
                    Entity* e = nearbyTuxes[i];
                    if (!e || e == this || e->removed || !e->isAlive()) continue;
                    if (e->isEntityType(MobTypes::Tux)) {
                        Tux* ot = (Tux*)e;
                        if (ot->isHoldingFlower() && !ot->isHoldingSword()) {
                            float d = distanceToSqr(ot);
                            if (d < nearestDistSqr) {
                                nearestDistSqr = d;
                                nearestFlowerTux = ot;
                            }
                        }
                    }
                }

                if (nearestFlowerTux) {
                    targetHandled = true;
                    wanderTimer = 0;
                    admireTimer = 0;
                    admiringHeldFlower = false;

                    float dx = nearestFlowerTux->x - x;
                    float dz = nearestFlowerTux->z - z;
                    float distSqr = dx * dx + dz * dz;

                    float targetYaw = std::atan2(dz, dx) * Mth::RADDEG - 90.0f;
                    yRot = targetYaw;
                    yBodyRot = targetYaw;

                    if (distSqr > 2.0f * 2.0f) {
                        yya = 0.40f;
                        if (horizontalCollision) jumping = true;
                    } else {
                        if (mutualAdmireTimer <= 0) {
                            mutualAdmireTimer = 90 + random.nextInt(40);
                        }
                        mutualAdmireTimer--;

                        if (mutualAdmireTimer <= 0) {
                            mutualAdmireCooldown = 250 + random.nextInt(200);
                            wanderTimer = 50 + random.nextInt(40);
                            yRot += (random.nextBoolean() ? 110.0f : -110.0f);
                            yBodyRot = yRot;
                            yya = 0.35f;
                        } else {
                            yya = 0.0f;

                            float eyeY = y + getHeadHeight();
                            float faceY = nearestFlowerTux->y + nearestFlowerTux->getHeadHeight();
                            float flowerY = nearestFlowerTux->y + 0.65f * nearestFlowerTux->getModelScale();
                            float blend = 0.5f + 0.5f * std::sin((float)tickCount * 0.06f);
                            float targetY = flowerY + (faceY - flowerY) * blend * 0.35f;

                            float dy = targetY - eyeY;
                            float dist = std::sqrt(distSqr);
                            xRot = -(float)(std::atan2(dy, dist > 0.01f ? dist : 0.01f) * 180.0f / Mth::PI);

                            yRot += std::sin((float)tickCount * 0.15f) * 1.5f;
                            yBodyRot = yRot;

                            if (tickCount % 80 == 0 && random.nextInt(2) == 0) {
                                playSound(TUX_SOUND_HQ, false);
                            }
                        }
                    }
                }
            }
        }

        if (!targetHandled && !isHoldingFlower()) {
            AABB searchBox = bb.grow(16.0f, 6.0f, 16.0f);
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
                admireTimer = 0;
                lookAt(nearestFlower, 30.0f, 30.0f);
                float dx = nearestFlower->x - x;
                float dz = nearestFlower->z - z;
                float distSqr = dx * dx + dz * dz;
                if (distSqr < 2.0f * 2.0f || distanceToSqr(nearestFlower) < 2.0f * 2.0f) {
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
                    if (horizontalCollision) jumping = true;
                }
            }
        }

        if (!targetHandled && p) {
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
                    if (horizontalCollision) jumping = true;
                } else {
                    yya = 0.0f;
                }
            }
        }

        if (!targetHandled) {
            if (admireTimer > 0) {
                admireTimer--;
                targetHandled = true;
                wanderTimer = 0;

                if (admiringHeldFlower) {
                    yya = 0.0f;
                    xRot = 35.0f;
                    yRot += std::sin((float)admireTimer * 0.15f) * 0.8f;
                    yBodyRot = yRot;
                    if (admireTimer == 35 && random.nextInt(2) == 0) {
                        playSound(TUX_SOUND_HQ, false);
                    }
                } else {
                    int curTile = level->getTile(admireTargetX, admireTargetY, admireTargetZ);
                    if (!isAdmirableTile(curTile)) {
                        admireTimer = 0;
                    } else {
                        float fx = (float)admireTargetX + 0.5f - x;
                        float fy = (float)admireTargetY + 0.3f - (y + getHeadHeight());
                        float fz = (float)admireTargetZ + 0.5f - z;
                        float distSqr = fx * fx + fz * fz;

                        float targetYaw = std::atan2(fz, fx) * Mth::RADDEG - 90.0f;
                        yRot = targetYaw;
                        yBodyRot = targetYaw;

                        if (distSqr > 2.0f * 2.0f) {
                            yya = 0.30f;
                            if (horizontalCollision) jumping = true;
                        } else {
                            yya = 0.0f;
                            if (!isHoldingFlower() && isFlowerTile(curTile)) {
                                level->setTile(admireTargetX, admireTargetY, admireTargetZ, 0);
                                setHoldingFlower(curTile);
                                playPickupMelody();
                                admireTimer = 0;
                            }
                        }

                        float dist = std::sqrt(distSqr);
                        xRot = -(float)(std::atan2(fy, dist) * 180.0f / Mth::PI);

                        if (admireTimer == 40 && random.nextInt(2) == 0) {
                            playSound(TUX_SOUND_HQ, false);
                        }
                    }
                }

                if (admireTimer == 0) {
                    admiringHeldFlower = false;
                    admireCooldown = 300 + random.nextInt(300);
                }
            } else {
                if (admireCooldown > 0) {
                    admireCooldown--;
                } else {
                    if (isHoldingFlower() && random.nextInt(80) == 0) {
                        admireTimer = 70 + random.nextInt(50);
                        admiringHeldFlower = true;
                        targetHandled = true;
                        wanderTimer = 0;
                        yya = 0.0f;
                    } else if (random.nextInt(60) == 0) {
                        int bx = Mth::floor(x);
                        int by = Mth::floor(y);
                        int bz = Mth::floor(z);
                        bool found = false;
                        for (int dy = -2; dy <= 2 && !found; ++dy) {
                            for (int dx = -5; dx <= 5 && !found; ++dx) {
                                for (int dz = -5; dz <= 5 && !found; ++dz) {
                                    int tid = level->getTile(bx + dx, by + dy, bz + dz);
                                    if (isAdmirableTile(tid)) {
                                        admireTargetX = bx + dx;
                                        admireTargetY = by + dy;
                                        admireTargetZ = bz + dz;
                                        admireTimer = 80 + random.nextInt(60);
                                        admiringHeldFlower = false;
                                        targetHandled = true;
                                        wanderTimer = 0;
                                        found = true;
                                    }
                                }
                            }
                        }
                        if (!found) {
                            admireCooldown = 150 + random.nextInt(150);
                        }
                    }
                }
            }
        }

        if (!targetHandled) {
            if (isInWater()) {
                if (wanderTimer <= 0) {
                    wanderTimer = 50 + random.nextInt(40);
                }
                wanderTimer--;
                yya = 0.35f;
                if (horizontalCollision) jumping = true;
            } else if (wanderTimer > 0) {
                wanderTimer--;
                yya = 0.32f;
                if (horizontalCollision) jumping = true;
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
        if (isUnderLiquid(Material::water) || yd < 0.0f) {
            yd += 0.04f;
        }
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
#if !defined(ANDROID) && !defined(__ANDROID__)
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
#else
    if (!isAlive()) {
        if (slPlayer != NULL) {
            SLObjectItf player = (SLObjectItf)slPlayer;
            (*player)->Destroy(player);
            slPlayer = NULL;
            slPlay = NULL;
            slBufferQueue = NULL;
            slVolume = NULL;
        }
        return;
    }
    if (slVolume != NULL) {
        float dist = std::sqrt(relX * relX + relY * relY + relZ * relZ);
        float vol = 1.0f - dist / 35.0f;
        if (vol < 0.0f) vol = 0.0f;
        SLVolumeItf volItf = (SLVolumeItf)slVolume;
        SLmillibel maxVol = 0;
        (*volItf)->GetMaxVolumeLevel(volItf, &maxVol);
        SLmillibel mbel = (vol <= 0.01f) ? -9600 : (SLmillibel)(maxVol - (1.0f - vol) * 2500.0f);
        (*volItf)->SetVolumeLevel(volItf, mbel);
    }
#endif
}

void Tux::tick() {
    Mob::tick();

    if (soundTicksRemaining > 0) {
        soundTicksRemaining--;
    }

    if (pendingDropFlowerId != 0) {
        if (level && !level->isClientSide) {
            spawnAtLocation(pendingDropFlowerId, 1, 0.3f);
        }
        pendingDropFlowerId = 0;
    }

    Player* p = level ? level->getNearestPlayer(this, 128.0f) : NULL;
    float rx = p ? (x - p->x) : 0.0f;
    float ry = p ? (y - p->y) : 0.0f;
    float rz = p ? (z - p->z) : 0.0f;
    updateAudio(rx, ry, rz);
}

void Tux::addAdditonalSaveData(CompoundTag* tag) {
    super::addAdditonalSaveData(tag);
    tag->putInt("HeldFlower", heldFlowerId != 0 ? heldFlowerId : pendingDropFlowerId);
    tag->putFloat("TuxScale", tuxScale);
    tag->putBoolean("LeftHanded", leftHanded);
}

void Tux::readAdditionalSaveData(CompoundTag* tag) {
    super::readAdditionalSaveData(tag);
    if (tag->contains("TuxScale")) {
        tuxScale = tag->getFloat("TuxScale");
        setSize(0.75f * tuxScale, 1.3f * tuxScale);
    }
    if (tag->contains("LeftHanded")) {
        leftHanded = tag->getBoolean("LeftHanded");
    }
    if (tag->contains("HeldFlower")) {
        int savedFlower = tag->getInt("HeldFlower");
        if (savedFlower != 0) {
            pendingDropFlowerId = savedFlower;
            heldFlowerId = 0;
        }
    }
}
