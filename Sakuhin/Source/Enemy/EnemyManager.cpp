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

    // ヒット連続発生抑制
    constexpr float kHitCooldown = 0.12f;
    constexpr float kAerialLaunchTargetY = 220.0f;
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

    // 相対パス候補を順番に試す
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

// 敵更新
void EnemyManager::Update(const VECTOR& playerPosition,
    const VECTOR& playerFacingDirection,
    AttackType playerAttackType,
    int playerComboStep,
    bool isPlayerAttackHitboxActive,
    bool isPlayerAttacking,
    bool isPlayerAerialStarterAttackActive)
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

    // コンボ段が進んだら、その段のヒットを許可
    if (isPlayerAttacking && wasPlayerAttackingPrev_ && playerComboStep != prevComboStep_)
    {
        hitRegisteredThisAttack_ = false;
        prevComboStep_ = playerComboStep;
    }

    // 攻撃終了フレームで次攻撃に備えて状態を戻す
    if (!isPlayerAttacking && wasPlayerAttackingPrev_)
    {
        hitRegisteredThisAttack_ = false;
        prevComboStep_ = 0;
    }

    wasPlayerAttackingPrev_ = isPlayerAttacking;

    enemy_.Update();

    // プレイヤー側が有効と判断したタイミングのみ判定を可視化・適用する
    showAttackHitbox_ = isPlayerAttackHitboxActive;
    if (!isPlayerAttackHitboxActive)
    {
        return;
    }

    // この攻撃段ですでにヒット済みなら、判定は描画のみで実ヒット処理は行わない
    if (hitRegisteredThisAttack_)
    {
        return;
    }

    // 攻撃方向（プレイヤーの向きベクトルをそのまま使用）
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

    // 右方向（剣側オフセット用）
    const VECTOR rightDir = VGet(attackDir.z, 0.0f, -attackDir.x);

    // 攻撃種別に応じて判定サイズを調整
    float swordForward = 78.0f;
    float swordSide = 42.0f;
    swordHitRadius_ = 44.0f;

    float frontForward = 52.0f;
    frontHitRadius_ = 30.0f;

    float knockbackDistance = 12.0f;

    if (playerAttackType == AttackType::StrongAttack)
    {
        swordForward = 92.0f;
        swordSide = 48.0f;
        swordHitRadius_ = 52.0f;

        frontForward = 62.0f;
        frontHitRadius_ = 36.0f;

        knockbackDistance = 16.0f;
    }

    // 剣判定（プレイヤー向き基準で前方 + 右側へ寄せる）
    swordHitCenter_ = VGet(
        playerPosition.x + attackDir.x * swordForward + rightDir.x * swordSide,
        playerPosition.y + 94.0f,
        playerPosition.z + attackDir.z * swordForward + rightDir.z * swordSide);

    // 前面補助判定（プレイヤー向き基準の正面）
    frontHitCenter_ = VGet(
        playerPosition.x + attackDir.x * frontForward,
        playerPosition.y + 90.0f,
        playerPosition.z + attackDir.z * frontForward);

    // ヒット判定は剣→前面の順で評価
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

// 敵描画
void EnemyManager::Draw() const
{
    enemy_.Draw();

    // プレイヤー攻撃判定の可視化
    if (showAttackHitbox_)
    {
        const int swordColor = GetColor(80, 200, 255);   // 剣判定: 水色
        const int frontColor = GetColor(255, 160, 80);   // 前面判定: 橙

        DrawSphere3D(swordHitCenter_, swordHitRadius_, 12, swordColor, swordColor, FALSE);
        DrawSphere3D(frontHitCenter_, frontHitRadius_, 12, frontColor, frontColor, FALSE);
    }
}

// 敵終了処理
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