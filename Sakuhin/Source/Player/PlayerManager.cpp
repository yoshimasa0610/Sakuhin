#include "PlayerManager.h"

#include <Windows.h>
#include <io.h>
#include <tchar.h>

namespace
{
    // ファイル存在確認
    bool FileExists(const TCHAR* path)
    {
        return _taccess(path, 0) == 0;
    }
}

// プレイヤーとモデル読み込みの初期化
void PlayerManager::Initialize()
{
    player_.Initialize();

    modelLoaded_ = false;
    _tcscpy_s(loadedModelPath_, _T("Load Failed"));

    // 相対パス候補を順番に試す
    const TCHAR* relativeCandidates[] =
    {
        _T("Source/Images/Player/Player.x"),
        _T("../Source/Images/Player/Player.x"),
        _T("../../Source/Images/Player/Player.x")
    };

    for (const auto& path : relativeCandidates)
    {
        if (!FileExists(path))
        {
            continue;
        }

        if (player_.LoadModel(path))
        {
            modelLoaded_ = true;
            _tcscpy_s(loadedModelPath_, path);
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
    _stprintf_s(absolutePath, _T("%s\\..\\..\\Source\\Images\\Player\\Player.x"), exePath);

    if (!FileExists(absolutePath))
    {
        return;
    }

    if (player_.LoadModel(absolutePath))
    {
        modelLoaded_ = true;
        _tcscpy_s(loadedModelPath_, absolutePath);
        return;
    }
}

// プレイヤー更新
void PlayerManager::Update(float cameraYaw)
{
    player_.Update(cameraYaw);
}

// プレイヤー描画
void PlayerManager::Draw() const
{
    player_.Draw();
}

// 終了処理
void PlayerManager::Finalize()
{
    player_.Finalize();
}

// プレイヤー位置取得
VECTOR PlayerManager::GetPlayerPosition() const
{
    return player_.GetPosition();
}

// 攻撃種別取得
AttackType PlayerManager::GetPlayerCurrentAttack() const
{
    return player_.GetCurrentAttack();
}

// 攻撃中判定
bool PlayerManager::IsPlayerAttacking() const
{
    return player_.IsAttacking();
}

// モデル読み込み成否
bool PlayerManager::IsPlayerModelLoaded() const
{
    return modelLoaded_;
}

// 読み込んだモデルパス取得
const TCHAR* PlayerManager::GetLoadedModelPath() const
{
    return loadedModelPath_;
}