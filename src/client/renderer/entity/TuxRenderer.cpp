#include "TuxRenderer.h"
#include "EntityRenderDispatcher.h"
#include "tux_tri_parts.h"
#include "../ItemInHandRenderer.h"
#include "../Tesselator.h"
#include "../Textures.h"
#include "../gles.h"
#include "../../../util/Mth.h"
#include "../../../world/entity/Mob.h"
#include "../../../world/entity/monster/Tux.h"
#include "../../../world/entity/monster/TuxAssets.h"
#include "../../../world/level/tile/Tile.h"
#include "../../../world/item/ItemInstance.h"

#include <cstdio>
#include <cstring>
#include <cmath>

bool TuxRenderer::loaded = false;
unsigned int TuxRenderer::vboId = 0;
int TuxRenderer::vertexCount = 0;
const float* TuxRenderer::baseVertices = NULL;
std::vector<float> TuxRenderer::animBuffer;

TuxRenderer::TuxRenderer() {
    shadowRadius = 0.5f;
}

TuxRenderer::~TuxRenderer() {
}

/*static*/
void TuxRenderer::ensureLoaded() {
    if (loaded) return;
    loaded = true;

    size_t modelSize = 0;
    const unsigned char* data = (const unsigned char*)getTuxModelData(&modelSize);
    if (!data || modelSize < 8 || memcmp(data, "MC3D", 4) != 0) {
        return;
    }

    uint32_t count = *(const uint32_t*)(data + 4);
    vertexCount = (int)count;
    baseVertices = (const float*)(data + 8);

    animBuffer.resize(vertexCount * 8);
    memcpy(animBuffer.data(), baseVertices, vertexCount * 8 * sizeof(float));

    glGenBuffers2(1, &vboId);
    if (vboId != 0) {
        glBindBuffer2(GL_ARRAY_BUFFER, vboId);
        glBufferData2(GL_ARRAY_BUFFER, vertexCount * 8 * sizeof(float), animBuffer.data(), GL_DYNAMIC_DRAW);
        glBindBuffer2(GL_ARRAY_BUFFER, 0);
    }
}

static inline void rotatePoint(float dx, float dy, float dz, float pitch, float yaw, float roll, float& outX, float& outY, float& outZ) {
    float cp = std::cos(pitch), sp = std::sin(pitch);
    float y1 = dy * cp - dz * sp;
    float z1 = dy * sp + dz * cp;

    float cy = std::cos(yaw), sy = std::sin(yaw);
    float x2 = dx * cy + z1 * sy;
    float z2 = -dx * sy + z1 * cy;

    float cr = std::cos(roll), sr = std::sin(roll);
    outX = x2 * cr - y1 * sr;
    outY = x2 * sr + y1 * cr;
    outZ = z2;
}

static inline float smoothstep(float edge0, float edge1, float x) {
    float t = (x - edge0) / (edge1 - edge0);
    if (t < 0.0f) return 0.0f;
    if (t > 1.0f) return 1.0f;
    return t * t * (3.0f - 2.0f * t);
}

