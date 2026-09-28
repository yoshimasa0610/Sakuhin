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

    constexpr float kEnemyGravityScale = 0.30f;
    constexpr float kEnemyHitFloatDuration = 0.24f;
    constexpr float kEnemyHitLiftVelocity = 82.0f;
    constexpr float kEnemyHitRiseGravityScale = 0.68f;
    constexpr float kEnemyHitFallGravityScale = 0.24f;
}

// 初期状態に戻す
void Enemy::Initialize()
{
    modelLoaded_ = false;
    modelHandle_ = -1;
    position_ = VGet(0.0f, 0.0f, 300.0f);
    rotationY_ = 3.14159f;
    verticalVelocity_ = 0.0f;
    gravity_ = -650.0f;
    isGrounded_ = true;
    hitFloatTimer_ = 0.0f;
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
    MV1SetScale(modelHandle_, VGet(1.0f, 1.0f, 1.0f));
    MV1SetPosition(modelHandle_, position_);
    MV1SetRotationXYZ(modelHandle_, VGet(0.0f, rotationY_, 0.0f));

    // 骨参照インデックスを取得しておく
    SetupColliderFrames();
    UpdateColliderWorldPositions();

    modelLoaded_ = true;
    _tcscpy_s(loadedModelPath_, modelPath);
    return true;
}

// 毎フレーム更新
void Enemy::Update()
{
    if (!modelLoaded_ || modelHandle_ < 0)
    {
        return;
    }

    const float deltaTime = 1.0f / 60.0f;

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

    MV1SetPosition(modelHandle_, position_);
    MV1SetRotationXYZ(modelHandle_, VGet(0.0f, rotationY_, 0.0f));

    // モデル姿勢に追従して当たり判定中心を更新
    UpdateColliderWorldPositions();
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

        // 被弾中は少し浮き上がり、ふわっと滞空する
        if (position_.y < kGroundY + 1.0f)
        {
            position_.y = kGroundY + 6.0f;
        }
        verticalVelocity_ = kEnemyHitLiftVelocity;
        hitFloatTimer_ = kEnemyHitFloatDuration;
        isGrounded_ = false;

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