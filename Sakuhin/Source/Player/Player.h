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

private:
    VECTOR position_;
    float moveSpeed_;

    int modelHandle_;
    bool useWalkAnimation_;

    bool modelLoaded_;
    float idleAnimSpeed_;
    float walkAnimSpeed_;

    Attack attack_;
    AnimationController animationController_;
};
