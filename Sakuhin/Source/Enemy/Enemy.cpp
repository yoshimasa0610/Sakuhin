#include "Enemy.h"

#include <tchar.h>
#include <cmath>

namespace
{
    constexpr float kGroundY = 0.0f;

    // ノックバック後に床面内へ軽くクランプ
    constexpr float kFloorMinX = -3000.0f;
    constexpr float kFloorMaxX = 3000.0f;
    constexpr float kFloorMinZ = -3000.0f;
    constexpr float kFloorMaxZ = 3000.0f;

    // 被弾時のふわっとした挙動
    constexpr float kEnemyGravityScale = 0.30f;
    constexpr float kEnemyHitFloatDuration = 0.24f;
    constexpr float kEnemyHitLiftVelocity = 82.0f;
    constexpr float kEnemyHitRiseGravityScale = 0.68f;
    constexpr float kEnemyHitFallGravityScale = 0.24f;

    // エネミーAIの基本値
    constexpr float kEnemyDetectRange = 960.0f;
    constexpr float kEnemyRoarSkipDistance = 220.0f;
    constexpr float kEnemyAttackRange = 150.0f;
    constexpr float kEnemyMoveSpeed = 2.2f;
    constexpr float kEnemyAttackCooldownWeak = 0.50f;
    constexpr float kEnemyAttackCooldownStrong = 0.85f;
    constexpr float kEnemyHitStunDuration = 0.55f;
    constexpr int kEnemyStrongAttackChance = 30;

    // ここを書き換えると敵全体の見た目サイズを一括調整できる
    constexpr float kDefaultEnemyModelScale = 125.0f;
}

// 初期状態に戻す
void Enemy::Initialize()
{
    modelLoaded_ = false;
    modelHandle_ = -1;
    position_ = VGet(0.0f, 0.0f, 300.0f);
    rotationY_ = 3.14159f;
    modelScale_ = kDefaultEnemyModelScale;
    verticalVelocity_ = 0.0f;
    gravity_ = -650.0f;
    isGrounded_ = true;
    hitFloatTimer_ = 0.0f;
    hitStunTimer_ = 0.0f;
    behaviorTimer_ = 0.0f;
    attackCooldownTimer_ = 0.0f;
    encounteredPlayer_ = false;
    currentAttackIsStrong_ = false;
    behaviorState_ = BehaviorState::Idle;
    _tcscpy_s(loadedModelPath_, _T("Load Failed"));

    // 敵モデルに沿う球コライダー定義（骨名 + 半径 + 骨が無い時の予備オフセット）
    hitSpheres_ =
    {
        ColliderSphere{ _T("mixamorig_Hips"),       -1, 28.0f, VGet(  0.0f,  80.0f, 0.0f), VGet(0,0,0) },
        ColliderSphere{ _T("mixamorig_Spine"),      -1, 26.0f, VGet(  0.0f, 105.0f, 0.0f), VGet(0,0,0) },
        ColliderSphere{ _T("mixamorig_Spine2"),     -1, 26.0f, VGet(  0.0f, 130.0f, 0.0f), VGet(0,0,0) },
        ColliderSphere{ _T("mixamorig_Head"),       -1, 20.0f, VGet(  0.0f, 160.0f, 0.0f), VGet(0,0,0) },
        ColliderSphere{ _T("mixamorig_LeftUpLeg"),  -1, 18.0f, VGet(-12.0f,  55.0f, 0.0f), VGet(0,0,0) },
        ColliderSphere{ _T("mixamorig_RightUpLeg"), -1, 18.0f, VGet( 12.0f,  55.0f, 0.0f), VGet(0,0,0) },
        ColliderSphere{ _T("mixamorig_LeftArm"),    -1, 16.0f, VGet(-26.0f, 125.0f, 0.0f), VGet(0,0,0) },
        ColliderSphere{ _T("mixamorig_RightArm"),   -1, 16.0f, VGet( 26.0f, 125.0f, 0.0f), VGet(0,0,0) }
    };
}

