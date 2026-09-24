#pragma once

#include <DxLib.h>

#include "AnimationController.h"
#include "PlayerAttack.h"

// プレイヤーのアニメ関連責務を集約するクラス
class PlayerAnimation
{
public:
    PlayerAnimation();

    // 初期化 / モデル接続 / 終了
    void Initialize();
    bool BindModel(int modelHandle);
    void Finalize();

    // 毎フレーム更新
    void Update();

    // 待機・移動のループ切替
    void SwitchAnimation(bool useWalkAnimation, bool modelLoaded);

    // 単発アクション再生（回避など）
    void PlayActionAnimation(int animIndex, float duration);

    // 回避攻撃再生
    void PlayDodgeAttack();

    // 通常攻撃コンボ段の再生
    void PlayComboSegment(int step,
        bool isGrounded,
        const VECTOR& position,
        bool& isAirAttackLocked,
        VECTOR& airAttackLockPosition,
        int& comboStep,
        Attack& attack);

    // コンボ進行と終了処理
    void UpdateAttackAnimation(bool modelLoaded,
        int modelHandle,
        VECTOR& position,
        bool isGrounded,
        bool& isFallingAnimActive,
        bool& isAirAttackLocked,
        VECTOR& airAttackLockPosition,
        int& comboStep,
        bool& pendingCombo,
        Attack& attack);

    // 外部制御用のアニメ操作API
    float GetAnimTotalTime(int animIndex) const;
    bool PlaySegment(int animIndex, float startTime, float endTime, float duration, bool loop);
    bool IsPlaying() const;
    int GetCurrentAnimIndex() const;
    void SetCurrentTime(float time);

    // 主要アニメーション番号取得
    int GetJumpAnimIndex() const;
    int GetDodgeBackAnimIndex() const;
    int GetDodgeForwardAnimIndex() const;

private:
    // DxLibアニメ再生本体
    AnimationController animationController_;

    // 現在が移動ループかどうか
    bool useWalkAnimation_;

    // モデル内アニメ番号テーブル
    int attackAnimIndex_;
    int walkAnimIndex_;
    int idleAnimIndex_;
    int jumpAnimIndex_;
    int dodgeBackAnimIndex_;
    int dodgeForwardAnimIndex_;
    int dodgeAttackAnimIndex_;
};
