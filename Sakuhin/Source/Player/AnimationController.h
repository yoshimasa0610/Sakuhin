#pragma once

#include <DxLib.h>

// DxLibのMV1アニメーションを1本だけ管理するコントローラー
class AnimationController
{
public:
    AnimationController();

    // 対象モデルを設定して初期化
    void Initialize(int modelHandle);

    // 再生時間の更新
    void Update();

    // アニメ再生開始
    bool PlayAnimation(int animIndex, float speed = 1.0f, bool loop = true, float startTime = 0.0f, float endTime = 0.0f);

    // 再生位置を直接指定
    void SetCurrentTime(float time);

    // 再生停止
    void StopAnimation();

    // 終了処理
    void Finalize();

    // 状態取得
    bool IsPlaying() const;
    float GetCurrentTime() const;
    float GetAnimationDuration() const;
    int GetCurrentAnimIndex() const;

private:
    int modelHandle_;
    int attachmentIndex_;
    int currentAnimIndex_;
    bool isPlaying_;
    float currentTime_;
    float speed_;
    bool loop_;
    float startTime_;
    float endTime_;
};
