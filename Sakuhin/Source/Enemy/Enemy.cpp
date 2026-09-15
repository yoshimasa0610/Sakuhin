#include "Enemy.h"

#include <tchar.h>

// 初期状態に戻す
void Enemy::Initialize()
{
    modelLoaded_ = false;
    modelHandle_ = -1;
    position_ = VGet(0.0f, 0.0f, 300.0f);
    rotationY_ = 3.14159f;
    _tcscpy_s(loadedModelPath_, _T("Load Failed"));
}

// モデル読み込み
bool Enemy::LoadModel(const TCHAR* modelPath)
{
    Finalize();

    modelHandle_ = MV1LoadModel(modelPath);
    if (modelHandle_ < 0)
    {
        modelLoaded_ = false;
        _tcscpy_s(loadedModelPath_, _T("Load Failed"));
        return false;
    }

    // 初期変換を適用
    MV1SetScale(modelHandle_, VGet(1.0f, 1.0f, 1.0f));
    MV1SetPosition(modelHandle_, position_);
    MV1SetRotationXYZ(modelHandle_, VGet(0.0f, rotationY_, 0.0f));

    modelLoaded_ = true;
    _tcscpy_s(loadedModelPath_, modelPath);
    return true;
}

// 毎フレーム、現在の配置を反映
void Enemy::Update()
{
    if (!modelLoaded_ || modelHandle_ < 0)
    {
        return;
    }

    MV1SetPosition(modelHandle_, position_);
    MV1SetRotationXYZ(modelHandle_, VGet(0.0f, rotationY_, 0.0f));
}

// 描画
void Enemy::Draw() const
{
    if (modelLoaded_ && modelHandle_ >= 0)
    {
        MV1DrawModel(modelHandle_);
    }
}

// モデル解放
void Enemy::Finalize()
{
    if (modelHandle_ >= 0)
    {
        MV1DeleteModel(modelHandle_);
        modelHandle_ = -1;
    }

    modelLoaded_ = false;
}

// 読み込み済みか
bool Enemy::IsModelLoaded() const
{
    return modelLoaded_;
}

// 読み込んだモデルパス
const TCHAR* Enemy::GetLoadedModelPath() const
{
    return loadedModelPath_;
}