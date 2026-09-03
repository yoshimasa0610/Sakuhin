#pragma once

#include <DxLib.h>
#include "PlayerAttack.h"
#include "AnimationController.h"

// プレイヤー本体クラス
// 入力・移動・重力・ジャンプ・回避・攻撃アニメ連携を管理する
class Player
{
public:
    Player();

    // 初期化
    void Initialize();

    // モデル読み込みと使用アニメーション解決
    bool LoadModel(const TCHAR* modelPath);

    // フレーム更新（cameraYaw はカメラの水平角）
    void Update(float cameraYaw);

    // 描画
    void Draw() const;

    // 終了処理
    void Finalize();

    // 現在位置の取得
    VECTOR GetPosition() const;

    // 現在攻撃状態の取得
    AttackType GetCurrentAttack() const;

    // 攻撃中かどうか
    bool IsAttacking() const;

private:
    // 指定アニメを targetDuration 秒で再生するための速度を計算
    float CalculateAnimationSpeed(int targetModelHandle, int animIndex, float targetDuration) const;

    // 待機 / 移動のループアニメ切り替え
    void SwitchAnimation(bool useWalkAnimation);

    // コンボ攻撃アニメ進行の更新
    void UpdateAttackAnimation();

    // コンボ段ごとの再生区間を設定して再生
    void PlayComboSegment(int step);

    // 単発アクションアニメを指定時間で再生
    void PlayActionAnimation(int animIndex, float duration);

private:
    // 基本移動情報
    VECTOR position_;
    float moveSpeed_;
    float modelRotationY_;

    // モデルと状態
    int modelHandle_;
    bool useWalkAnimation_;

    // 読み込み済みモデル情報
    bool modelLoaded_;
    float idleAnimSpeed_;
    float walkAnimSpeed_;
    int totalAnimationCount_;

    // 使用アニメーション番号
    int attackAnimIndex_;
    int walkAnimIndex_;
    int idleAnimIndex_;
    int jumpAnimIndex_;
    int dodgeBackAnimIndex_;
    int dodgeForwardAnimIndex_;

    // コンボ状態
    int comboStep_;
    bool pendingCombo_;

    // 空中 / 回避 / 物理状態
    bool isJumping_;
    bool isDodging_;
    bool isFallingAnimActive_;
    float actionTimer_;
    float jumpHeight_;
    float verticalVelocity_;
    float gravity_;
    float jumpStartVelocity_;
    bool isGrounded_;

    // サブシステム
    Attack attack_;
    AnimationController animationController_;

    // 入力の前フレーム値（押下瞬間判定用）
    int previousMouseInput_;
    int previousKeyInput_;
};
