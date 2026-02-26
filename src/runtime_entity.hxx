#ifndef RUNTIME_ENTITY_HXX_INCLUDED
#define RUNTIME_ENTITY_HXX_INCLUDED

#include "amb_types.hxx"
#include "runtime_object.hxx"

namespace amb::runtime {
    enum class FacingDirection : u8 {
        down = 0,
        left = 1,
        right = 2,
        up = 3,
    };

    struct EntityRenderable {
        float world_x = 0.0f;
        float world_y = 0.0f;
    };

    class Entity : public RuntimeObject {
    public:
        Entity() = default;
        virtual ~Entity() = default;

        EntityRenderable& renderable() noexcept { return m_renderable; }
        const EntityRenderable& renderable() const noexcept { return m_renderable; }

        FacingDirection facing() const noexcept { return m_facing; }
        void setFacing(FacingDirection facing) noexcept { m_facing = facing; }

        float velocity() const noexcept { return m_velocity; }
        void setVelocity(float velocity) noexcept { m_velocity = velocity; }

    protected:
        EntityRenderable m_renderable {};
        FacingDirection m_facing = FacingDirection::down;
        float m_velocity = 0.0f;
    };

    class PlayerEntity final : public Entity {
    public:
        PlayerEntity() = default;
        ~PlayerEntity() override = default;

        const char* typeName() const noexcept override { return "PlayerEntity"; }
    };
}

#endif
