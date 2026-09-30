#include "PlayerAnimation.h"

namespace
{
    // ダッシュ攻撃だけ再生を少しゆっくりにする
    constexpr float kDashAttackSlowScale = 1.8f;

    // モデル側で固定運用しているアニメ番号
    constexpr int kKnownAttackAnimIndex = 0;
    // ユーザー指定のダッシュ攻撃候補番号
    constexpr int kKnownDashAttackAnimIndexPrimary = 12;
    // DCCツールや一覧表示によっては12番目=添字11のことがあるため予備候補も持つ
    constexpr int kKnownDashAttackAnimIndexSecondary = 11;
    // 旧回避攻撃番号は最終フォールバックとして残しておく
    constexpr int kKnownDodgeAttackAnimIndexFallback = 1;
    constexpr int kKnownMoveAnimIndex = 2;
    constexpr int kKnownIdleAnimIndex = 3;
    constexpr int kKnownDodgeAnimIndex = 6;
    constexpr int kKnownJumpAnimIndex = 10;
    constexpr int kKnownPullAttackAnimIndex = 4;
}

PlayerAnimation::PlayerAnimation()
    : useWalkAnimation_(false)
    , attackAnimIndex_(kKnownAttackAnimIndex)
    , walkAnimIndex_(kKnownMoveAnimIndex)
    , idleAnimIndex_(kKnownIdleAnimIndex)
    , jumpAnimIndex_(kKnownJumpAnimIndex)
    , dodgeBackAnimIndex_(kKnownDodgeAnimIndex)
    , dodgeForwardAnimIndex_(kKnownDodgeAnimIndex)
    , dodgeAttackAnimIndex_(kKnownDodgeAttackAnimIndexFallback)
    , pullAttackAnimIndex_(kKnownPullAttackAnimIndex)
{
}

void PlayerAnimation::Initialize()
{
    // 参照アニメ番号と再生状態を初期値へ戻す
    useWalkAnimation_ = false;
    attackAnimIndex_ = kKnownAttackAnimIndex;
    walkAnimIndex_ = kKnownMoveAnimIndex;
    idleAnimIndex_ = kKnownIdleAnimIndex;
    jumpAnimIndex_ = kKnownJumpAnimIndex;
    dodgeBackAnimIndex_ = kKnownDodgeAnimIndex;
    dodgeForwardAnimIndex_ = kKnownDodgeAnimIndex;
    dodgeAttackAnimIndex_ = kKnownDodgeAttackAnimIndexFallback;
    pullAttackAnimIndex_ = kKnownPullAttackAnimIndex;
    animationController_.Initialize(-1);
}

bool PlayerAnimation::BindModel(int modelHandle)
{
    // 主要アニメ番号がモデル内に存在するか確認
    const int animCount = MV1GetAnimNum(modelHandle);
    auto isValidIndex = [animCount](int index) { return index >= 0 && index < animCount; };

    // モデル表示に必須の基本アニメだけをロード必須条件にする
    if (!isValidIndex(kKnownAttackAnimIndex)
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

    // ダッシュ攻撃は 12 → 11 → 旧1番 の順で使えるものを採用する
    if (isValidIndex(kKnownDashAttackAnimIndexPrimary))
    {
        dodgeAttackAnimIndex_ = kKnownDashAttackAnimIndexPrimary;
    }
    else if (isValidIndex(kKnownDashAttackAnimIndexSecondary))
    {
        dodgeAttackAnimIndex_ = kKnownDashAttackAnimIndexSecondary;
    }
    else
    {
        dodgeAttackAnimIndex_ = isValidIndex(kKnownDodgeAttackAnimIndexFallback)
            ? kKnownDodgeAttackAnimIndexFallback
            : attackAnimIndex_;
    }

    pullAttackAnimIndex_ = isValidIndex(kKnownPullAttackAnimIndex) ? kKnownPullAttackAnimIndex : attackAnimIndex_;

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
    // ダッシュ攻撃だけは少しゆっくり最後まで再生する
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
