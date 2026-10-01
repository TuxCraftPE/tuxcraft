#ifndef NET_MINECRAFT_CLIENT_RENDERER_ENTITY__TuxRenderer_H__
#define NET_MINECRAFT_CLIENT_RENDERER_ENTITY__TuxRenderer_H__

#include "EntityRenderer.h"
#include <vector>

class Textures;

class TuxRenderer : public EntityRenderer {
public:
    TuxRenderer();
    virtual ~TuxRenderer();

    virtual void render(Entity* entity, float x, float y, float z, float rot, float a);
    static void renderPreview(Textures* textures = NULL);

private:
    static void ensureLoaded();

    static bool loaded;
    static unsigned int vboId;
    static int vertexCount;
    static const float* baseVertices;
    static std::vector<float> animBuffer;
};

#endif
