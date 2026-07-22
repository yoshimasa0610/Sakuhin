#pragma once

#include <DxLib.h>
#include "PlayerAttack.h"
#include "AnimationController.h"

class Player
{
public:
    Player();

    void Initialize();
    bool LoadModel(const TCHAR* modelPath);
    void Update();
    void Draw() const;
    void Finalize();

    VECTOR GetPosition() const;
    AttackType GetCurrentAttack() const;
    bool IsAttacking() const;

private:
    float CalculateAnimationSpeed(int targetModelHandle, int animIndex, float targetDuration) const;
    void SwitchAnimation(bool useWalkAnimation);
    void UpdateAttackAnimation();
    void PlayComboSegment(int step);
    void HandleJump();
    void HandleDodge();
    void PlayActionAnimation(int animIndex, float duration);

private:
    VECTOR position_;
    float moveSpeed_;
    float modelRotationY_;

    int modelHandle_;
    bool useWalkAnimation_;

    bool modelLoaded_;
    float idleAnimSpeed_;
    float walkAnimSpeed_;
    int totalAnimationCount_;
    int testAnimationIndex_;

    int comboStep_;
    bool pendingCombo_;
    float comboSegmentDuration_;

    // ジャンプ・回避関連
    bool isJumping_;
    bool isDodging_;
    float actionTimer_;
    float jumpHeight_;
    VECTOR dodgeDirection_;
    VECTOR savedPosition_;  // アニメーション切り替え時の位置保存用

    Attack attack_;
    AnimationController animationController_;

    int previousMouseInput_;
    int previousKeyInput_;
};
