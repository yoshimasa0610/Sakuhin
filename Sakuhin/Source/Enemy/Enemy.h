#pragma once

#include <DxLib.h>
#include <array>

#include "EnemyAnimation.h"

// 敵1体のモデル・当たり判定・行動・被弾挙動を管理するクラス
class Enemy
{
public:
    // 状態初期化
    void Initialize();

    // モデル読み込み
    bool LoadModel(const TCHAR* modelPath);

    // サイズ調整用のスケール設定
    void SetModelScale(float scale);
    float GetModelScale() const;

    // 毎フレーム更新（プレイヤー位置を見て行動とアニメを切り替える）
    void Update(const VECTOR& playerPosition);

    // 描画（モデル + 当たり判定可視化）
    void Draw() const;

    // 終了処理（モデル解放）
    void Finalize();

    // 攻撃球との当たり判定
    // attackCenter: 攻撃中心, attackRadius: 攻撃半径, attackerPos: 攻撃者位置, knockback: 押し出し距離
    bool CheckAttackHit(const VECTOR& attackCenter, float attackRadius, const VECTOR& attackerPos, float knockbackDistance);

    // 引っ張り攻撃を受けた時のひるみ反応
    void ApplyPullAttackHit(const VECTOR& attackerPos, float knockbackDistance);

    // エリアル始動用に敵を空中へ持ち上げる
    void LaunchToHeight(float targetY);

    // 指定位置へ移動させる（引き寄せ用）
    void MoveToPosition(const VECTOR& targetPosition);

    // 外部参照
    bool IsModelLoaded() const;
    const TCHAR* GetLoadedModelPath() const;
    VECTOR GetPosition() const;

private:
    // 部位ごとの球コライダー
    struct ColliderSphere
    {
        const TCHAR* frameName = _T("");
        int frameIndex = -1;
        float radius = 20.0f;
        VECTOR localFallbackOffset = VGet(0.0f, 0.0f, 0.0f);
        VECTOR worldCenter = VGet(0.0f, 0.0f, 0.0f);
    };

    // 行動パターンを分けておくと、敵追加時に流用しやすい
    enum class BehaviorState
    {
        Idle,
        Roaring,
        Chasing,
        Attacking,
        HitStun
    };

    void SetupColliderFrames();
    void UpdateColliderWorldPositions();
    void UpdateBehavior(const VECTOR& playerPosition, float deltaTime);
    void FaceToTarget(const VECTOR& targetPosition);
    float GetHorizontalDistance(const VECTOR& targetPosition) const;
    void StartRoar();
    void StartAttack();
    void StartHitStun();
    void ApplyModelScale();

private:
    int modelHandle_ = -1;
    bool modelLoaded_ = false;

    // アニメーション制御をまとめる
    EnemyAnimation enemyAnimation_;

    // 行動状態
    BehaviorState behaviorState_ = BehaviorState::Idle;
    float behaviorTimer_ = 0.0f;
    float attackCooldownTimer_ = 0.0f;
    bool encounteredPlayer_ = false;
    bool currentAttackIsStrong_ = false;

    // 配置情報（マップ中央付近）
    VECTOR position_ = VGet(0.0f, 0.0f, 300.0f);
    float rotationY_ = 3.14159f;

    // モデルの拡大率
    float modelScale_ = 1.0f;

    // 重力挙動
    float verticalVelocity_ = 0.0f;
    float gravity_ = -650.0f;
    bool isGrounded_ = true;

    // 被弾中ふわふわ
    float hitFloatTimer_ = 0.0f;
    float hitStunTimer_ = 0.0f;

    // モデルに追従させる当たり判定（骨ベース）
    std::array<ColliderSphere, 8> hitSpheres_;

    // 読み込み済みパス
    TCHAR loadedModelPath_[MAX_PATH] = _T("");
};