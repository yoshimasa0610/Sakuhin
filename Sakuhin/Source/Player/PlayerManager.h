#pragma once

#include <DxLib.h>
#include "Player.h"

class PlayerManager
{
public:
    void Initialize();
    void Update();
    void Draw() const;
    void Finalize();

    VECTOR GetPlayerPosition() const;
    AttackType GetPlayerCurrentAttack() const;
    bool IsPlayerAttacking() const;

    bool IsPlayerModelLoaded() const;
    const TCHAR* GetLoadedModelPath() const;

private:
    Player player_;
    bool modelLoaded_ = false;
    TCHAR loadedModelPath_[MAX_PATH] = _T("");
};
