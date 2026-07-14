#include "AnimationController.h"

#include <cmath>

AnimationController::AnimationController()
    : modelHandle_(-1)
    , attachmentIndex_(-1)
    , currentAnimIndex_(-1)
    , isPlaying_(false)
    , currentTime_(0.0f)
    , speed_(1.0f)
    , loop_(true)
{
}

void AnimationController::Initialize(int modelHandle)
{
    modelHandle_ = modelHandle;
    attachmentIndex_ = -1;
    currentAnimIndex_ = -1;
    isPlaying_ = false;
    currentTime_ = 0.0f;
}

void AnimationController::Update()
{
    if (modelHandle_ < 0 || attachmentIndex_ < 0 || !isPlaying_)
    {
        return;
    }

    const float deltaTime = 1.0f / 60.0f;
    currentTime_ += deltaTime * speed_;

    const float totalTime = MV1GetAnimTotalTime(modelHandle_, currentAnimIndex_);
    if (totalTime > 0.0f)
    {
        if (loop_)
        {
            currentTime_ = fmod(currentTime_, totalTime);
        }
        else if (currentTime_ >= totalTime)
        {
            currentTime_ = totalTime;
            isPlaying_ = false;
        }
    }

    MV1SetAttachAnimTime(modelHandle_, attachmentIndex_, currentTime_);
}

bool AnimationController::PlayAnimation(int animIndex, float speed, bool loop)
{
    if (modelHandle_ < 0 || animIndex < 0)
    {
        return false;
    }

    if (isPlaying_ && currentAnimIndex_ == animIndex)
    {
        speed_ = speed;
        loop_ = loop;
        return true;
    }

    if (attachmentIndex_ >= 0)
    {
        MV1DetachAnim(modelHandle_, attachmentIndex_);
    }

    attachmentIndex_ = MV1AttachAnim(modelHandle_, animIndex);

    if (attachmentIndex_ >= 0)
    {
        currentAnimIndex_ = animIndex;
        isPlaying_ = true;
        currentTime_ = 0.0f;
        speed_ = speed;
        loop_ = loop;

        MV1SetAttachAnimBlendRate(modelHandle_, attachmentIndex_, 1.0f);
        return true;
    }

    currentAnimIndex_ = -1;
    return false;
}

void AnimationController::SetCurrentTime(float time)
{
    if (modelHandle_ < 0 || attachmentIndex_ < 0 || currentAnimIndex_ < 0)
    {
        return;
    }

    const float totalTime = MV1GetAnimTotalTime(modelHandle_, currentAnimIndex_);
    if (totalTime > 0.0f)
    {
        if (loop_)
        {
            currentTime_ = fmod(time, totalTime);
            if (currentTime_ < 0.0f)
            {
                currentTime_ += totalTime;
            }
        }
        else
        {
            currentTime_ = (time < 0.0f) ? 0.0f : ((time > totalTime) ? totalTime : time);
        }
    }
    else
    {
        currentTime_ = time;
    }

    MV1SetAttachAnimTime(modelHandle_, attachmentIndex_, currentTime_);
}

void AnimationController::StopAnimation()
{
    if (attachmentIndex_ >= 0 && modelHandle_ >= 0)
    {
        MV1DetachAnim(modelHandle_, attachmentIndex_);
        attachmentIndex_ = -1;
    }

    currentAnimIndex_ = -1;
    isPlaying_ = false;
    currentTime_ = 0.0f;
}

void AnimationController::Finalize()
{
    StopAnimation();
    modelHandle_ = -1;
}

bool AnimationController::IsPlaying() const
{
    return isPlaying_;
}

float AnimationController::GetCurrentTime() const
{
    return currentTime_;
}

float AnimationController::GetAnimationDuration() const
{
    if (modelHandle_ < 0 || currentAnimIndex_ < 0)
    {
        return 0.0f;
    }

    return MV1GetAnimTotalTime(modelHandle_, currentAnimIndex_);
}
