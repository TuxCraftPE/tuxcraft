#ifndef NET_MINECRAFT_WORLD_ENTITY_MONSTER__Tux_H__
#define NET_MINECRAFT_WORLD_ENTITY_MONSTER__Tux_H__

#include "Monster.h"
#include "TuxAssets.h"
#if !defined(ANDROID)
#include <AL/al.h>
#else
typedef unsigned int ALuint;
#endif

class Level;

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

    bool isHoldingFlower() const { return heldFlowerId != 0; }
    int getHeldFlowerId() const { return heldFlowerId; }
    void setHoldingFlower(int flowerId) { heldFlowerId = flowerId; }
    void setHoldingFlower(bool holding) { heldFlowerId = holding ? 37 : 0; }

    void playSound(TuxSoundId soundId, bool interrupt = false);
    bool isAudioPlaying();
    void updateAudio(float relX, float relY, float relZ);
    void playPickupMelody();

protected:
    ALuint alSource;
    int heldFlowerId;
    int idleSoundTimer;
    int splashCooldown;
    int wanderTimer;
    int lastIdleSound;
    int lastPickupMelody;
    int lastSoundId;
};

#endif
