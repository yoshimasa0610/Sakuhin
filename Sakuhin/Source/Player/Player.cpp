#include "Player.h"

#include <cmath>

namespace
{
    constexpr float kModelScale = 240.0f;
    constexpr float kModelRotationY = 3.14159f;
    constexpr int kWalkAnimIndex = 0;
    constexpr int kIdleAnimIndex = 1;
}

Player::Player()
    : position_(VGet(0.0f, 0.0f, 0.0f))
    , moveSpeed_(4.0f)
    , modelHandle_(-1)
    , useWalkAnimation_(false)
    , modelLoaded_(false)
    , idleAnimSpeed_(1.0f)
    , walkAnimSpeed_(1.0f)
{
}

void Player::Initialize()
{
    position_ = VGet(0.0f, 0.0f, 0.0f);
    attack_.Initialize();
    animationController_.Initialize(-1);
    useWalkAnimation_ = false;
}

bool Player::LoadModel(const TCHAR* modelPath)
{
    Finalize();

    modelHandle_ = MV1LoadModel(modelPath);
    if (modelHandle_ < 0)
    {
        modelLoaded_ = false;
        return false;
    }

    MV1SetScale(modelHandle_, VGet(kModelScale, kModelScale, kModelScale));
    MV1SetPosition(modelHandle_, position_);
    MV1SetRotationXYZ(modelHandle_, VGet(0.0f, kModelRotationY, 0.0f));

    walkAnimSpeed_ = CalculateAnimationSpeed(modelHandle_, kWalkAnimIndex, 1.5f);
    idleAnimSpeed_ = CalculateAnimationSpeed(modelHandle_, kIdleAnimIndex, 4.0f);

    modelLoaded_ = true;
    SwitchAnimation(false);

    return true;
}

void Player::Update()
{
    VECTOR move = VGet(0.0f, 0.0f, 0.0f);

    if (CheckHitKey(KEY_INPUT_W)) move.z += 1.0f;
    if (CheckHitKey(KEY_INPUT_S)) move.z -= 1.0f;
    if (CheckHitKey(KEY_INPUT_A)) move.x -= 1.0f;
    if (CheckHitKey(KEY_INPUT_D)) move.x += 1.0f;

    const float length = std::sqrt(move.x * move.x + move.z * move.z);
    const bool isMoving = (length > 0.0f);

    if (isMoving)
    {
        move.x /= length;
        move.z /= length;

        position_.x += move.x * moveSpeed_;
        position_.z += move.z * moveSpeed_;
    }

    SwitchAnimation(isMoving);

    if (GetMouseInput() & MOUSE_INPUT_LEFT)
    {
        attack_.ExecuteWeakAttack();
    }

    if (GetMouseInput() & MOUSE_INPUT_RIGHT)
    {
        attack_.ExecuteStrongAttack();
    }

    attack_.Update();
    animationController_.Update();

    if (modelHandle_ >= 0)
    {
        MV1SetPosition(modelHandle_, position_);
    }
}

void Player::Draw() const
{
    if (modelLoaded_ && modelHandle_ >= 0)
    {
        MV1DrawModel(modelHandle_);
    }
    else
    {
        DrawSphere3D(position_, 20.0f, 16, GetColor(220, 80, 80), GetColor(255, 255, 255), TRUE);
    }

    attack_.Draw();
}

void Player::Finalize()
{
    animationController_.Finalize();

    if (modelHandle_ >= 0)
    {
        MV1DeleteModel(modelHandle_);
        modelHandle_ = -1;
    }

    modelLoaded_ = false;
    useWalkAnimation_ = false;
    attack_.Finalize();
}

VECTOR Player::GetPosition() const
{
    return position_;
}

AttackType Player::GetCurrentAttack() const
{
    return attack_.GetCurrentAttack();
}

bool Player::IsAttacking() const
{
    return attack_.IsAttacking();
}

float Player::CalculateAnimationSpeed(int targetModelHandle, int animIndex, float targetDuration) const
{
    if (targetModelHandle < 0 || animIndex < 0 || targetDuration <= 0.0f)
    {
        return 1.0f;
    }

    const int animNum = MV1GetAnimNum(targetModelHandle);
    if (animIndex >= animNum)
    {
        return 1.0f;
    }

    const float totalTime = MV1GetAnimTotalTime(targetModelHandle, animIndex);
    if (totalTime <= 0.0f)
    {
        return 1.0f;
    }

    return totalTime / targetDuration;
}

void Player::SwitchAnimation(bool useWalkAnimation)
{
    if (!modelLoaded_)
    {
        return;
    }

    if (useWalkAnimation_ == useWalkAnimation && animationController_.IsPlaying())
    {
        return;
    }

    useWalkAnimation_ = useWalkAnimation;

    const int animIndex = useWalkAnimation_ ? kWalkAnimIndex : kIdleAnimIndex;
    const float animSpeed = useWalkAnimation_ ? walkAnimSpeed_ : idleAnimSpeed_;

    animationController_.StopAnimation();
    animationController_.Initialize(modelHandle_);
    animationController_.PlayAnimation(animIndex, animSpeed, true);
}