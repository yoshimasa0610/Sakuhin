#pragma once

#include "Enemy.h"

// 敵管理クラス（現状は1体を管理）
class EnemyManager
{
public:
    // 初期化（モデル読み込み）
    void Initialize();

    // 更新
    void Update();

    // 描画
    void Draw() const;

    // 終了処理
    void Finalize();

private:
    Enemy enemy_;
};