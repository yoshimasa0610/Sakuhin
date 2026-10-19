#include "EnemyManager.h"

#include <Windows.h>
#include <io.h>
#include <tchar.h>
#include <cmath>

namespace
{
    // ファイル存在確認
    bool FileExists(const TCHAR* path)
    {
        return _taccess(path, 0) == 0;
    }

    // ヒット判定アニメーション
    constexpr float kHitCooldown = 0.12f;
    constexpr float kAerialLaunchTargetY = 220.0f;
    constexpr float kPullAttackHitStart = 0.45f;
    constexpr float kPullAttackHitEnd = 0.5f;
    // 引っ張り攻撃の有効射程を長めにする
    constexpr float kPullAttackMinRange = 240.0f;
    constexpr float kPullAttackMaxRange = 1200.0f;
    constexpr float kPullAttackFrontDistance = 120.0f;
    constexpr float kPullAttackFlinchKnockback = 18.0f;

    // 回避攻撃のヒット判定時間
    constexpr float kDodgeAttackHitStart = 0.18f;
    constexpr float kDodgeAttackHitEnd = 1.66f;
}

// Enemy.xの読み込み
void EnemyManager::Initialize()
{
    enemy_.Initialize();
    hitCooldownTimer_ = 0.0f;
    wasPlayerAttackingPrev_ = false;
    hitRegisteredThisAttack_ = false;
    prevComboStep_ = 0;
    showAttackHitbox_ = false;
    hasAerialFollowJumpRequest_ = false;
    aerialFollowJumpTargetY_ = 0.0f;

    // まず相対パスを順番に試す
    const TCHAR* relativeCandidates[] =
    {
        _T("Source/Images/Enemy/Enemy.x"),
        _T("../Source/Images/Enemy/Enemy.x"),
        _T("../../Source/Images/Enemy/Enemy.x"),
        _T("Source/Images/Enemy.x"),
        _T("../Source/Images/Enemy.x"),
        _T("../../Source/Images/Enemy.x")
    };

    for (const auto& path : relativeCandidates)
    {
        if (!FileExists(path))
        {
            continue;
        }

        if (enemy_.LoadModel(path))
        {
            return;
        }
    }

    // 実行ファイル位置から絶対パスで再試行
    TCHAR exePath[MAX_PATH] = { 0 };
    GetModuleFileName(NULL, exePath, MAX_PATH);

    TCHAR* lastSlash = _tcsrchr(exePath, _T('\\'));
    if (lastSlash != nullptr)
    {
        *lastSlash = _T('\0');
    }

    TCHAR absolutePath[MAX_PATH] = { 0 };
    _stprintf_s(absolutePath, _T("%s\\..\\..\\Source\\Images\\Enemy\\Enemy.x"), exePath);

    if (FileExists(absolutePath))
    {
        enemy_.LoadModel(absolutePath);
    }
}

