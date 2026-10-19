#pragma once

#include <DxLib.h>
#include "Enemy.h"
#include "../Player/PlayerAttack.h"

// 敵管理クラス（現在は1体の敵を管理）
class EnemyManager
{
public:
    // 初期化（モデル読み込み）
    void Initialize();

    // 更新（敵の行動更新 + プレイヤー攻撃との当たり判定）
    void Update(const VECTOR& playerPosition,
        const VECTOR& playerFacingDirection,
        AttackType playerAttackType,
        int playerComboStep,
        bool isPlayerAttackHitboxActive,
        bool isPlayerAttacking,
        bool isPlayerAerialStarterAttackActive,
        float playerAttackElapsedTime);

    // 描画（エネミー本体 + 攻撃判定の可視化）
    void Draw() const;

    // 終了処理
    void Finalize();

    // エリアル始動ヒット後の追従ジャンプ要求を1回だけ取得
    bool ConsumeAerialFollowJumpRequest(float& outTargetY);

    // ロックオン対象として使えるかどうか
    bool HasLockOnTarget() const;

    // ロックオン用の注視位置を取得する
    VECTOR GetLockOnTargetPosition() const;

private:
    Enemy enemy_;

    // ヒット中の連続ヒットを防ぐためのクールタイム
    float hitCooldownTimer_ = 0.0f;

    // 1回の攻撃中に複数ヒットしないための状態
    bool wasPlayerAttackingPrev_ = false;
    bool hitRegisteredThisAttack_ = false;
    int prevComboStep_ = 0;

    // 攻撃判定の可視化用（現在は前方と剣の2つ）
    bool showAttackHitbox_ = false;
    VECTOR swordHitCenter_ = VGet(0.0f, 0.0f, 0.0f);
    float swordHitRadius_ = 0.0f;
    VECTOR frontHitCenter_ = VGet(0.0f, 0.0f, 0.0f);
    float frontHitRadius_ = 0.0f;

    bool hasAerialFollowJumpRequest_ = false;
    float aerialFollowJumpTargetY_ = 0.0f;
};