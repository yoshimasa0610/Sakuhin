#include "PlayerManager.h"

#include <Windows.h>
#include <io.h>
#include <tchar.h>

namespace
{
    bool FileExists(const TCHAR* path)
    {
        return _taccess(path, 0) == 0;
    }

    void OutputLoadLog(const TCHAR* message, const TCHAR* path)
    {
        TCHAR buffer[1024] = { 0 };
        _stprintf_s(buffer, _T("[PlayerModel] %s : %s\n"), message, path);
        OutputDebugString(buffer);
    }
}

void PlayerManager::Initialize()
{
    player_.Initialize();

    modelLoaded_ = false;
    _tcscpy_s(loadedModelPath_, _T("Load Failed"));

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
            OutputLoadLog(_T("File not found"), path);
            continue;
        }

        if (player_.LoadModel(path))
        {
            modelLoaded_ = true;
            _tcscpy_s(loadedModelPath_, path);
            OutputLoadLog(_T("Load success"), path);
            return;
        }

        OutputLoadLog(_T("Load failed"), path);
    }

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
        OutputLoadLog(_T("File not found"), absolutePath);
        return;
    }

    if (player_.LoadModel(absolutePath))
    {
        modelLoaded_ = true;
        _tcscpy_s(loadedModelPath_, absolutePath);
        OutputLoadLog(_T("Load success"), absolutePath);
        return;
    }

    OutputLoadLog(_T("Load failed"), absolutePath);
}

void PlayerManager::Update()
{
    player_.Update();
}

void PlayerManager::Draw() const
{
    player_.Draw();
}

void PlayerManager::Finalize()
{
    player_.Finalize();
}

VECTOR PlayerManager::GetPlayerPosition() const
{
    return player_.GetPosition();
}

AttackType PlayerManager::GetPlayerCurrentAttack() const
{
    return player_.GetCurrentAttack();
}

bool PlayerManager::IsPlayerAttacking() const
{
    return player_.IsAttacking();
}

bool PlayerManager::IsPlayerModelLoaded() const
{
    return modelLoaded_;
}

const TCHAR* PlayerManager::GetLoadedModelPath() const
{
    return loadedModelPath_;
}