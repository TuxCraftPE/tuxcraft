#ifndef NET_MINECRAFT_WORLD_ENTITY_MONSTER__Tux_H__
#define NET_MINECRAFT_WORLD_ENTITY_MONSTER__Tux_H__

#include "Monster.h"
#include "TuxAssets.h"
#if !defined(ANDROID) && !defined(__ANDROID__)
#include <AL/al.h>
#else
typedef unsigned int ALuint;
#endif

class Level;
class CompoundTag;

class Tux : public Monster {
    typedef Monster super;
public:
    Tux(Level* level);
    virtual ~Tux();

    virtual int getMaxHealth();
    virtual void aiStep();
    virtual void tick();
    virtual int getEntityTypeId() const;
    virtual void die(Entity* source);
    virtual Entity* findAttackTarget();
    virtual bool hurt(Entity* source, int damage);
    virtual std::string getHurtSound();
    virtual std::string getDeathSound();
    virtual void causeFallDamage(float distance);
    virtual void jumpFromGround();
    virtual bool removeWhenFarAway() { return false; }

    virtual void updateAi();
    virtual void travel(float xa, float ya);

    bool isHoldingFlower() const { return heldFlowerId != 0; }
    int getHeldFlowerId() const { return heldFlowerId; }
    void setHoldingFlower(int flowerId) { heldFlowerId = flowerId; }
    void setHoldingFlower(bool holding) { heldFlowerId = holding ? 37 : 0; }
    float getModelScale() const { return tuxScale; }
    bool isHoldingSword() const { return holdingSword; }
    void setHoldingSword(bool holding) { holdingSword = holding; }
    bool isLeftHanded() const { return leftHanded; }
    void setLeftHanded(bool left) { leftHanded = left; }
    void alertAttack(Entity* target);

    virtual void addAdditonalSaveData(CompoundTag* tag);
    virtual void readAdditionalSaveData(CompoundTag* tag);

    void playSound(TuxSoundId soundId, bool interrupt = false);
    bool isAudioPlaying();
    void updateAudio(float relX, float relY, float relZ);
    void playPickupMelody();

protected:
    ALuint alSource;
    int soundTicksRemaining;
    void* slPlayer;
    void* slPlay;
    void* slBufferQueue;
    void* slVolume;
    uint32_t slCurrentRate;
    int heldFlowerId;
    int idleSoundTimer;
    int splashCooldown;
    int wanderTimer;
    int lastIdleSound;
    int lastPickupMelody;
    int lastSoundId;
    int playerStareDuration;
    int disdainLookTimer;
    int disdainCooldown;
    int admireTimer;
    int admireCooldown;
    int admireTargetX;
    int admireTargetY;
    int admireTargetZ;
    bool admiringHeldFlower;
    float tuxScale;
    bool holdingSword;
    int attackCooldown;
    int pendingDropFlowerId;
    bool leftHanded;
    int mutualAdmireTimer;
    int mutualAdmireCooldown;
};

#endif