// モデル読み込み
bool Enemy::LoadModel(const TCHAR* modelPath)
{
    Finalize();

    modelHandle_ = MV1LoadModel(modelPath);
    if (modelHandle_ < 0)
    {
        modelLoaded_ = false;
        _tcscpy_s(loadedModelPath_, _T("Load Failed"));
        return false;
    }

    // 初期変換適用
    ApplyModelScale();
    MV1SetPosition(modelHandle_, position_);
    MV1SetRotationXYZ(modelHandle_, VGet(0.0f, rotationY_, 0.0f));

    // 骨参照インデックスを取得しておく
    SetupColliderFrames();
    UpdateColliderWorldPositions();

    // エネミー用アニメーションをモデルへ接続する
    if (!enemyAnimation_.BindModel(modelHandle_))
    {
        Finalize();
        _tcscpy_s(loadedModelPath_, _T("Load Failed"));
        return false;
    }

    modelLoaded_ = true;
    _tcscpy_s(loadedModelPath_, modelPath);
    return true;
}

// サイズ調整用のスケール設定
void Enemy::SetModelScale(float scale)
{
    // 極端に小さくなりすぎないよう下限だけ保証する
    if (scale < 0.1f)
    {
        scale = 0.1f;
    }

    modelScale_ = scale;
    ApplyModelScale();
}

// 現在のスケールを返す
float Enemy::GetModelScale() const
{
    return modelScale_;
}

// 毎フレーム更新
void Enemy::Update(const VECTOR& playerPosition)
{
    if (!modelLoaded_ || modelHandle_ < 0)
    {
        return;
    }

    const float deltaTime = 1.0f / 60.0f;

    // 被弾ヒットストップとクールダウンを少しずつ減らす
    if (hitStunTimer_ > 0.0f)
    {
        hitStunTimer_ -= deltaTime;
        if (hitStunTimer_ < 0.0f)
        {
            hitStunTimer_ = 0.0f;
        }
    }
    if (attackCooldownTimer_ > 0.0f)
    {
        attackCooldownTimer_ -= deltaTime;
        if (attackCooldownTimer_ < 0.0f)
        {
            attackCooldownTimer_ = 0.0f;
        }
    }

    // 被弾中のふわっとした上昇・落下
    if (hitFloatTimer_ > 0.0f)
    {
        hitFloatTimer_ -= deltaTime;
        if (hitFloatTimer_ < 0.0f)
        {
            hitFloatTimer_ = 0.0f;
        }
    }

    if (!isGrounded_ || position_.y > kGroundY)
    {
        float gravityScale = (verticalVelocity_ < 0.0f) ? kEnemyGravityScale : 1.0f;

        if (hitFloatTimer_ > 0.0f)
        {
            gravityScale = (verticalVelocity_ < 0.0f)
                ? kEnemyHitFallGravityScale
                : kEnemyHitRiseGravityScale;
        }

        verticalVelocity_ += gravity_ * gravityScale * deltaTime;
        position_.y += verticalVelocity_ * deltaTime;

        if (position_.y <= kGroundY)
        {
            position_.y = kGroundY;
            verticalVelocity_ = 0.0f;
            isGrounded_ = true;
        }
    }
    else
    {
        isGrounded_ = true;
    }

    // 行動とアニメーションを切り替える
    UpdateBehavior(playerPosition, deltaTime);

    // モデルへ反映
    ApplyModelScale();
    MV1SetPosition(modelHandle_, position_);
    MV1SetRotationXYZ(modelHandle_, VGet(0.0f, rotationY_, 0.0f));

    // モデル姿勢に追従して当たり判定中心を更新
    UpdateColliderWorldPositions();

    // アニメーション再生を進める
    enemyAnimation_.Update();
}

