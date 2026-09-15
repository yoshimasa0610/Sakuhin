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

    // 低レベル再生API
    bool PlayAnimation(int animIndex, float speed = 1.0f, bool loop = true, float startTime = 0.0f, float endTime = 0.0f);

    // 指定区間を指定時間で再生（アニメ処理をここに集約）
    bool PlaySegment(int animIndex, float startTime, float endTime, float duration, bool loop);

    // 区間指定なしで全体を指定時間再生
    bool PlayOneShot(int animIndex, float duration);
    bool PlayLoop(int animIndex, float duration);

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

    // 指定アニメ情報取得
    bool IsValidAnimIndex(int animIndex) const;
    float GetAnimTotalTime(int animIndex) const;

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
