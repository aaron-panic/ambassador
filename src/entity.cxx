#include "entity.hxx"

#include <algorithm>
#include <cmath>

namespace {
    constexpr float BASE_FORWARD_VELOCITY = 600.0f;
    constexpr float BASE_FORWARD_ACCELERATION = 550.0f;
    constexpr float BASE_FORWARD_BRAKE_ACCELERATION = 900.0f;

    constexpr float BASE_YAW_VELOCITY = 220.0f;
    constexpr float BASE_YAW_ACCELERATION = 280.0f;
    constexpr float BASE_YAW_BRAKE_ACCELERATION = 420.0f;

    constexpr float PI_VALUE = 3.14159265358979323846f;

    float clampPercent(float value) {
        return std::clamp(value, 0.0f, 1.0f);
    }

    int normalizeDirection(int value) {
        if (value > 0) {
            return 1;
        }

        if (value < 0) {
            return -1;
        }

        return 0;
    }

    float normalizeDegrees(float value) {
        float normalized = std::fmod(value, 360.0f);
        if (normalized < 0.0f) {
            normalized += 360.0f;
        }

        return normalized;
    }

    float approach(float current, float target, float step) {
        if (current < target) {
            return std::min(current + step, target);
        }

        if (current > target) {
            return std::max(current - step, target);
        }

        return current;
    }

    float brakeToward(float current, float step, bool stop_at_zero) {
        if (current > 0.0f) {
            const float next = current - step;
            if (stop_at_zero) {
                return std::max(next, 0.0f);
            }
            return next;
        }

        if (current < 0.0f) {
            const float next = current + step;
            if (stop_at_zero) {
                return std::min(next, 0.0f);
            }
            return next;
        }

        if (stop_at_zero) {
            return 0.0f;
        }

        return -step;
    }
}

namespace amb::entity {
    Entity::Entity(EntityRuntime& runtime, const u16 entity_id)
    : m_runtime(&runtime),
      m_entity_id(entity_id) {
        syncRuntimeRollDegrees();
    }

    u16 Entity::id() const noexcept {
        return m_entity_id;
    }

    EntityRuntime& Entity::runtime() noexcept {
        return *m_runtime;
    }

    const EntityRuntime& Entity::runtime() const noexcept {
        return *m_runtime;
    }

    void Entity::integrate(u64 dt_ms) {
        if (m_runtime == nullptr || dt_ms == 0) {
            return;
        }

        const float dt_seconds = static_cast<float>(dt_ms) / 1000.0f;

        updateForward(dt_seconds);
        updateYaw(dt_seconds);

        m_runtime->heading_degrees = normalizeDegrees(
            m_runtime->heading_degrees + (m_current_yaw_velocity * dt_seconds));

        const float heading_radians = m_runtime->heading_degrees * (PI_VALUE / 180.0f);
        const float unit_x = std::sin(heading_radians);
        const float unit_y = -std::cos(heading_radians);

        m_runtime->world_x += unit_x * m_current_forward_velocity * dt_seconds;
        m_runtime->world_y += unit_y * m_current_forward_velocity * dt_seconds;
        m_runtime->speed = m_current_forward_velocity;

        m_forward_brake_requested = false;
        m_yaw_brake_requested = false;
    }

    void Entity::setForwardThrottle(float velocity) {
        m_forward_throttle_target = std::clamp(velocity, -maxForwardVelocity(), maxForwardVelocity());
    }

    void Entity::setForwardThrottleMax(float percentage) {
        m_forward_throttle_max_percentage = clampPercent(percentage);
        m_forward_throttle_target = std::clamp(m_forward_throttle_target, -maxForwardVelocity(), maxForwardVelocity());
        m_current_forward_velocity = std::clamp(m_current_forward_velocity, -maxForwardVelocity(), maxForwardVelocity());
    }

    void Entity::setForwardThrottleDamper(float percentage) {
        m_forward_damper_percentage = clampPercent(percentage);
    }

    void Entity::setForwardThrottleDamperOn(bool enabled) {
        m_forward_damper_on = enabled;
    }

    bool Entity::isForwardThrottleDamperOn() const {
        return m_forward_damper_on;
    }

    void Entity::setForwardThrottleBrake(float percentage) {
        m_forward_brake_percentage = clampPercent(percentage);
    }