// 描画
void Enemy::Draw() const
{
    if (!modelLoaded_ || modelHandle_ < 0)
    {
        return;
    }

    MV1DrawModel(modelHandle_);

    // 当たり判定を可視化（黄色ワイヤー球）
    const int lineColor = GetColor(255, 220, 40);
    const int fillColor = GetColor(255, 220, 40);
    for (const auto& sphere : hitSpheres_)
    {
        DrawSphere3D(sphere.worldCenter, sphere.radius, 12, lineColor, fillColor, FALSE);
    }
}

// モデル解放
void Enemy::Finalize()
{
    enemyAnimation_.Finalize();

    if (modelHandle_ >= 0)
    {
        MV1DeleteModel(modelHandle_);
        modelHandle_ = -1;
    }

    modelLoaded_ = false;
}

// 攻撃球とのヒット判定
bool Enemy::CheckAttackHit(const VECTOR& attackCenter, float attackRadius, const VECTOR& attackerPos, float knockbackDistance)
{
    if (!modelLoaded_ || modelHandle_ < 0)
    {
        return false;
    }

    // どれか1球に当たればヒット
    for (const auto& sphere : hitSpheres_)
    {
        const VECTOR diff = VSub(sphere.worldCenter, attackCenter);
        const float distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
        const float hitRadius = sphere.radius + attackRadius;
        if (distSq > hitRadius * hitRadius)
        {
            continue;
        }

        // 微弱ノックバック（水平のみ）
        VECTOR pushDir = VSub(sphere.worldCenter, attackerPos);
        pushDir.y = 0.0f;
        const float dirLen = std::sqrt(pushDir.x * pushDir.x + pushDir.z * pushDir.z);
        if (dirLen > 0.0001f)
        {
            pushDir.x /= dirLen;
            pushDir.z /= dirLen;
        }
        else
        {
            pushDir = VGet(0.0f, 0.0f, 1.0f);
        }

        position_.x += pushDir.x * knockbackDistance;
        position_.z += pushDir.z * knockbackDistance;

        // 被弾中は少し浮き上がり、攻撃できない状態にする
        if (position_.y < kGroundY + 1.0f)
        {
            position_.y = kGroundY + 6.0f;
        }
        verticalVelocity_ = kEnemyHitLiftVelocity;
        hitFloatTimer_ = kEnemyHitFloatDuration;
        hitStunTimer_ = kEnemyHitStunDuration;
        isGrounded_ = false;
        behaviorState_ = BehaviorState::HitStun;
        attackCooldownTimer_ = 0.0f;
        encounteredPlayer_ = true;
        currentAttackIsStrong_ = false;

        // 被弾アニメーションへ切り替え
        enemyAnimation_.PlayHit();

        // ステージ範囲内へクランプ
        if (position_.x < kFloorMinX) position_.x = kFloorMinX;
        if (position_.x > kFloorMaxX) position_.x = kFloorMaxX;
        if (position_.z < kFloorMinZ) position_.z = kFloorMinZ;
        if (position_.z > kFloorMaxZ) position_.z = kFloorMaxZ;

        MV1SetPosition(modelHandle_, position_);
        UpdateColliderWorldPositions();
        return true;
    }

    return false;
}

// 引っ張り攻撃のヒット反応
void Enemy::ApplyPullAttackHit(const VECTOR& attackerPos, float knockbackDistance)
{
    if (!modelLoaded_ || modelHandle_ < 0)
    {
        return;
    }

    // 引っ張り攻撃でもひるみ扱いにする
    VECTOR pushDir = VSub(position_, attackerPos);
    pushDir.y = 0.0f;
    const float dirLen = std::sqrt(pushDir.x * pushDir.x + pushDir.z * pushDir.z);
    if (dirLen > 0.0001f)
    {
        pushDir.x /= dirLen;
        pushDir.z /= dirLen;
    }
    else
    {
        pushDir = VGet(0.0f, 0.0f, 1.0f);
    }

    position_.x += pushDir.x * knockbackDistance;
    position_.z += pushDir.z * knockbackDistance;

    // ひるみ中は通常行動を止める
    if (position_.y < kGroundY + 1.0f)
    {
        position_.y = kGroundY + 6.0f;
    }
    verticalVelocity_ = kEnemyHitLiftVelocity;
    hitFloatTimer_ = kEnemyHitFloatDuration;
    hitStunTimer_ = kEnemyHitStunDuration;
    isGrounded_ = false;
    behaviorState_ = BehaviorState::HitStun;
    attackCooldownTimer_ = 0.0f;
    encounteredPlayer_ = true;
    currentAttackIsStrong_ = false;

    // ひるみモーションを再生する
    enemyAnimation_.PlayHit();

    // 範囲外へ飛びすぎないようにする
    if (position_.x < kFloorMinX) position_.x = kFloorMinX;
    if (position_.x > kFloorMaxX) position_.x = kFloorMaxX;
    if (position_.z < kFloorMinZ) position_.z = kFloorMinZ;
    if (position_.z > kFloorMaxZ) position_.z = kFloorMaxZ;

    MV1SetPosition(modelHandle_, position_);
    UpdateColliderWorldPositions();
}

