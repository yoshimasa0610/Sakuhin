#include "PlayerAnimation.h"

namespace
{
    // ダッシュ攻撃だけ見た目の再生速度低下に合わせて少し長く保持する
    constexpr float kDashAttackSlowScale = 1.8f;

    // モデル側で固定して使っているアニメーション番号
    constexpr int kKnownAttackAnimIndex = 0;
    constexpr int kKnownDodgeAttackAnimIndex = 1;
    constexpr int kKnownMoveAnimIndex = 2;
    constexpr int kKnownIdleAnimIndex = 3;
    constexpr int kKnownPullAttackAnimIndex = 4;
    constexpr int kKnownDodgeAnimIndex = 6;
    constexpr int kKnownJumpAnimIndex = 10;
    constexpr int kKnownDashAttackAnimIndexPrimary = 12;
    constexpr int kKnownDashAttackAnimIndexSecondary = 11;
}

PlayerAnimation::PlayerAnimation()
    : useWalkAnimation_(false)
    , attackAnimIndex_(kKnownAttackAnimIndex)
    , walkAnimIndex_(kKnownMoveAnimIndex)
    , idleAnimIndex_(kKnownIdleAnimIndex)
    , jumpAnimIndex_(kKnownJumpAnimIndex)
    , dodgeBackAnimIndex_(kKnownDodgeAnimIndex)
    , dodgeForwardAnimIndex_(kKnownDodgeAnimIndex)
    , dashAttackAnimIndex_(kKnownAttackAnimIndex)
    , dodgeAttackAnimIndex_(kKnownDodgeAttackAnimIndex)
    , pullAttackAnimIndex_(kKnownPullAttackAnimIndex)
{
}

void PlayerAnimation::Initialize()
{
    // 参照アニメーション番号と再生状態を初期値へ戻す
    useWalkAnimation_ = false;
    attackAnimIndex_ = kKnownAttackAnimIndex;
    walkAnimIndex_ = kKnownMoveAnimIndex;
    idleAnimIndex_ = kKnownIdleAnimIndex;
    jumpAnimIndex_ = kKnownJumpAnimIndex;
    dodgeBackAnimIndex_ = kKnownDodgeAnimIndex;
    dodgeForwardAnimIndex_ = kKnownDodgeAnimIndex;
    dashAttackAnimIndex_ = kKnownAttackAnimIndex;
    dodgeAttackAnimIndex_ = kKnownDodgeAttackAnimIndex;
    pullAttackAnimIndex_ = kKnownPullAttackAnimIndex;
    animationController_.Initialize(-1);
}

bool PlayerAnimation::BindModel(int modelHandle)
{
    // 各アニメーション番号がモデル内に存在するか確認する
    const int animCount = MV1GetAnimNum(modelHandle);
    auto isValidIndex = [animCount](int index) { return index >= 0 && index < animCount; };

    // 必須のアニメーション番号を確認する
    if (!isValidIndex(kKnownAttackAnimIndex)
        || !isValidIndex(kKnownMoveAnimIndex)
        || !isValidIndex(kKnownIdleAnimIndex)
        || !isValidIndex(kKnownDodgeAnimIndex)
        || !isValidIndex(kKnownJumpAnimIndex)
        || !isValidIndex(kKnownDodgeAttackAnimIndex))
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
    pullAttackAnimIndex_ = isValidIndex(kKnownPullAttackAnimIndex) ? kKnownPullAttackAnimIndex : attackAnimIndex_;

    if (isValidIndex(kKnownDashAttackAnimIndexPrimary))
    {
        dashAttackAnimIndex_ = kKnownDashAttackAnimIndexPrimary;
    }
    else if (isValidIndex(kKnownDashAttackAnimIndexSecondary))
    {
        dashAttackAnimIndex_ = kKnownDashAttackAnimIndexSecondary;
    }
    else
    {
        dashAttackAnimIndex_ = attackAnimIndex_;
    }

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

    // ループ再生中は再アタッチしない
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

void PlayerAnimation::PlayDashAttack()
{
    // ダッシュ攻撃は別番号で再生する
    const float duration = 1.05f * kDashAttackSlowScale;
    animationController_.PlayOneShot(dashAttackAnimIndex_, duration);
}

void PlayerAnimation::PlayDodgeAttack()
{
    // 回避攻撃は指定のアニメーション番号1を再生する
    const float duration = 1.05f * kDashAttackSlowScale;
    animationController_.PlayOneShot(dodgeAttackAnimIndex_, duration);
}

void PlayerAnimation::PlayPullAttack()
{
    const float duration = 0.85f;
    animationController_.PlayOneShot(pullAttackAnimIndex_, duration);
}

void PlayerAnimation::PlayComboSegment(int step,
    bool isGrounded,
    const VECTOR& position,
    bool& isAirAttackLocked,
    VECTOR& airAttackLockPosition,
    int& comboStep,
    Attack& attack)
{
    // 空中コンボ中は現在位置を固定
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

    // コンボ継続予約があるなら次の段を再生する
    if (pendingCombo && comboStep < 3)
    {
        pendingCombo = false;
        PlayComboSegment(comboStep, isGrounded, position, isAirAttackLocked, airAttackLockPosition, comboStep, attack);
        return;
    }

    // コンボ終了時の状態を戻す
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