    void Entity::useForwardThrottleBrake() {
        m_forward_brake_requested = true;
    }

    void Entity::setYawThrottle(float velocity) {
        m_yaw_throttle_target = std::clamp(velocity, -maxYawVelocity(), maxYawVelocity());
    }

    void Entity::setYawThrottleMax(float percentage) {
        m_yaw_throttle_max_percentage = clampPercent(percentage);
        m_yaw_throttle_target = std::clamp(m_yaw_throttle_target, -maxYawVelocity(), maxYawVelocity());
        m_current_yaw_velocity = std::clamp(m_current_yaw_velocity, -maxYawVelocity(), maxYawVelocity());
    }

    void Entity::setYawThrottleDamper(float percentage) {
        m_yaw_damper_percentage = clampPercent(percentage);
    }

    void Entity::setYawThrottleDamperOn(bool enabled) {
        m_yaw_damper_on = enabled;
    }

    bool Entity::isYawThrottleDamperOn() const {
        return m_yaw_damper_on;
    }

    void Entity::setYawThrottleBrake(float percentage) {
        m_yaw_brake_percentage = clampPercent(percentage);
    }

    void Entity::useYawThrottleBrake() {
        m_yaw_brake_requested = true;
    }

    void Entity::setRoll(int direction) {
        m_roll_steps = std::clamp(direction, -ROLL_MAX_STEPS, ROLL_MAX_STEPS);
        syncRuntimeRollDegrees();
    }

    void Entity::stepRoll(int direction) {
        const int roll_direction = normalizeDirection(direction);
        if (roll_direction == 0) {
            return;
        }

        m_roll_steps = std::clamp(
            m_roll_steps + roll_direction,
            -ROLL_MAX_STEPS,
            ROLL_MAX_STEPS);
        syncRuntimeRollDegrees();
    }

    void Entity::setYaw(int direction) {
        const int yaw_direction = normalizeDirection(direction);
        setYawThrottle(static_cast<float>(yaw_direction) * maxYawVelocity());
    }

    void Entity::stepYaw(int direction) {
        const int yaw_direction = normalizeDirection(direction);
        const float step_size = maxYawVelocity() / static_cast<float>(ROLL_MAX_STEPS);
        setYawThrottle(m_yaw_throttle_target + (step_size * static_cast<float>(yaw_direction)));
    }

    void Entity::accelerateForward(float amount) {
        const float step = BASE_FORWARD_ACCELERATION * std::max(0.0f, amount) * m_forward_acceleration_response * 0.016f;
        setForwardThrottle(m_forward_throttle_target + step);
    }

    void Entity::accelerateReverse(float amount) {
        const float step = BASE_FORWARD_ACCELERATION * std::max(0.0f, amount) * m_forward_acceleration_response * 0.016f;
        setForwardThrottle(m_forward_throttle_target - step);
    }

    void Entity::accelerateYawLeft(float amount) {
        const float step = BASE_YAW_ACCELERATION * std::max(0.0f, amount) * m_yaw_acceleration_response * 0.016f;
        setYawThrottle(m_yaw_throttle_target - step);
    }

    void Entity::accelerateYawRight(float amount) {
        const float step = BASE_YAW_ACCELERATION * std::max(0.0f, amount) * m_yaw_acceleration_response * 0.016f;
        setYawThrottle(m_yaw_throttle_target + step);
    }

    void Entity::setForwardBrakeStopsAtZero(bool enabled) {
        m_forward_brake_stops_at_zero = enabled;
    }

    void Entity::setYawBrakeStopsAtZero(bool enabled) {
        m_yaw_brake_stops_at_zero = enabled;
    }

    void Entity::setForwardAccelerationResponse(float response_scale) {
        m_forward_acceleration_response = std::max(0.0f, response_scale);
    }

    void Entity::setYawAccelerationResponse(float response_scale) {
        m_yaw_acceleration_response = std::max(0.0f, response_scale);
    }

    float Entity::maxForwardVelocity() const {
        return BASE_FORWARD_VELOCITY * m_forward_throttle_max_percentage;
    }

    float Entity::maxYawVelocity() const {
        return BASE_YAW_VELOCITY * m_yaw_throttle_max_percentage;
    }