void Enemy::LaunchToHeight(float targetY)
{
    if (!modelLoaded_ || modelHandle_ < 0)
    {
        return;
    }

    if (targetY < kGroundY)
    {
        targetY = kGroundY;
    }

    position_.y = targetY;
    verticalVelocity_ = 0.0f;
    hitFloatTimer_ = kEnemyHitFloatDuration;
    isGrounded_ = (position_.y <= kGroundY);
    MV1SetPosition(modelHandle_, position_);
    UpdateColliderWorldPositions();
}

void Enemy::MoveToPosition(const VECTOR& targetPosition)
{
    if (!modelLoaded_ || modelHandle_ < 0)
    {
        return;
    }

    position_ = targetPosition;

    if (position_.x < kFloorMinX) position_.x = kFloorMinX;
    if (position_.x > kFloorMaxX) position_.x = kFloorMaxX;
    if (position_.z < kFloorMinZ) position_.z = kFloorMinZ;
    if (position_.z > kFloorMaxZ) position_.z = kFloorMaxZ;

    if (position_.y <= kGroundY)
    {
        position_.y = kGroundY;
        verticalVelocity_ = 0.0f;
        isGrounded_ = true;
    }
    else
    {
        isGrounded_ = false;
    }

    MV1SetPosition(modelHandle_, position_);
    UpdateColliderWorldPositions();
}

// 外部参照
bool Enemy::IsModelLoaded() const
{
    return modelLoaded_;
}

const TCHAR* Enemy::GetLoadedModelPath() const
{
    return loadedModelPath_;
}

VECTOR Enemy::GetPosition() const
{
    return position_;
}

// 骨名からフレームインデックスを取得
void Enemy::SetupColliderFrames()
{
    if (modelHandle_ < 0)
    {
        return;
    }

    for (auto& sphere : hitSpheres_)
    {
        sphere.frameIndex = MV1SearchFrame(modelHandle_, sphere.frameName);
    }
}

// コライダー中心を骨位置に追従
void Enemy::UpdateColliderWorldPositions()
{
    for (auto& sphere : hitSpheres_)
    {
        if (sphere.frameIndex >= 0)
        {
            sphere.worldCenter = MV1GetFramePosition(modelHandle_, sphere.frameIndex);
        }
        else
        {
            // 骨が無い場合は本体位置 + 予備オフセット
            sphere.worldCenter = VAdd(position_, sphere.localFallbackOffset);
        }
    }
}

// モデルスケールを適用する
void Enemy::ApplyModelScale()
{
    if (modelHandle_ < 0)
    {
        return;
    }

    MV1SetScale(modelHandle_, VGet(modelScale_, modelScale_, modelScale_));
}

// プレイヤーとの距離を横方向だけで計算する
float Enemy::GetHorizontalDistance(const VECTOR& targetPosition) const
{
    const float diffX = targetPosition.x - position_.x;
    const float diffZ = targetPosition.z - position_.z;
    return std::sqrt(diffX * diffX + diffZ * diffZ);
}

