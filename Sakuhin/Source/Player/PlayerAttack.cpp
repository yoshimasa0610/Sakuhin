#include "PlayerAttack.h"

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
