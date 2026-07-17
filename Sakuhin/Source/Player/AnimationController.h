#pragma once

#include <DxLib.h>

class AnimationController
{
public:
    AnimationController();

    void Initialize(int modelHandle);
    void Update();
    bool PlayAnimation(int animIndex, float speed = 1.0f, bool loop = true, float startTime = 0.0f, float endTime = 0.0f);
    void SetCurrentTime(float time);
    void StopAnimation();
    void Finalize();

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
