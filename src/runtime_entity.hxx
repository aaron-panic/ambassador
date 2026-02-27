#ifndef RUNTIME_ENTITY_HXX_INCLUDED
#define RUNTIME_ENTITY_HXX_INCLUDED

#include "runtime_object.hxx"

namespace amb::entity {
    class EntityRuntime final : public RuntimeObject {
    public:
        EntityRuntime() = default;
        ~EntityRuntime() override = default;

        const char* typeName() const noexcept override { return "EntityRuntime"; }

        float world_x = 0.0f;
        float world_y = 0.0f;
        float heading_degrees = 0.0f;
        float speed = 0.0f;
    };
}

#endif
