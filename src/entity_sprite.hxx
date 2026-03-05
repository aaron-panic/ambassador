#ifndef ENTITY_SPRITE_HXX_INCLUDED
#define ENTITY_SPRITE_HXX_INCLUDED

#include "amb_types.hxx"

#include <array>
#include <cstddef>

namespace amb::entity {
    enum class SpriteState : u8 {
        idle = 0,
        thrust_min,
        thrust_mid,
        thrust_max,
        brake,
        yaw_right,
        yaw_left,
        purge,
    };

    constexpr std::size_t SPRITE_STATE_COUNT = 8;

    struct SpriteStateDescriptor {
        u16 base_frame = 0;
        u16 roll_frame_count = 0;
        u16 anim_frame_count = 1;
    };

    using SpriteStateTable = std::array<SpriteStateDescriptor, SPRITE_STATE_COUNT>;

    inline u16 resolveSpriteFrame(
        const SpriteStateDescriptor& descriptor,
        int roll_offset,
        u16 anim_frame
    ) {
        const u16 clamped_roll = static_cast<u16>(
            (roll_offset < 0) ? 0 :
            (roll_offset >= descriptor.roll_frame_count) ? (descriptor.roll_frame_count - 1) :
            roll_offset);

        const u16 clamped_anim = (descriptor.anim_frame_count > 0)
            ? static_cast<u16>(anim_frame % descriptor.anim_frame_count)
            : 0;

        return static_cast<u16>(
            descriptor.base_frame
            + (clamped_anim * descriptor.roll_frame_count)
            + clamped_roll);
    }
}

#endif
