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
    int testAnimationIndex_;  // テスト用：どのアニメーションを再生するか

    int comboStep_;
    bool pendingCombo_;
    float comboSegmentDuration_;

    Attack attack_;
    AnimationController animationController_;
    
    int previousMouseInput_;
};
