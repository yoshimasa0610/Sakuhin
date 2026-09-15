#pragma once

#include <DxLib.h>

// 敵1体のモデル管理クラス
class Enemy
{
public:
    // 状態初期化
    void Initialize();

    // モデル読み込み
    bool LoadModel(const TCHAR* modelPath);

    // 毎フレーム更新（位置・回転の適用）
    void Update();

    // 描画
    void Draw() const;

    // 終了処理（モデル解放）
    void Finalize();

    // モデル読み込み状態
    bool IsModelLoaded() const;

    // 読み込んだパス取得
    const TCHAR* GetLoadedModelPath() const;

private:
    int modelHandle_ = -1;
    bool modelLoaded_ = false;

    // 配置情報（マップ中央付近に固定）
    VECTOR position_ = VGet(0.0f, 0.0f, 300.0f);
    float rotationY_ = 3.14159f;

    // 読み込み済みパス
    TCHAR loadedModelPath_[MAX_PATH] = _T("");
};