#pragma once

enum class AttackType
{
    WeakAttack,
    StrongAttack,
    None
};

class Attack
{
public:
    Attack();

    void Initialize();
    void Update();
    void Draw() const;
    void Finalize();

    void ExecuteWeakAttack();
    void ExecuteStrongAttack();
    void CancelAttack();

    AttackType GetCurrentAttack() const;
    bool IsAttacking() const;
    float GetAttackDuration() const;

private:
    AttackType currentAttack_;
    float attackTimer_;
    float attackDuration_;
};
