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
    , startTime_(0.0f)
    , endTime_(0.0f)
{
}

void AnimationController::Initialize(int modelHandle)
{
    modelHandle_ = modelHandle;
    attachmentIndex_ = -1;
    currentAnimIndex_ = -1;
    isPlaying_ = false;
    currentTime_ = 0.0f;
    startTime_ = 0.0f;
    endTime_ = 0.0f;
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
    const float limit = (endTime_ > 0.0f) ? endTime_ : totalTime;

    if (limit > 0.0f)
    {
        if (loop_)
        {
            if (currentTime_ >= limit)
            {
                currentTime_ = startTime_ + fmod(currentTime_ - startTime_, limit - startTime_);
            }
        }
        else
        {
            if (currentTime_ >= limit)
            {
                currentTime_ = limit;
                isPlaying_ = false;

                TCHAR debugMsg[256];
                _stprintf_s(debugMsg, 256, _T("[AnimController] Animation finished: time=%.2f, limit=%.2f\n"), currentTime_, limit);
                OutputDebugString(debugMsg);
            }
        }
    }

    MV1SetAttachAnimTime(modelHandle_, attachmentIndex_, currentTime_);
}

bool AnimationController::PlayAnimation(int animIndex, float speed, bool loop, float startTime, float endTime)
{
    if (modelHandle_ < 0 || animIndex < 0)
    {
        OutputDebugString(_T("PlayAnimation failed: modelHandle or animIndex invalid\n"));
        return false;
    }

    // 同じアニメーションでも開始位置が異なる場合は再アタッチが必要なので、常に再開始する
    if (attachmentIndex_ >= 0)
    {
        MV1DetachAnim(modelHandle_, attachmentIndex_);
    }

    attachmentIndex_ = MV1AttachAnim(modelHandle_, animIndex);

    if (attachmentIndex_ >= 0)
    {
        currentAnimIndex_ = animIndex;
        isPlaying_ = true;
        currentTime_ = startTime;
        speed_ = speed;
        loop_ = loop;
        startTime_ = startTime;
        endTime_ = endTime;

        MV1SetAttachAnimBlendRate(modelHandle_, attachmentIndex_, 1.0f);
        MV1SetAttachAnimTime(modelHandle_, attachmentIndex_, currentTime_);

        TCHAR debugMsg[256];
        _stprintf_s(debugMsg, 256, _T("PlayAnimation Success: animIndex=%d, start=%.2f, end=%.2f\n"), animIndex, startTime, endTime);
        OutputDebugString(debugMsg);
        return true;
    }

    currentAnimIndex_ = -1;
    TCHAR debugMsg[256];
    _stprintf_s(debugMsg, 256, _T("PlayAnimation failed: MV1AttachAnim returned -1 for animIndex=%d\n"), animIndex);
    OutputDebugString(debugMsg);
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

int AnimationController::GetCurrentAnimIndex() const
{
    return currentAnimIndex_;
}
