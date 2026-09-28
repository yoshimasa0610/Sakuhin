#pragma once

#include <DxLib.h>

#include "PlayerAttack.h"
#include "PlayerAnimation.h"

// プレイヤー本体クラス
// 入力・移動・重力・ジャンプ・回避・攻撃アニメ連携を管理する
class Player
{
public:
    Player();

    // 初期化
    void Initialize();

    // モデル読み込み
    bool LoadModel(const TCHAR* modelPath);

    // フレーム更新（cameraYaw はカメラの水平角）
    void Update(float cameraYaw);

    // 描画
    void Draw() const;

    // 終了処理
    void Finalize();

    // 現在位置の取得
    VECTOR GetPosition() const;

    // 見た目の向き（水平前方向ベクトル）を取得
    VECTOR GetFacingDirection() const;

    // 現在攻撃状態の取得
    AttackType GetCurrentAttack() const;

    // 現在のコンボ段（0:なし / 1..3:各段）
    int GetComboStep() const;

    // 攻撃判定を出してよいタイミングか
    bool IsAttackHitboxActive() const;

    // エリアル始動攻撃（長押し始動1段目）中か
    bool IsAerialStarterAttackActive() const;

    // 敵打ち上げ後、指定高度へジャンプ追従させる
    void StartAerialFollowJump(float targetY);

    // 攻撃経過時間（コンボ段の切り替わり検出用）
    float GetAttackElapsedTime() const;

    // 攻撃中かどうか
    bool IsAttacking() const;

private:
    // 基本座標・移動速度・見た目向き
    VECTOR position_;
    float moveSpeed_;
    float modelRotationY_;

    // 3Dモデルハンドルと読み込み状態
    int modelHandle_;
    bool modelLoaded_;

    // 通常攻撃コンボ状態
    int comboStep_;
    bool pendingCombo_;

    // ジャンプ / 回避 / 落下アニメ / 物理
    bool isJumping_;
    bool isDodging_;
    bool isFallingAnimActive_;
    float actionTimer_;
    float jumpHeight_;
    float verticalVelocity_;
    float gravity_;
    float jumpStartVelocity_;
    bool isGrounded_;

    // 空中攻撃時の位置固定
    bool isAirAttackLocked_;
    VECTOR airAttackLockPosition_;

    // 回避終了後の回避攻撃受付（猶予）
    bool canDodgeAttack_;
    float dodgeAttackGraceTimer_;

    // 攻撃後隙管理と回避攻撃後隙の切替フラグ
    float attackRecoveryTimer_;
    bool pendingDodgeAttackRecovery_;

    // 回避中に入れた攻撃の先行入力バッファ
    bool queuedDodgeAttack_;

    // 左クリック長押しエリアル始動管理
    float attackHoldTimer_;
    bool wasAttackHeld_;
    bool longPressAttackTriggered_;
    bool isAerialStarterAttack_;

    // 敵打ち上げ追従ジャンプ管理
    bool isFollowingAerialTarget_;
    float aerialTargetY_;

    // ヒット判定タイミング管理（コンボ段ごとの経過時間）
    float comboStepElapsedTime_;
    int prevComboStepForHitbox_;

    // 空中攻撃時のふわふわ上昇トリガー用
    int prevComboStepForAirFloat_;

    // 攻撃状態管理とアニメ管理
    Attack attack_;
    PlayerAnimation playerAnimation_;

    // 前フレーム入力（押下瞬間判定用）
    int previousMouseInput_;
    int previousKeyInput_;
};