// 対象へ向きを合わせる
void Enemy::FaceToTarget(const VECTOR& targetPosition)
{
    const float diffX = targetPosition.x - position_.x;
    const float diffZ = targetPosition.z - position_.z;
    rotationY_ = std::atan2(diffX, diffZ) + 3.14159f;
}

// 咆哮を開始する
void Enemy::StartRoar()
{
    behaviorState_ = BehaviorState::Roaring;
    behaviorTimer_ = 0.0f;
    enemyAnimation_.PlayRoar();
}

// 攻撃を開始する
void Enemy::StartAttack()
{
    behaviorState_ = BehaviorState::Attacking;
    behaviorTimer_ = 0.0f;
    currentAttackIsStrong_ = (GetRand(100) < kEnemyStrongAttackChance);

    if (currentAttackIsStrong_)
    {
        enemyAnimation_.PlayStrongAttack();
    }
    else
    {
        enemyAnimation_.PlayWeakAttack();
    }
}

// 被弾のひるみを開始する
void Enemy::StartHitStun()
{
    behaviorState_ = BehaviorState::HitStun;
    behaviorTimer_ = 0.0f;
    enemyAnimation_.PlayHit();
}

// 行動とアニメーションの更新
void Enemy::UpdateBehavior(const VECTOR& playerPosition, float deltaTime)
{
    const float distanceToPlayer = GetHorizontalDistance(playerPosition);

    // 被弾中は完全に行動を止める
    if (hitStunTimer_ > 0.0f)
    {
        StartHitStun();
        FaceToTarget(playerPosition);
        return;
    }

    // 画面外相当の距離なら待機に戻す
    if (distanceToPlayer > kEnemyDetectRange)
    {
        encounteredPlayer_ = false;
        behaviorState_ = BehaviorState::Idle;
        currentAttackIsStrong_ = false;
        enemyAnimation_.PlayIdle();
        return;
    }

    // 発見したら咆哮を挟む。かなり近いならスキップしてそのまま接近する
    if (!encounteredPlayer_)
    {
        encounteredPlayer_ = true;
        if (distanceToPlayer > kEnemyRoarSkipDistance)
        {
            StartRoar();
            FaceToTarget(playerPosition);
            return;
        }
    }

    // 咆哮中は終わるまで待つ
    if (behaviorState_ == BehaviorState::Roaring)
    {
        FaceToTarget(playerPosition);
        if (!enemyAnimation_.IsActionPlaying())
        {
            behaviorState_ = BehaviorState::Chasing;
        }
        return;
    }

    // 攻撃中はアニメが終わるまで待つ
    if (behaviorState_ == BehaviorState::Attacking)
    {
        FaceToTarget(playerPosition);
        if (!enemyAnimation_.IsActionPlaying())
        {
            behaviorState_ = BehaviorState::Chasing;
            attackCooldownTimer_ = currentAttackIsStrong_ ? kEnemyAttackCooldownStrong : kEnemyAttackCooldownWeak;
            currentAttackIsStrong_ = false;
        }
        return;
    }

    // クールダウンが終わるまでは追跡だけ行う
    FaceToTarget(playerPosition);

    if (distanceToPlayer > kEnemyAttackRange)
    {
        // 追跡中は移動アニメーションを再生する
        behaviorState_ = BehaviorState::Chasing;
        const float diffX = playerPosition.x - position_.x;
        const float diffZ = playerPosition.z - position_.z;
        const float length = std::sqrt(diffX * diffX + diffZ * diffZ);
        if (length > 0.0001f)
        {
            position_.x += (diffX / length) * kEnemyMoveSpeed;
            position_.z += (diffZ / length) * kEnemyMoveSpeed;
        }
        enemyAnimation_.PlayMove();
        return;
    }

    if (attackCooldownTimer_ > 0.0f)
    {
        behaviorState_ = BehaviorState::Chasing;
        enemyAnimation_.PlayMove();
        return;
    }

    // 攻撃範囲に入ったらランダムで弱攻撃か強攻撃を選ぶ
    StartAttack();
}