#include "AnimationController.h"

#include <cmath>

// コンストラクタ
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

// モデルに対するアニメ制御初期化
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

// 再生中アニメ時間を進める
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
            }
        }
    }

    MV1SetAttachAnimTime(modelHandle_, attachmentIndex_, currentTime_);
}

// アニメ再生開始
bool AnimationController::PlayAnimation(int animIndex, float speed, bool loop, float startTime, float endTime)
{
    if (modelHandle_ < 0 || animIndex < 0)
    {
        return false;
    }

    // 既存のアタッチを外してから新規アタッチ
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

        return true;
    }

    currentAnimIndex_ = -1;
    return false;
}

// 指定区間を指定時間で再生
bool AnimationController::PlaySegment(int animIndex, float startTime, float endTime, float duration, bool loop)
{
    if (!IsValidAnimIndex(animIndex))
    {
        return false;
    }

    const float totalTime = GetAnimTotalTime(animIndex);
    const float start = (startTime < 0.0f) ? 0.0f : startTime;
    const float end = (endTime > 0.0f) ? endTime : totalTime;
    const float length = end - start;

    const float speed = (duration > 0.0f && length > 0.0f) ? (length / duration) : 1.0f;
    return PlayAnimation(animIndex, speed, loop, start, end);
}

// 全体を1回再生
bool AnimationController::PlayOneShot(int animIndex, float duration)
{
    if (!IsValidAnimIndex(animIndex))
    {
        return false;
    }

    const float totalTime = GetAnimTotalTime(animIndex);
    return PlaySegment(animIndex, 0.0f, totalTime, duration, false);
}

// 全体をループ再生
bool AnimationController::PlayLoop(int animIndex, float duration)
{
    if (!IsValidAnimIndex(animIndex))
    {
        return false;
    }

    const float totalTime = GetAnimTotalTime(animIndex);
    return PlaySegment(animIndex, 0.0f, totalTime, duration, true);
}

// 再生位置を直接設定
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

// 再生停止
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

// 終了処理
void AnimationController::Finalize()
{
    StopAnimation();
    modelHandle_ = -1;
}

// 再生中か
bool AnimationController::IsPlaying() const
{
    return isPlaying_;
}

// 現在再生時間
float AnimationController::GetCurrentTime() const
{
    return currentTime_;
}

// 現在アニメの全長
float AnimationController::GetAnimationDuration() const
{
    if (modelHandle_ < 0 || currentAnimIndex_ < 0)
    {
        return 0.0f;
    }

    return MV1GetAnimTotalTime(modelHandle_, currentAnimIndex_);
}

// 現在アニメ番号
int AnimationController::GetCurrentAnimIndex() const
{
    return currentAnimIndex_;
}

// 指定アニメ番号が有効か
bool AnimationController::IsValidAnimIndex(int animIndex) const
{
    if (modelHandle_ < 0 || animIndex < 0)
    {
        return false;
    }

    return animIndex < MV1GetAnimNum(modelHandle_);
}

// 指定アニメの総時間取得
float AnimationController::GetAnimTotalTime(int animIndex) const
{
    if (!IsValidAnimIndex(animIndex))
    {
        return 0.0f;
    }

    return MV1GetAnimTotalTime(modelHandle_, animIndex);
}