    void Entity::syncRuntimeRollDegrees() {
        if (m_runtime == nullptr) {
            return;
        }

        m_runtime->roll_degrees = static_cast<float>(m_roll_steps * ROLL_STEP_DEGREES);
    }

    void Entity::updateForward(float dt_seconds) {
        const float max_forward_velocity = maxForwardVelocity();
        const float acceleration = BASE_FORWARD_ACCELERATION * m_forward_acceleration_response * dt_seconds;

        if (m_forward_brake_requested) {
            const float brake_step = BASE_FORWARD_BRAKE_ACCELERATION * m_forward_brake_percentage * dt_seconds;
            m_current_forward_velocity = brakeToward(
                m_current_forward_velocity,
                brake_step,
                m_forward_brake_stops_at_zero);
        } else if (m_forward_damper_on && std::abs(m_forward_throttle_target) < 0.001f) {
            const float damper_step = BASE_FORWARD_BRAKE_ACCELERATION * m_forward_damper_percentage * dt_seconds;
            m_current_forward_velocity = approach(m_current_forward_velocity, 0.0f, damper_step);
        } else {
            m_current_forward_velocity = approach(
                m_current_forward_velocity,
                m_forward_throttle_target,
                acceleration);
        }

        m_current_forward_velocity = std::clamp(m_current_forward_velocity, -max_forward_velocity, max_forward_velocity);
    }

    void Entity::updateYaw(float dt_seconds) {
        const float max_yaw_velocity = maxYawVelocity();
        const float acceleration = BASE_YAW_ACCELERATION * m_yaw_acceleration_response * dt_seconds;

        if (m_yaw_brake_requested) {
            const float brake_step = BASE_YAW_BRAKE_ACCELERATION * m_yaw_brake_percentage * dt_seconds;
            m_current_yaw_velocity = brakeToward(
                m_current_yaw_velocity,
                brake_step,
                m_yaw_brake_stops_at_zero);
        } else if (m_yaw_damper_on && std::abs(m_yaw_throttle_target) < 0.001f) {
            const float damper_step = BASE_YAW_BRAKE_ACCELERATION * m_yaw_damper_percentage * dt_seconds;
            m_current_yaw_velocity = approach(m_current_yaw_velocity, 0.0f, damper_step);
        } else {
            m_current_yaw_velocity = approach(
                m_current_yaw_velocity,
                m_yaw_throttle_target,
                acceleration);
        }

        m_current_yaw_velocity = std::clamp(m_current_yaw_velocity, -max_yaw_velocity, max_yaw_velocity);
    }

    PlayerEntity::PlayerEntity(EntityRuntime& runtime, const u16 entity_id)
    : Entity(runtime, entity_id) {}

    std::size_t PlayerEntity::addControlPreset(const ControlPreset& preset) {
        m_control_presets.push_back(preset);
        return m_control_presets.size() - 1;
    }

    bool PlayerEntity::activateControlPreset(std::size_t index) {
        if (index >= m_control_presets.size()) {
            return false;
        }

        m_active_preset_index = index;
        const ControlPreset& preset = m_control_presets[index];

        setForwardThrottleMax(preset.forward_throttle_max_percentage);
        setForwardThrottleDamper(preset.forward_damper_percentage);
        setForwardThrottleDamperOn(preset.forward_damper_on);
        setForwardThrottleBrake(preset.forward_brake_percentage);
        setForwardBrakeStopsAtZero(preset.forward_brake_stops_at_zero);

        setYawThrottleMax(preset.yaw_throttle_max_percentage);
        setYawThrottleDamper(preset.yaw_damper_percentage);
        setYawThrottleDamperOn(preset.yaw_damper_on);
        setYawThrottleBrake(preset.yaw_brake_percentage);
        setYawBrakeStopsAtZero(preset.yaw_brake_stops_at_zero);

        setForwardAccelerationResponse(preset.forward_acceleration_response);
        setYawAccelerationResponse(preset.yaw_acceleration_response);

        return true;
    }

    const std::vector<ControlPreset>& PlayerEntity::controlPresets() const noexcept {
        return m_control_presets;
    }

    std::size_t PlayerEntity::activeControlPresetIndex() const noexcept {
        return m_active_preset_index;
    }
}