void TuxRenderer::render(Entity* entity, float x, float y, float z, float rot, float a) {
    if (!entity) return;
    ensureLoaded();
    if (vertexCount == 0 || !baseVertices) return;

    Tux* tux = (Tux*)entity;
    tux->updateAudio(x, y, z);

    float bodyRot = tux->yBodyRotO + (tux->yBodyRot - tux->yBodyRotO) * a;
    float headRot = tux->yRotO + (tux->yRot - tux->yRotO) * a;
    float headPitchDeg = tux->xRotO + (tux->xRot - tux->xRotO) * a;

    float headYaw = (headRot - bodyRot) * Mth::DEGRAD;
    float headPitch = headPitchDeg * Mth::DEGRAD;

    float ws = tux->walkAnimSpeedO + (tux->walkAnimSpeed - tux->walkAnimSpeedO) * a;
    float wp = tux->walkAnimPos - tux->walkAnimSpeed * (1.0f - a);
    if (ws > 1.0f) ws = 1.0f;
    float walkCycle = wp * 1.8f;

    bool holdingFlower = tux->isHoldingFlower();
    bool isTalking = tux->isAudioPlaying();
    float timeAnim = (tux->tickCount + a) * 0.08f;
    float mouthPitch = isTalking ? std::abs(std::sin((tux->tickCount + a) * 0.70f)) * 6.5f * Mth::DEGRAD : 0.0f;

    float walkFactor = std::min(1.0f, ws * 2.2f);
    float waddleRoll = -std::sin(walkCycle) * walkFactor * 8.0f;
    float waddleSway = std::sin(walkCycle) * walkFactor * 0.035f;
    float bodyBob = (1.0f - std::abs(std::cos(walkCycle))) * walkFactor * 0.025f;

    float wingFlap = (10.0f + std::abs(std::sin(walkCycle)) * walkFactor * 22.0f) * Mth::DEGRAD;
    float wingSwing = (std::sin(walkCycle) * walkFactor * 28.0f) * Mth::DEGRAD;

    float sinCycle = std::sin(walkCycle);
    float stepCycleL = sinCycle * walkFactor;
    float stepCycleR = -sinCycle * walkFactor;

    float footStrideL = (stepCycleL > 0.0f ? stepCycleL * 0.12f : stepCycleL * 0.03f);
    float footStrideR = (stepCycleR > 0.0f ? stepCycleR * 0.12f : stepCycleR * 0.03f);
    float footLiftL = (stepCycleL > 0.0f ? std::sin(stepCycleL * 3.14159f / (walkFactor > 0.01f ? walkFactor : 1.0f)) * walkFactor * 0.06f : 0.0f);
    float footLiftR = (stepCycleR > 0.0f ? std::sin(stepCycleR * 3.14159f / (walkFactor > 0.01f ? walkFactor : 1.0f)) * walkFactor * 0.06f : 0.0f);
    float footPitchL = (stepCycleL > 0.0f ? stepCycleL * 18.0f * Mth::DEGRAD : 0.0f);
    float footPitchR = (stepCycleR > 0.0f ? stepCycleR * 18.0f * Mth::DEGRAD : 0.0f);

    float toeYawL = 10.0f * Mth::DEGRAD;
    float toeYawR = -10.0f * Mth::DEGRAD;

    if (walkFactor < 0.05f) {
        float idlePhase = std::sin(timeAnim * 0.8f);
        footLiftL = (idlePhase > 0.0f ? idlePhase : 0.0f) * 0.02f;
        footLiftR = (idlePhase < 0.0f ? -idlePhase : 0.0f) * 0.02f;
        waddleRoll = idlePhase * 1.5f;
    }

    for (int tri = 0; tri < 5304; ++tri) {
        uint8_t part = (tri < 5304) ? tuxTriParts[tri] : 0;
        for (int vi = 0; vi < 3; ++vi) {
            int i = tri * 3 + vi;
            int idx = i * 8;
            float vx = baseVertices[idx + 0];
            float vy = baseVertices[idx + 1];
            float vz = baseVertices[idx + 2];
            float px = vx, py = vy, pz = vz;

            if (part == 0) {
            } else if (part == 1) {
                float rx, ry, rz;
                float w_blend = smoothstep(0.18f, 0.26f, -vx) * (1.0f - smoothstep(0.78f, 0.88f, vy));
                if (holdingFlower) {
                    rotatePoint(vx - (-0.26f), vy - 0.80f, vz - 0.0f, -1.05f, 0.50f, 0.30f, rx, ry, rz);
                    float t = std::max(0.0f, std::min(1.0f, (0.80f - vy) / 0.38f));
                    rx += 0.12f * t;
                    ry -= 0.04f * t;
                    rz += 0.22f * t;
                    px = vx + (rx + (-0.26f) - vx) * w_blend;
                    py = vy + (ry + 0.80f - vy) * w_blend;
                    pz = vz + (rz + 0.0f - vz) * w_blend;
                } else {
                    rotatePoint(vx - (-0.26f), vy - 0.80f, vz - 0.0f, wingSwing, 0.0f, -wingFlap, rx, ry, rz);
                    px = vx + (rx + (-0.26f) - vx) * w_blend;
                    py = vy + (ry + 0.80f - vy) * w_blend;
                    pz = vz + (rz + 0.0f - vz) * w_blend;
                }
            } else if (part == 2) {
                float rx, ry, rz;
                float w_blend = smoothstep(0.18f, 0.26f, vx) * (1.0f - smoothstep(0.78f, 0.88f, vy));
                if (holdingFlower) {
                    rotatePoint(vx - 0.26f, vy - 0.80f, vz - 0.0f, -1.05f, -0.50f, -0.30f, rx, ry, rz);
                    float t = std::max(0.0f, std::min(1.0f, (0.80f - vy) / 0.38f));
                    rx -= 0.12f * t;
                    ry -= 0.04f * t;
                    rz += 0.22f * t;
                    px = vx + (rx + 0.26f - vx) * w_blend;
                    py = vy + (ry + 0.80f - vy) * w_blend;
                    pz = vz + (rz + 0.0f - vz) * w_blend;
                } else {
                    rotatePoint(vx - 0.26f, vy - 0.80f, vz - 0.0f, -wingSwing, 0.0f, wingFlap, rx, ry, rz);
                    px = vx + (rx + 0.26f - vx) * w_blend;
                    py = vy + (ry + 0.80f - vy) * w_blend;
                    pz = vz + (rz + 0.0f - vz) * w_blend;
                }
            } else if (part == 3) {
                float rx, ry, rz;
                rotatePoint(vx - (-0.11f), vy - 0.22f, vz - 0.05f, footPitchL, toeYawL, 0.0f, rx, ry, rz);
                px = rx + (-0.11f);
                py = ry + 0.22f + footLiftL;
                pz = rz + 0.05f + footStrideL;
            } else if (part == 4) {
                float rx, ry, rz;
                rotatePoint(vx - 0.11f, vy - 0.22f, vz - 0.05f, footPitchR, toeYawR, 0.0f, rx, ry, rz);
                px = rx + 0.11f;
                py = ry + 0.22f + footLiftR;
                pz = rz + 0.05f + footStrideR;
            } else if (part == 5) {
                float rx, ry, rz;
                rotatePoint(vx - 0.0f, vy - 0.80f, vz - 0.0f, headPitch, -headYaw, 0.0f, rx, ry, rz);
                px = rx + 0.0f;
                py = ry + 0.80f;
                pz = rz + 0.0f;
            } else if (part == 6) {
                float jx, jy, jz;
                rotatePoint(vx - 0.0f, vy - 0.94f, vz - 0.06f, mouthPitch, 0.0f, 0.0f, jx, jy, jz);
                jx += 0.0f;
                jy += 0.94f;
                jz += 0.06f;

                float rx, ry, rz;
                rotatePoint(jx - 0.0f, jy - 0.80f, jz - 0.0f, headPitch, -headYaw, 0.0f, rx, ry, rz);
                px = rx + 0.0f;
                py = ry + 0.80f;
                pz = rz + 0.0f;
            }

            animBuffer[idx + 0] = px;
            animBuffer[idx + 1] = py;
            animBuffer[idx + 2] = pz;
        }
    }

    bindTexture("mob/tux.png");

    glDisable2(GL_CULL_FACE);
    glDisableClientState2(GL_COLOR_ARRAY);

    glPushMatrix2();
    glTranslatef2(x, y + bodyBob, z);
    glRotatef2(-bodyRot, 0.0f, 1.0f, 0.0f);
    glTranslatef2(waddleSway, 0.0f, 0.0f);
    glRotatef2(waddleRoll, 0.0f, 0.0f, 1.0f);

    float br = entity->getBrightness(a);
    glColor4f2(br, br, br, 1.0f);

    if (vboId != 0) {
        glBindBuffer2(GL_ARRAY_BUFFER, vboId);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertexCount * 8 * sizeof(float), animBuffer.data());
        glEnableClientState2(GL_VERTEX_ARRAY);
        glVertexPointer2(3, GL_FLOAT, 32, (const GLvoid*)0);
        glEnableClientState2(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer2(2, GL_FLOAT, 32, (const GLvoid*)(6 * sizeof(float)));
        glDrawArrays2(GL_TRIANGLES, 0, vertexCount);
        glDisableClientState2(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState2(GL_VERTEX_ARRAY);
        glBindBuffer2(GL_ARRAY_BUFFER, 0);
    } else {
        glEnableClientState2(GL_VERTEX_ARRAY);
        glVertexPointer2(3, GL_FLOAT, 32, (const GLvoid*)animBuffer.data());
        glEnableClientState2(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer2(2, GL_FLOAT, 32, (const GLvoid*)(animBuffer.data() + 6));
        glDrawArrays2(GL_TRIANGLES, 0, vertexCount);
        glDisableClientState2(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState2(GL_VERTEX_ARRAY);
    }

    int flowerId = tux->getHeldFlowerId();
    if (flowerId == 0 && tux->isHoldingFlower()) {
        flowerId = Tile::flower ? Tile::flower->id : 37;
    }

    Tile* flowerTile = NULL;
    if (Tile::rose && flowerId == Tile::rose->id) {
        flowerTile = Tile::rose;
    } else if (Tile::flower && flowerId == Tile::flower->id) {
        flowerTile = Tile::flower;
    } else if (flowerId != 0 && Tile::tiles[flowerId]) {
        flowerTile = Tile::tiles[flowerId];
    }

    if (flowerTile) {
        bindTexture("terrain.png");
        glPushMatrix2();
        glTranslatef2(0.0f, 0.68f, 0.55f);
        glScalef2(0.44f, 0.44f, 0.44f);
        glRotatef2(8.0f, 1.0f, 0.0f, 0.0f);

        int tex = flowerTile->getTexture(0, 0);
        int xt = (tex & 0xf) << 4;
        int yt = tex & 0xf0;
        float u0 = (xt + 0.00f) / 256.0f;
        float u1 = (xt + 15.99f) / 256.0f;
        float v0 = (yt + 0.00f) / 256.0f;
        float v1 = (yt + 15.99f) / 256.0f;

        float w = 0.40f;
        float y0 = -0.20f;
        float y1 = 0.80f;

        Tesselator& t = Tesselator::instance;
        t.begin();
        t.color(br, br, br);
        t.vertexUV(-w, y1, -w, u0, v0);
        t.vertexUV(-w, y0, -w, u0, v1);
        t.vertexUV( w, y0,  w, u1, v1);
        t.vertexUV( w, y1,  w, u1, v0);

        t.vertexUV( w, y1,  w, u0, v0);
        t.vertexUV( w, y0,  w, u0, v1);
        t.vertexUV(-w, y0, -w, u1, v1);
        t.vertexUV(-w, y1, -w, u1, v0);

        t.vertexUV(-w, y1,  w, u0, v0);
        t.vertexUV(-w, y0,  w, u0, v1);
        t.vertexUV( w, y0, -w, u1, v1);
        t.vertexUV( w, y1, -w, u1, v0);

        t.vertexUV( w, y1, -w, u0, v0);
        t.vertexUV( w, y0, -w, u0, v1);
        t.vertexUV(-w, y0,  w, u1, v1);
        t.vertexUV(-w, y1,  w, u1, v0);
        t.draw();

        glDisableClientState2(GL_COLOR_ARRAY);
        glDisableClientState2(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState2(GL_VERTEX_ARRAY);
        glBindBuffer2(GL_ARRAY_BUFFER, 0);
        bindTexture("mob/tux.png");

        glPopMatrix2();
    }

    glPopMatrix2();
    glEnable2(GL_CULL_FACE);
}

/*static*/
void TuxRenderer::renderPreview(Textures* textures) {
    ensureLoaded();
    if (vertexCount == 0 || !baseVertices) return;

    if (textures) {
        textures->loadAndBindTexture("mob/tux.png");
    } else {
        EntityRenderDispatcher* disp = EntityRenderDispatcher::getInstance();
        if (disp && disp->textures) {
            disp->textures->loadAndBindTexture("mob/tux.png");
        }
    }

    glDisable2(GL_CULL_FACE);
    glColor4f2(1.0f, 1.0f, 1.0f, 1.0f);

    glEnableClientState2(GL_VERTEX_ARRAY);
    glVertexPointer2(3, GL_FLOAT, 32, (const GLvoid*)baseVertices);
    glEnableClientState2(GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer2(2, GL_FLOAT, 32, (const GLvoid*)(baseVertices + 6));
    glDrawArrays2(GL_TRIANGLES, 0, vertexCount);
    glDisableClientState2(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState2(GL_VERTEX_ARRAY);

    glEnable2(GL_CULL_FACE);
}
