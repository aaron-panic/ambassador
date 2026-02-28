#ifndef ENTITY_HXX_INCLUDED
#define ENTITY_HXX_INCLUDED

#include "amb_types.hxx"
#include "runtime_entity.hxx"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace amb::entity {
    constexpr int ROLL_STEP_DEGREES = 15;
    constexpr int ROLL_MAX_DEGREES = 75;
    constexpr int ROLL_MAX_STEPS = ROLL_MAX_DEGREES / ROLL_STEP_DEGREES;

    constexpr int ROLL_LEFT_DIRECTION = -1;
    constexpr int ROLL_RIGHT_DIRECTION = 1;

    struct ControlPreset {
        std::string name;

        float forward_throttle_max_percentage = 1.0f;
        float forward_damper_percentage = 0.0f;
        bool forward_damper_on = true;
        float forward_brake_percentage = 1.0f;
        bool forward_brake_stops_at_zero = true;

        float yaw_throttle_max_percentage = 1.0f;
        float yaw_damper_percentage = 0.0f;
        bool yaw_damper_on = true;
        float yaw_brake_percentage = 1.0f;
        bool yaw_brake_stops_at_zero = true;

        float forward_acceleration_response = 1.0f;
        float yaw_acceleration_response = 1.0f;
    };

    class EntityBehavior {
    public:
        virtual ~EntityBehavior() = default;

        virtual EntityRuntime& runtime() noexcept = 0;
        virtual const EntityRuntime& runtime() const noexcept = 0;

        virtual void integrate(u64 dt_ms) = 0;

        virtual void setForwardThrottle(float velocity) = 0;
        virtual void setForwardThrottleMax(float percentage) = 0;
        virtual void setForwardThrottleDamper(float percentage) = 0;
        virtual void setForwardThrottleDamperOn(bool enabled) = 0;
        virtual bool isForwardThrottleDamperOn() const = 0;
        virtual void setForwardThrottleBrake(float percentage) = 0;
        virtual void useForwardThrottleBrake() = 0;

        virtual void setYawThrottle(float velocity) = 0;
        virtual void setYawThrottleMax(float percentage) = 0;
        virtual void setYawThrottleDamper(float percentage) = 0;
        virtual void setYawThrottleDamperOn(bool enabled) = 0;
        virtual bool isYawThrottleDamperOn() const = 0;
        virtual void setYawThrottleBrake(float percentage) = 0;
        virtual void useYawThrottleBrake() = 0;

        virtual void setRoll(int direction) = 0;
        virtual void stepRoll(int direction) = 0;
        virtual void setYaw(int direction) = 0;
        virtual void stepYaw(int direction) = 0;
    };

    class Entity : public EntityBehavior {
    public:
        explicit Entity(EntityRuntime& runtime, u16 entity_id);
        ~Entity() override = default;

        u16 id() const noexcept;

        EntityRuntime& runtime() noexcept override;
        const EntityRuntime& runtime() const noexcept override;

        void integrate(u64 dt_ms) override;

        void setForwardThrottle(float velocity) override;
        void setForwardThrottleMax(float percentage) override;
        void setForwardThrottleDamper(float percentage) override;
        void setForwardThrottleDamperOn(bool enabled) override;
        bool isForwardThrottleDamperOn() const override;
        void setForwardThrottleBrake(float percentage) override;
        void useForwardThrottleBrake() override;

        void setYawThrottle(float velocity) override;
        void setYawThrottleMax(float percentage) override;
        void setYawThrottleDamper(float percentage) override;
        void setYawThrottleDamperOn(bool enabled) override;
        bool isYawThrottleDamperOn() const override;
        void setYawThrottleBrake(float percentage) override;
        void useYawThrottleBrake() override;

        void setRoll(int direction) override;
        void stepRoll(int direction) override;
        void setYaw(int direction) override;
        void stepYaw(int direction) override;

        void accelerateForward(float amount = 1.0f);
        void accelerateReverse(float amount = 1.0f);
        void accelerateYawLeft(float amount = 1.0f);
        void accelerateYawRight(float amount = 1.0f);

        void setForwardBrakeStopsAtZero(bool enabled);
        void setYawBrakeStopsAtZero(bool enabled);

        void setForwardAccelerationResponse(float response_scale);
        void setYawAccelerationResponse(float response_scale);

    protected:
        float maxForwardVelocity() const;
        float maxYawVelocity() const;

    private:
        void updateForward(float dt_seconds);
        void updateYaw(float dt_seconds);
        void syncRuntimeRollDegrees();

        EntityRuntime* m_runtime = nullptr;
        u16 m_entity_id = 0;

        float m_forward_throttle_target = 0.0f;
        float m_current_forward_velocity = 0.0f;
        float m_forward_throttle_max_percentage = 1.0f;
        float m_forward_damper_percentage = 0.0f;
        bool m_forward_damper_on = true;
        float m_forward_brake_percentage = 1.0f;
        bool m_forward_brake_stops_at_zero = true;
        bool m_forward_brake_requested = false;
        float m_forward_acceleration_response = 1.0f;

        float m_yaw_throttle_target = 0.0f;
        float m_current_yaw_velocity = 0.0f;
        float m_yaw_throttle_max_percentage = 1.0f;
        float m_yaw_damper_percentage = 0.0f;
        bool m_yaw_damper_on = true;
        float m_yaw_brake_percentage = 1.0f;
        bool m_yaw_brake_stops_at_zero = true;
        bool m_yaw_brake_requested = false;
        float m_yaw_acceleration_response = 1.0f;

        int m_roll_steps = 0;
    };


    struct EntityDeleter {
        void operator()(Entity* entity) const noexcept {
            delete entity;
        }
    };

    using EntityPtr = std::unique_ptr<Entity, EntityDeleter>;

    class PlayerEntity final : public Entity {
    public:
        explicit PlayerEntity(EntityRuntime& runtime, u16 entity_id);
        ~PlayerEntity() override = default;

        std::size_t addControlPreset(const ControlPreset& preset);
        bool activateControlPreset(std::size_t index);

        const std::vector<ControlPreset>& controlPresets() const noexcept;
        std::size_t activeControlPresetIndex() const noexcept;

    private:
        std::vector<ControlPreset> m_control_presets;
        std::size_t m_active_preset_index = 0;
    };
}

#endif
