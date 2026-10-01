#ifndef NET_MINECRAFT_WORLD_ITEM__TuxSpawnItem_H__
#define NET_MINECRAFT_WORLD_ITEM__TuxSpawnItem_H__

#include "Item.h"
#include "../entity/player/Player.h"
#include "../entity/monster/Tux.h"
#include "../level/Level.h"
#include "../Facing.h"
#include "../../util/Mth.h"

class TuxSpawnItem : public Item {
    typedef Item super;
public:
    TuxSpawnItem(int id) : super(id) {
        maxStackSize = 64;
    }

    virtual bool useOn(ItemInstance* itemInstance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ) {
        if (!level) return false;
        int sx = x;
        int sy = y;
        int sz = z;
        if (face >= 0 && face < 6) {
            sx += Facing::STEP_X[face];
            sy += Facing::STEP_Y[face];
            sz += Facing::STEP_Z[face];
        }

        Tux* tux = new Tux(level);
        tux->setPos((float)sx + 0.5f, (float)sy, (float)sz + 0.5f);
        level->addEntity(tux);

        if (!player->abilities.instabuild) {
            itemInstance->count--;
        }
        return true;
    }

    virtual ItemInstance* use(ItemInstance* instance, Level* level, Player* player) {
        if (!level || !player) return instance;
        float sx = player->x - Mth::sin(player->yRot * Mth::RADDEG) * 2.0f;
        float sy = player->y;
        float sz = player->z + Mth::cos(player->yRot * Mth::RADDEG) * 2.0f;

        Tux* tux = new Tux(level);
        tux->setPos(sx, sy, sz);
        level->addEntity(tux);

        if (!player->abilities.instabuild) {
            instance->count--;
        }
        return instance;
    }
};

#endif
