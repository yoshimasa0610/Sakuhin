#include "PlayerAnimation.h"

namespace
{
    // モデル側で固定利用しているアニメ番号
    constexpr int kKnownAttackAnimIndex = 0;
    constexpr int kKnownDodgeAttackAnimIndex = 1;
    constexpr int kKnownMoveAnimIndex = 2;
    constexpr int kKnownIdleAnimIndex = 3;
    constexpr int kKnownDodgeAnimIndex = 6;
    constexpr int kKnownJumpAnimIndex = 10;
}

PlayerAnimation::PlayerAnimation()
    : useWalkAnimation_(false)
    , attackAnimIndex_(kKnownAttackAnimIndex)
    , walkAnimIndex_(kKnownMoveAnimIndex)
    , idleAnimIndex_(kKnownIdleAnimIndex)
    , jumpAnimIndex_(kKnownJumpAnimIndex)
    , dodgeBackAnimIndex_(kKnownDodgeAnimIndex)
    , dodgeForwardAnimIndex_(kKnownDodgeAnimIndex)
    , dodgeAttackAnimIndex_(kKnownDodgeAttackAnimIndex)
{
}

void PlayerAnimation::Initialize()
{
    // 参照アニメ番号と再生状態を既定値へ戻す
    useWalkAnimation_ = false;
    attackAnimIndex_ = kKnownAttackAnimIndex;
    walkAnimIndex_ = kKnownMoveAnimIndex;
    idleAnimIndex_ = kKnownIdleAnimIndex;
    jumpAnimIndex_ = kKnownJumpAnimIndex;
    dodgeBackAnimIndex_ = kKnownDodgeAnimIndex;
    dodgeForwardAnimIndex_ = kKnownDodgeAnimIndex;
    dodgeAttackAnimIndex_ = kKnownDodgeAttackAnimIndex;
    animationController_.Initialize(-1);
}

bool PlayerAnimation::BindModel(int modelHandle)
{
    // 期待する固定アニメ番号がモデルに存在するか検証
    const int animCount = MV1GetAnimNum(modelHandle);
    auto isValidIndex = [animCount](int index) { return index >= 0 && index < animCount; };

    if (!isValidIndex(kKnownAttackAnimIndex)
        || !isValidIndex(kKnownDodgeAttackAnimIndex)
        || !isValidIndex(kKnownMoveAnimIndex)
        || !isValidIndex(kKnownIdleAnimIndex)
        || !isValidIndex(kKnownDodgeAnimIndex)
        || !isValidIndex(kKnownJumpAnimIndex))
    {
        return false;
    }

    attackAnimIndex_ = kKnownAttackAnimIndex;
    walkAnimIndex_ = kKnownMoveAnimIndex;
    idleAnimIndex_ = kKnownIdleAnimIndex;
    jumpAnimIndex_ = kKnownJumpAnimIndex;
    dodgeBackAnimIndex_ = kKnownDodgeAnimIndex;
    dodgeForwardAnimIndex_ = kKnownDodgeAnimIndex;
    dodgeAttackAnimIndex_ = kKnownDodgeAttackAnimIndex;

    animationController_.Initialize(modelHandle);
    return true;
}

void PlayerAnimation::Finalize()
{
    animationController_.Finalize();
}

void PlayerAnimation::Update()
{
    animationController_.Update();
}

void PlayerAnimation::SwitchAnimation(bool useWalkAnimation, bool modelLoaded)
{
    if (!modelLoaded)
    {
        return;
    }

    // 同一ループ再生中は再アタッチしない
    if (useWalkAnimation_ == useWalkAnimation && animationController_.IsPlaying())
    {
        return;
    }

    useWalkAnimation_ = useWalkAnimation;

    const int animIndex = useWalkAnimation_ ? walkAnimIndex_ : idleAnimIndex_;
    const float targetDuration = useWalkAnimation_ ? 1.5f : 4.0f;
    animationController_.PlayLoop(animIndex, targetDuration);
}

void PlayerAnimation::PlayActionAnimation(int animIndex, float duration)
{
    animationController_.PlayOneShot(animIndex, duration);
}

void PlayerAnimation::PlayDodgeAttack()
{
    // 回避攻撃はモデルのアニメ番号1を単発再生
    const float duration = 1.05f;
    animationController_.PlayOneShot(dodgeAttackAnimIndex_, duration);
}

void PlayerAnimation::PlayComboSegment(int step,
    bool isGrounded,
    const VECTOR& position,
    bool& isAirAttackLocked,
    VECTOR& airAttackLockPosition,
    int& comboStep,
    Attack& attack)
{
    // 空中コンボ時は段中の位置を固定
    if (!isGrounded)
    {
        isAirAttackLocked = true;
        airAttackLockPosition = position;
    }
    else
    {
        isAirAttackLocked = false;
    }

    float start = 0.0f;
    float end = 0.0f;
    float targetDuration = 0.5f;

    if (step == 0)
    {
        start = 0.0f;
        end = 35.0f;
        targetDuration = 0.5f;
    }
    else if (step == 1)
    {
        start = 35.0f;
        end = 45.0f;
        targetDuration = 0.25f;
    }
    else if (step == 2)
    {
        start = 45.0f;
        end = 105.0f;
        targetDuration = 0.7f;
    }

    animationController_.PlaySegment(attackAnimIndex_, start, end, targetDuration, false);
    comboStep = step + 1;
    attack.ExecuteWeakAttack();
}

void PlayerAnimation::UpdateAttackAnimation(bool modelLoaded,
    int modelHandle,
    VECTOR& position,
    bool isGrounded,
    bool& isFallingAnimActive,
    bool& isAirAttackLocked,
    VECTOR& airAttackLockPosition,
    int& comboStep,
    bool& pendingCombo,
    Attack& attack)
{
    if (!modelLoaded || comboStep == 0)
    {
        return;
    }

    if (animationController_.IsPlaying())
    {
        return;
    }

    // 現段終了時に予約があれば次段へ
    if (pendingCombo && comboStep < 3)
    {
        pendingCombo = false;
        PlayComboSegment(comboStep, isGrounded, position, isAirAttackLocked, airAttackLockPosition, comboStep, attack);
        return;
    }

    // コンボ完了時の状態復帰
    if (modelHandle >= 0)
    {
        position = MV1GetPosition(modelHandle);
    }

    comboStep = 0;
    pendingCombo = false;
    attack.CancelAttack();
    isAirAttackLocked = false;
    isFallingAnimActive = false;

    if (isGrounded)
    {
        SwitchAnimation(false, modelLoaded);
    }
}

float PlayerAnimation::GetAnimTotalTime(int animIndex) const
{
    return animationController_.GetAnimTotalTime(animIndex);
}

bool PlayerAnimation::PlaySegment(int animIndex, float startTime, float endTime, float duration, bool loop)
{
    return animationController_.PlaySegment(animIndex, startTime, endTime, duration, loop);
}

bool PlayerAnimation::IsPlaying() const
{
    return animationController_.IsPlaying();
}

int PlayerAnimation::GetCurrentAnimIndex() const
{
    return animationController_.GetCurrentAnimIndex();
}

void PlayerAnimation::SetCurrentTime(float time)
{
    animationController_.SetCurrentTime(time);
}

int PlayerAnimation::GetJumpAnimIndex() const
{
    return jumpAnimIndex_;
}

int PlayerAnimation::GetDodgeBackAnimIndex() const
{
    return dodgeBackAnimIndex_;
}

int PlayerAnimation::GetDodgeForwardAnimIndex() const
{
    return dodgeForwardAnimIndex_;
}
