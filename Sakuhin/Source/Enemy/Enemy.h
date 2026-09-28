#pragma once

#include <DxLib.h>
#include <array>

// 敵1体のモデル・当たり判定・被弾挙動を管理するクラス
class Enemy
{
public:
    // 状態初期化
    void Initialize();

    // モデル読み込み
    bool LoadModel(const TCHAR* modelPath);

    // 毎フレーム更新（位置・回転・コライダー追従）
    void Update();

    // 描画（モデル + 当たり判定可視化）
    void Draw() const;

    // 終了処理（モデル解放）
    void Finalize();

    // 攻撃球との当たり判定
    // attackCenter: 攻撃中心, attackRadius: 攻撃半径, attackerPos: 攻撃者位置, knockback: 押し出し距離
    bool CheckAttackHit(const VECTOR& attackCenter, float attackRadius, const VECTOR& attackerPos, float knockbackDistance);

    // エリアル始動用に敵を空中へ持ち上げる
    void LaunchToHeight(float targetY);

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

    void SetupColliderFrames();
    void UpdateColliderWorldPositions();

private:
    int modelHandle_ = -1;
    bool modelLoaded_ = false;

    // 配置情報（マップ中央付近）
    VECTOR position_ = VGet(0.0f, 0.0f, 300.0f);
    float rotationY_ = 3.14159f;

    // 重力挙動
    float verticalVelocity_ = 0.0f;
    float gravity_ = -650.0f;
    bool isGrounded_ = true;

    // 被弾中ふわふわ
    float hitFloatTimer_ = 0.0f;

    // モデルに追従させる当たり判定（骨ベース）
    std::array<ColliderSphere, 8> hitSpheres_;

    // 読み込み済みパス
    TCHAR loadedModelPath_[MAX_PATH] = _T("");
};