// 更新
void EnemyManager::Update(const VECTOR& playerPosition,
    const VECTOR& playerFacingDirection,
    AttackType playerAttackType,
    int playerComboStep,
    bool isPlayerAttackHitboxActive,
    bool isPlayerAttacking,
    bool isPlayerAerialStarterAttackActive,
    float playerAttackElapsedTime)
{
    const float deltaTime = 1.0f / 60.0f;
    hasAerialFollowJumpRequest_ = false;

    if (hitCooldownTimer_ > 0.0f)
    {
        hitCooldownTimer_ -= deltaTime;
        if (hitCooldownTimer_ < 0.0f)
        {
            hitCooldownTimer_ = 0.0f;
        }
    }

    // 攻撃開始フレームでヒット済み状態をリセット
    if (isPlayerAttacking && !wasPlayerAttackingPrev_)
    {
        hitRegisteredThisAttack_ = false;
        prevComboStep_ = playerComboStep;
    }

    // コンボ段が変わったら次のヒットを受け付ける
    if (isPlayerAttacking && wasPlayerAttackingPrev_ && playerComboStep != prevComboStep_)
    {
        hitRegisteredThisAttack_ = false;
        prevComboStep_ = playerComboStep;
    }

    // 攻撃終了時に状態を戻す
    if (!isPlayerAttacking && wasPlayerAttackingPrev_)
    {
        hitRegisteredThisAttack_ = false;
        prevComboStep_ = 0;
    }

    wasPlayerAttackingPrev_ = isPlayerAttacking;

    // エネミー本体の行動とアニメーション更新
    enemy_.Update(playerPosition);

    const bool isPullAttackActive =
        playerAttackType == AttackType::PullAttack
        && playerAttackElapsedTime >= kPullAttackHitStart
        && playerAttackElapsedTime <= kPullAttackHitEnd;

    // プレイヤーのヒット判定が有効なタイミングだけ可視化する
    showAttackHitbox_ = isPlayerAttackHitboxActive;

    // 1回の攻撃で既にヒットしているなら、そのまま次フレームへ
    if (hitRegisteredThisAttack_)
    {
        return;
    }

    // 攻撃向きの基準ベクトルを作る
    VECTOR attackDir = VGet(playerFacingDirection.x, 0.0f, playerFacingDirection.z);
    const float dirLen = std::sqrt(attackDir.x * attackDir.x + attackDir.z * attackDir.z);
    if (dirLen > 0.0001f)
    {
        attackDir.x /= dirLen;
        attackDir.z /= dirLen;
    }
    else
    {
        // 取得できない場合は前方固定
        attackDir = VGet(0.0f, 0.0f, 1.0f);
    }

    if (isPullAttackActive)
    {
        const VECTOR enemyPosition = enemy_.GetPosition();
        const VECTOR toEnemy = VSub(enemyPosition, playerPosition);
        const float distance = std::sqrt(toEnemy.x * toEnemy.x + toEnemy.z * toEnemy.z);

        if (distance >= kPullAttackMinRange && distance <= kPullAttackMaxRange)
        {
            VECTOR targetPosition = enemyPosition;
            targetPosition.x = playerPosition.x + attackDir.x * kPullAttackFrontDistance;
            targetPosition.z = playerPosition.z + attackDir.z * kPullAttackFrontDistance;
            enemy_.MoveToPosition(targetPosition);
            enemy_.ApplyPullAttackHit(playerPosition, kPullAttackFlinchKnockback);
            hitCooldownTimer_ = kHitCooldown;
            hitRegisteredThisAttack_ = true;
        }
        return;
    }

    if (!isPlayerAttackHitboxActive)
    {
        return;
    }

    // 右方向ベクトルを作る
    const VECTOR rightDir = VGet(attackDir.z, 0.0f, -attackDir.x);

    // 攻撃種別に応じて判定サイズと位置を切り替える
    float swordForward = 78.0f;
    float swordSide = 42.0f;
    swordHitRadius_ = 44.0f;

    float frontForward = 52.0f;
    frontHitRadius_ = 30.0f;

    float knockbackDistance = 12.0f;

    // ダッシュ攻撃は少し広く強めにする
    if (playerAttackType == AttackType::StrongAttack)
    {
        swordForward = 92.0f;
        swordSide = 48.0f;
        swordHitRadius_ = 52.0f;

        frontForward = 62.0f;
        frontHitRadius_ = 36.0f;

        knockbackDistance = 16.0f;
    }
    // 回避攻撃も同じ流れで判定を出す
    else if (playerAttackType == AttackType::DodgeAttack)
    {
        swordForward = 86.0f;
        swordSide = 0.0f;
        swordHitRadius_ = 42.0f;

        frontForward = 58.0f;
        frontHitRadius_ = 30.0f;

        knockbackDistance = 14.0f;
    }

    // 武器判定の中心座標
    swordHitCenter_ = VGet(
        playerPosition.x + attackDir.x * swordForward + rightDir.x * swordSide,
        playerPosition.y + 94.0f,
        playerPosition.z + attackDir.z * swordForward + rightDir.z * swordSide);

    // 前方の補助判定
    frontHitCenter_ = VGet(
        playerPosition.x + attackDir.x * frontForward,
        playerPosition.y + 90.0f,
        playerPosition.z + attackDir.z * frontForward);

    // ヒット受付中だけ敵に反映する
    if (hitCooldownTimer_ <= 0.0f)
    {
        bool hit = false;

        if (enemy_.CheckAttackHit(swordHitCenter_, swordHitRadius_, playerPosition, knockbackDistance))
        {
            hit = true;
        }
        else if (enemy_.CheckAttackHit(frontHitCenter_, frontHitRadius_, playerPosition, knockbackDistance))
        {
            hit = true;
        }

        if (hit)
        {
            // エリアル始動攻撃なら敵を打ち上げて追従させる
            if (isPlayerAerialStarterAttackActive)
            {
                enemy_.LaunchToHeight(kAerialLaunchTargetY);
                aerialFollowJumpTargetY_ = kAerialLaunchTargetY;
                hasAerialFollowJumpRequest_ = true;
            }

            hitCooldownTimer_ = kHitCooldown;
            hitRegisteredThisAttack_ = true;
        }
    }
}

// 描画
void EnemyManager::Draw() const
{
    enemy_.Draw();

    // プレイヤー側の攻撃判定を描画
    if (showAttackHitbox_)
    {
        const int swordColor = GetColor(80, 200, 255);   // 武器側: 青系
        const int frontColor = GetColor(255, 160, 80);   // 前方判定: 橙系

        DrawSphere3D(swordHitCenter_, swordHitRadius_, 12, swordColor, swordColor, FALSE);
        DrawSphere3D(frontHitCenter_, frontHitRadius_, 12, frontColor, frontColor, FALSE);
    }
}

// 終了処理
void EnemyManager::Finalize()
{
    enemy_.Finalize();
}

bool EnemyManager::ConsumeAerialFollowJumpRequest(float& outTargetY)
{
    if (!hasAerialFollowJumpRequest_)
    {
        return false;
    }

    outTargetY = aerialFollowJumpTargetY_;
    hasAerialFollowJumpRequest_ = false;
    return true;
}

// ロックオン対象として使えるかどうか
bool EnemyManager::HasLockOnTarget() const
{
    return enemy_.IsModelLoaded();
}

// ロックオン用の注視位置を返す
VECTOR EnemyManager::GetLockOnTargetPosition() const
{
    // 敵の胸元あたりを狙うと見やすいので少し上へずらす
    const VECTOR enemyPosition = enemy_.GetPosition();
    return VGet(enemyPosition.x, enemyPosition.y + 110.0f, enemyPosition.z);
}