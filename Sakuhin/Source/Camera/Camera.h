#pragma once

#include <DxLib.h>
#include <cmath>

class Camera
{
public:
    void Initialize(const VECTOR& playerPosition, const VECTOR& playerFacingDirection)
    {
        forward_ = NormalizeOrDefault(playerFacingDirection, VGet(0.0f, 0.0f, 1.0f));

        const VECTOR desiredPosition = VGet(
            playerPosition.x - forward_.x * kFollowDistance,
            playerPosition.y + kHeightOffset,
            playerPosition.z - forward_.z * kFollowDistance);

        position_ = desiredPosition;
        target_ = VGet(playerPosition.x, playerPosition.y + kTargetHeightOffset, playerPosition.z);
    }

    void Update(const VECTOR& playerPosition, const VECTOR& playerFacingDirection)
    {
        const VECTOR desiredForward = NormalizeOrDefault(playerFacingDirection, forward_);
        forward_ = NormalizeOrDefault(
            VGet(
                forward_.x + (desiredForward.x - forward_.x) * kForwardLerpRate,
                0.0f,
                forward_.z + (desiredForward.z - forward_.z) * kForwardLerpRate),
            forward_);

        const VECTOR desiredPosition = VGet(
            playerPosition.x - forward_.x * kFollowDistance,
            playerPosition.y + kHeightOffset,
            playerPosition.z - forward_.z * kFollowDistance);

        position_ = VGet(
            position_.x + (desiredPosition.x - position_.x) * kPositionLerpRate,
            position_.y + (desiredPosition.y - position_.y) * kPositionLerpRate,
            position_.z + (desiredPosition.z - position_.z) * kPositionLerpRate);

        const VECTOR desiredTarget = VGet(
            playerPosition.x,
            playerPosition.y + kTargetHeightOffset,
            playerPosition.z);

        target_ = VGet(
            target_.x + (desiredTarget.x - target_.x) * kTargetLerpRate,
            target_.y + (desiredTarget.y - target_.y) * kTargetLerpRate,
            target_.z + (desiredTarget.z - target_.z) * kTargetLerpRate);
    }

    void Apply() const
    {
        SetCameraPositionAndTarget_UpVecY(position_, target_);
    }

private:
    static VECTOR NormalizeOrDefault(const VECTOR& v, const VECTOR& fallback)
    {
        const float length = std::sqrt(v.x * v.x + v.z * v.z);
        if (length <= 0.0001f)
        {
            return fallback;
        }

        return VGet(v.x / length, 0.0f, v.z / length);
    }

private:
    static constexpr float kFollowDistance = 560.0f;
    static constexpr float kHeightOffset = 360.0f;
    static constexpr float kTargetHeightOffset = 180.0f;
    static constexpr float kForwardLerpRate = 0.20f;
    static constexpr float kPositionLerpRate = 0.30f;
    static constexpr float kTargetLerpRate = 0.40f;

    VECTOR position_ = VGet(0.0f, 0.0f, 0.0f);
    VECTOR target_ = VGet(0.0f, 0.0f, 0.0f);
    VECTOR forward_ = VGet(0.0f, 0.0f, 1.0f);
};