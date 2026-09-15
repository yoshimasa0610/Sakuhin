#include "EnemyManager.h"

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

// Enemy.xの読み込み
void EnemyManager::Initialize()
{
    enemy_.Initialize();

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
void EnemyManager::Update()
{
    enemy_.Update();
}

// 敵描画
void EnemyManager::Draw() const
{
    enemy_.Draw();
}

// 敵終了処理
void EnemyManager::Finalize()
{
    enemy_.Finalize();
}