#include "PlayerAttack.h"
#include <DxLib.h>

Attack::Attack()
    : currentAttack_(AttackType::None)
    , attackTimer_(0.0f)
    , attackDuration_(0.0f)
{
}

void Attack::Initialize()
{
    currentAttack_ = AttackType::None;
    attackTimer_ = 0.0f;
    attackDuration_ = 0.0f;
}

void Attack::Update()
{
    if (currentAttack_ != AttackType::None)
    {
        attackTimer_ += 1.0f / 60.0f;

        if (attackTimer_ >= attackDuration_)
        {
            currentAttack_ = AttackType::None;
            attackTimer_ = 0.0f;
        }
    }
}

void Attack::Draw() const
{
    // デバッグ表示
    DrawFormatString(10, 10, GetColor(255, 255, 255), _T("Press Q for Weak Attack"));
    
    const TCHAR* attackState = _T("None");
    if (currentAttack_ == AttackType::WeakAttack)
    {
        attackState = _T("WeakAttack");
    }
    else if (currentAttack_ == AttackType::StrongAttack)
    {
        attackState = _T("StrongAttack");
    }
    
    DrawFormatString(10, 30, GetColor(255, 255, 0), _T("Attack State: %s"), attackState);
    DrawFormatString(10, 50, GetColor(255, 255, 0), _T("Attack Timer: %.2f / %.2f"), attackTimer_, attackDuration_);
    DrawFormatString(10, 70, GetColor(0, 255, 0), _T("IsAttacking: %s"), 
                     IsAttacking() ? _T("TRUE") : _T("FALSE"));
}

void Attack::Finalize()
{
}

void Attack::ExecuteWeakAttack()
{
    currentAttack_ = AttackType::WeakAttack;
    attackTimer_ = 0.0f;
    attackDuration_ = 0.5f;
}

void Attack::ExecuteStrongAttack()
{
    currentAttack_ = AttackType::StrongAttack;
    attackTimer_ = 0.0f;
    attackDuration_ = 1.0f;
}

void Attack::CancelAttack()
{
    currentAttack_ = AttackType::None;
    attackTimer_ = 0.0f;
}

AttackType Attack::GetCurrentAttack() const
{
    return currentAttack_;
}

bool Attack::IsAttacking() const
{
    return currentAttack_ != AttackType::None;
}

float Attack::GetAttackDuration() const
{
    return attackTimer_;
}
