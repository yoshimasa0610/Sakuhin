#include "PlayerAttack.h"

// コンストラクタ
Attack::Attack()
    : currentAttack_(AttackType::None)
    , attackTimer_(0.0f)
    , attackDuration_(0.0f)
{
}

// 初期化
void Attack::Initialize()
{
    currentAttack_ = AttackType::None;
    attackTimer_ = 0.0f;
    attackDuration_ = 0.0f;
}

// 攻撃タイマー更新
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

// 現在は描画なし
void Attack::Draw() const
{
}

// 終了処理
void Attack::Finalize()
{
}

// 弱攻撃開始
void Attack::ExecuteWeakAttack()
{
    currentAttack_ = AttackType::WeakAttack;
    attackTimer_ = 0.0f;
    attackDuration_ = 0.5f;
}

// 強攻撃開始
void Attack::ExecuteStrongAttack()
{
    currentAttack_ = AttackType::StrongAttack;
    attackTimer_ = 0.0f;
    attackDuration_ = 1.0f;
}

// 攻撃キャンセル
void Attack::CancelAttack()
{
    currentAttack_ = AttackType::None;
    attackTimer_ = 0.0f;
}

// 現在攻撃種別
AttackType Attack::GetCurrentAttack() const
{
    return currentAttack_;
}

// 攻撃中判定
bool Attack::IsAttacking() const
{
    return currentAttack_ != AttackType::None;
}

// 経過時間取得
float Attack::GetAttackDuration() const
{
    return attackTimer_;
}
