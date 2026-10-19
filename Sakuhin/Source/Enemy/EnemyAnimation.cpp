#include "EnemyAnimation.h"

namespace
{
    // エネミーモデル内のアニメーション番号
    constexpr int kEnemyHitAnimIndex = 0;
    constexpr int kEnemyMoveAnimIndex = 1;
    constexpr int kEnemyStrongAttackAnimIndex = 2;
    constexpr int kEnemyRoarAnimIndex = 3;
    constexpr int kEnemyWeakAttackAnimIndex = 4;
    constexpr int kEnemyIdleAnimIndex = 5;

    // 初期の再生時間は少し長めにして、全体のテンポを落とす
    constexpr float kDefaultEnemyIdleDuration = 6.0f;
    constexpr float kDefaultEnemyMoveDuration = 2.4f;
    constexpr float kDefaultEnemyRoarDuration = 3.6f;
    constexpr float kDefaultEnemyWeakAttackDuration = 2.2f;
    constexpr float kDefaultEnemyStrongAttackDuration = 3.5f;
    constexpr float kDefaultEnemyHitDuration = 1.55f;
}

// 初期化
EnemyAnimation::EnemyAnimation()
    : modelHandle_(-1)
    , idleAnimIndex_(kEnemyIdleAnimIndex)
    , moveAnimIndex_(kEnemyMoveAnimIndex)
    , roarAnimIndex_(kEnemyRoarAnimIndex)
    , weakAttackAnimIndex_(kEnemyWeakAttackAnimIndex)
    , strongAttackAnimIndex_(kEnemyStrongAttackAnimIndex)
    , hitAnimIndex_(kEnemyHitAnimIndex)
    , idleDuration_(kDefaultEnemyIdleDuration)
    , moveDuration_(kDefaultEnemyMoveDuration)
    , roarDuration_(kDefaultEnemyRoarDuration)
    , weakAttackDuration_(kDefaultEnemyWeakAttackDuration)
    , strongAttackDuration_(kDefaultEnemyStrongAttackDuration)
    , hitDuration_(kDefaultEnemyHitDuration)
    , currentMotion_(Motion::Idle)
{
}

// モデルに紐づける
bool EnemyAnimation::BindModel(int modelHandle)
{
    if (modelHandle < 0)
    {
        return false;
    }

    const int animCount = MV1GetAnimNum(modelHandle);
    auto isValidIndex = [animCount](int index)
    {
        return index >= 0 && index < animCount;
    };

    // ここで固定番号を確認し、敵モデルのアニメーション構成を前提通りにそろえる
    if (!isValidIndex(kEnemyIdleAnimIndex)
        || !isValidIndex(kEnemyMoveAnimIndex)
        || !isValidIndex(kEnemyRoarAnimIndex)
        || !isValidIndex(kEnemyWeakAttackAnimIndex)
        || !isValidIndex(kEnemyStrongAttackAnimIndex)
        || !isValidIndex(kEnemyHitAnimIndex))
    {
        return false;
    }

    modelHandle_ = modelHandle;
    idleAnimIndex_ = kEnemyIdleAnimIndex;
    moveAnimIndex_ = kEnemyMoveAnimIndex;
    roarAnimIndex_ = kEnemyRoarAnimIndex;
    weakAttackAnimIndex_ = kEnemyWeakAttackAnimIndex;
    strongAttackAnimIndex_ = kEnemyStrongAttackAnimIndex;
    hitAnimIndex_ = kEnemyHitAnimIndex;
    currentMotion_ = Motion::Idle;

    animationController_.Initialize(modelHandle_);
    return true;
}

// 毎フレーム更新
void EnemyAnimation::Update()
{
    animationController_.Update();
}

// 終了処理
void EnemyAnimation::Finalize()
{
    animationController_.Finalize();
    modelHandle_ = -1;
    currentMotion_ = Motion::Idle;
}

// アイドル再生
bool EnemyAnimation::PlayIdle()
{
    return PlayLoop(Motion::Idle, idleAnimIndex_, idleDuration_);
}

// 移動再生
bool EnemyAnimation::PlayMove()
{
    return PlayLoop(Motion::Move, moveAnimIndex_, moveDuration_);
}

// 発見時の咆哮再生
bool EnemyAnimation::PlayRoar()
{
    return PlayOneShot(Motion::Roar, roarAnimIndex_, roarDuration_);
}

// 弱攻撃再生
bool EnemyAnimation::PlayWeakAttack()
{
    return PlayOneShot(Motion::WeakAttack, weakAttackAnimIndex_, weakAttackDuration_);
}

// 強攻撃再生
bool EnemyAnimation::PlayStrongAttack()
{
    return PlayOneShot(Motion::StrongAttack, strongAttackAnimIndex_, strongAttackDuration_);
}

// 被弾再生
bool EnemyAnimation::PlayHit()
{
    return PlayOneShot(Motion::Hit, hitAnimIndex_, hitDuration_);
}

// 個別の再生時間調整
void EnemyAnimation::SetIdleDuration(float duration)
{
    if (duration > 0.0f)
    {
        idleDuration_ = duration;
    }
}

void EnemyAnimation::SetMoveDuration(float duration)
{
    if (duration > 0.0f)
    {
        moveDuration_ = duration;
    }
}

void EnemyAnimation::SetRoarDuration(float duration)
{
    if (duration > 0.0f)
    {
        roarDuration_ = duration;
    }
}

void EnemyAnimation::SetWeakAttackDuration(float duration)
{
    if (duration > 0.0f)
    {
        weakAttackDuration_ = duration;
    }
}

void EnemyAnimation::SetStrongAttackDuration(float duration)
{
    if (duration > 0.0f)
    {
        strongAttackDuration_ = duration;
    }
}

void EnemyAnimation::SetHitDuration(float duration)
{
    if (duration > 0.0f)
    {
        hitDuration_ = duration;
    }
}

// まとめて再生時間を調整
void EnemyAnimation::SetAllDurations(float idleDuration, float moveDuration, float roarDuration, float weakAttackDuration, float strongAttackDuration, float hitDuration)
{
    SetIdleDuration(idleDuration);
    SetMoveDuration(moveDuration);
    SetRoarDuration(roarDuration);
    SetWeakAttackDuration(weakAttackDuration);
    SetStrongAttackDuration(strongAttackDuration);
    SetHitDuration(hitDuration);
}

// 現在モーション取得
EnemyAnimation::Motion EnemyAnimation::GetCurrentMotion() const
{
    return currentMotion_;
}

// 攻撃アニメが再生中かどうか
bool EnemyAnimation::IsActionPlaying() const
{
    return animationController_.IsPlaying();
}

// ループアニメを再生する共通処理
bool EnemyAnimation::PlayLoop(Motion motion, int animIndex, float duration)
{
    if (modelHandle_ < 0)
    {
        return false;
    }

    // すでに同じモーションなら再接続しない
    if (currentMotion_ == motion && animationController_.IsPlaying())
    {
        return true;
    }

    currentMotion_ = motion;
    return animationController_.PlayLoop(animIndex, duration);
}

// 一発再生を行う共通処理
bool EnemyAnimation::PlayOneShot(Motion motion, int animIndex, float duration)
{
    if (modelHandle_ < 0)
    {
        return false;
    }

    // 同じ一発モーションの再生中はそのまま維持する
    if (currentMotion_ == motion && animationController_.IsPlaying())
    {
        return true;
    }

    currentMotion_ = motion;
    return animationController_.PlayOneShot(animIndex, duration);
}
