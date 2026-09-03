#pragma once

#include <DxLib.h>
#include "Player.h"

// プレイヤーのライフサイクル管理クラス
class PlayerManager
{
public:
    // 初期化
    void Initialize();

    // 更新
    void Update(float cameraYaw);

    // 描画
    void Draw() const;

    // 終了処理
    void Finalize();

    // プレイヤー位置
    VECTOR GetPlayerPosition() const;

    // プレイヤー攻撃状態
    AttackType GetPlayerCurrentAttack() const;
    bool IsPlayerAttacking() const;

    // モデル読み込み状態
    bool IsPlayerModelLoaded() const;
    const TCHAR* GetLoadedModelPath() const;

private:
    Player player_;
    bool modelLoaded_ = false;
    TCHAR loadedModelPath_[MAX_PATH] = _T("");
};
