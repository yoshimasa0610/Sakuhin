#include "Player.h"

#include <cmath>

namespace
{
    constexpr float kModelScale = 120.0f;
    constexpr float kDefaultRotationY = 3.14159f;
    constexpr int kAttackAnimIndex = 0;
    constexpr int kWalkAnimIndex = 1;
    constexpr int kIdleAnimIndex = 2;
    constexpr int kJumpAnimIndex = 3;
    constexpr int kDodgeBackAnimIndex = 4;
    constexpr int kDodgeForwardAnimIndex = 5;
}

Player::Player()
    : position_(VGet(0.0f, 0.0f, 0.0f))
    , moveSpeed_(4.0f)
    , modelRotationY_(kDefaultRotationY)
    , modelHandle_(-1)
    , useWalkAnimation_(false)
    , modelLoaded_(false)
    , idleAnimSpeed_(1.0f)
    , walkAnimSpeed_(1.0f)
    , totalAnimationCount_(0)
    , previousMouseInput_(0)
    , comboStep_(0)
    , pendingCombo_(false)
    , comboSegmentDuration_(0.0f)
    , isJumping_(false)
    , isDodging_(false)
    , actionTimer_(0.0f)
    , jumpHeight_(0.0f)
    , dodgeDirection_(VGet(0.0f, 0.0f, 0.0f))
    , savedPosition_(VGet(0.0f, 0.0f, 0.0f))
    , previousKeyInput_(0)
{
}

void Player::Initialize()
{
    position_ = VGet(0.0f, 0.0f, 0.0f);
    modelRotationY_ = kDefaultRotationY;
    attack_.Initialize();
    animationController_.Initialize(-1);
    useWalkAnimation_ = false;
    comboStep_ = 0;
    pendingCombo_ = false;
    comboSegmentDuration_ = 0.0f;
    isJumping_ = false;
    isDodging_ = false;
    actionTimer_ = 0.0f;
    jumpHeight_ = 0.0f;
    dodgeDirection_ = VGet(0.0f, 0.0f, 0.0f);
    savedPosition_ = VGet(0.0f, 0.0f, 0.0f);
    previousKeyInput_ = 0;
}

bool Player::LoadModel(const TCHAR* modelPath)
{
    Finalize();

    modelHandle_ = MV1LoadModel(modelPath);
    if (modelHandle_ < 0)
    {
        modelLoaded_ = false;
        totalAnimationCount_ = 0;
        OutputDebugString(_T("Failed to load model\n"));
        return false;
    }

    // モデルのアニメーション数を確認
    totalAnimationCount_ = MV1GetAnimNum(modelHandle_);

    // 各アニメーションの情報をファイルに出力（デバッグ用）
    TCHAR debugMsg[512];
    _stprintf_s(debugMsg, 512, _T("=== Model Animation Info ===\n"));
    OutputDebugString(debugMsg);
    _stprintf_s(debugMsg, 512, _T("Total Animations: %d\n"), totalAnimationCount_);
    OutputDebugString(debugMsg);

    for (int i = 0; i < totalAnimationCount_; i++)
    {
        float totalTime = MV1GetAnimTotalTime(modelHandle_, i);
        const TCHAR* animName = MV1GetAnimName(modelHandle_, i);
        if (animName == nullptr) animName = _T("(NULL)");

        _stprintf_s(debugMsg, 512, _T("Anim[%d]: %.2f sec, Name: %s\n"), i, totalTime, animName);
        OutputDebugString(debugMsg);
    }

    MV1SetScale(modelHandle_, VGet(kModelScale, kModelScale, kModelScale));
    MV1SetPosition(modelHandle_, position_);
    MV1SetRotationXYZ(modelHandle_, VGet(0.0f, modelRotationY_, 0.0f));

    walkAnimSpeed_ = CalculateAnimationSpeed(modelHandle_, kWalkAnimIndex, 1.5f);
    idleAnimSpeed_ = CalculateAnimationSpeed(modelHandle_, kIdleAnimIndex, 4.0f);

    // 攻撃アニメーション全体を3等分してコンボ1段分の時間を算出
    const float attackTotalTime = MV1GetAnimTotalTime(modelHandle_, kAttackAnimIndex);

    // デバッグ: 実際のアニメーション時間をログ出力
    TCHAR debugMsg2[256];
    _stprintf_s(debugMsg2, 256, _T("=== ATTACK ANIMATION INFO ===\n"));
    OutputDebugString(debugMsg2);
    _stprintf_s(debugMsg2, 256, _T("Attack anim index: %d\n"), kAttackAnimIndex);
    OutputDebugString(debugMsg2);
    _stprintf_s(debugMsg2, 256, _T("Attack anim total time: %.2f seconds\n"), attackTotalTime);
    OutputDebugString(debugMsg2);

    // 攻撃アニメーションが異常に長い場合は、実際の長さを手動で設定
    // モデルのFPS設定が正しくないため、手動で適切な時間を設定
    const float actualAttackDuration = 1.5f;  // 3連撃全体で1.5秒
    comboSegmentDuration_ = actualAttackDuration / 3.0f;  // 1段あたり0.5秒

    _stprintf_s(debugMsg2, 256, _T("Using manual attack duration: %.2f sec\n"), actualAttackDuration);
    OutputDebugString(debugMsg2);
    _stprintf_s(debugMsg2, 256, _T("Combo segment duration: %.2f sec\n"), comboSegmentDuration_);
    OutputDebugString(debugMsg2);
    _stprintf_s(debugMsg2, 256, _T("=============================\n"));
    OutputDebugString(debugMsg2);

    modelLoaded_ = true;
    SwitchAnimation(false);

    return true;
}

void Player::Update()
{
    // アクション中の更新
    if (isJumping_ || isDodging_)
    {
        const float deltaTime = 1.0f / 60.0f;
        actionTimer_ += deltaTime;

        if (isJumping_)
        {
            // ジャンプの放物線運動
            const float jumpDuration = 0.8f;
            const float maxHeight = 50.0f;

            if (actionTimer_ < jumpDuration)
            {
                const float t = actionTimer_ / jumpDuration;
                jumpHeight_ = maxHeight * 4.0f * t * (1.0f - t);  // 放物線
            }
            else
            {
                jumpHeight_ = 0.0f;
                isJumping_ = false;
                actionTimer_ = 0.0f;
            }
        }
        else if (isDodging_)
        {
            // 回避移動
            const float dodgeDuration = 0.5f;
            const float animDuration = 0.4f;
            const float dodgeSpeed = 12.0f;  // 移動速度を調整

            // 移動はコードで制御（アニメーションのルートモーションは無視）
            if (actionTimer_ < dodgeDuration)
            {
                const float moveAmount = dodgeSpeed * deltaTime;
                position_.x += dodgeDirection_.x * moveAmount;
                position_.z += dodgeDirection_.z * moveAmount;
            }

            // アニメーションは0.4秒で終了
            if (actionTimer_ >= animDuration)
            {
                isDodging_ = false;
                actionTimer_ = 0.0f;

                TCHAR debugMsg[256];
                _stprintf_s(debugMsg, 256, _T("[Dodge End] Final position: (%.2f, %.2f)\n"), 
                           position_.x, position_.z);
                OutputDebugString(debugMsg);
            }
        }

        animationController_.Update();

        // モデル位置を常にposition_で上書き（アニメーションのルートモーションを無視）
        if (modelHandle_ >= 0)
        {
            VECTOR drawPos = position_;
            drawPos.y += jumpHeight_;
            MV1SetPosition(modelHandle_, drawPos);
            MV1SetRotationXYZ(modelHandle_, VGet(0.0f, modelRotationY_, 0.0f));
        }

        // アクション終了後、通常状態に戻る処理を継続
        if (!isJumping_ && !isDodging_)
        {
            // アクション終了後、通常アニメーションに戻す
            SwitchAnimation(false);
        }
        else
        {
            return;  // まだアクション中なら早期リターン
        }
    }

    // 通常の移動処理
    VECTOR move = VGet(0.0f, 0.0f, 0.0f);
    const bool isShiftPressed = CheckHitKey(KEY_INPUT_LSHIFT) || CheckHitKey(KEY_INPUT_RSHIFT);

    if (CheckHitKey(KEY_INPUT_W)) move.z += 1.0f;
    if (CheckHitKey(KEY_INPUT_S)) move.z -= 1.0f;
    if (CheckHitKey(KEY_INPUT_A)) move.x -= 1.0f;
    if (CheckHitKey(KEY_INPUT_D)) move.x += 1.0f;

    const float length = std::sqrt(move.x * move.x + move.z * move.z);
    const bool isMoving = (length > 0.0f);

    // スペースキーでジャンプ
    int currentKeyInput = 0;
    if (CheckHitKey(KEY_INPUT_SPACE)) currentKeyInput |= 1;
    if (CheckHitKey(KEY_INPUT_W)) currentKeyInput |= 2;
    if (CheckHitKey(KEY_INPUT_S)) currentKeyInput |= 4;
    if (CheckHitKey(KEY_INPUT_A)) currentKeyInput |= 8;
    if (CheckHitKey(KEY_INPUT_D)) currentKeyInput |= 16;

    const bool spacePressed = (currentKeyInput & 1) && !(previousKeyInput_ & 1);

    // Shiftキー押下時の回避処理
    const bool shiftPressed = isShiftPressed && !(CheckHitKey(KEY_INPUT_LSHIFT) || CheckHitKey(KEY_INPUT_RSHIFT));  // 今回押された瞬間を検知したい場合

    // Shift単体で後方回避、WASD+Shiftで前方回避
    if (isShiftPressed && !attack_.IsAttacking() && comboStep_ == 0)
    {
        // WASDいずれかが押されている場合は前方回避
        if (isMoving)
        {
            isDodging_ = true;
            actionTimer_ = 0.0f;

            // 前方方向を計算（現在の向き）
            dodgeDirection_ = VGet(
                std::sin(modelRotationY_ - kDefaultRotationY),
                0.0f,
                std::cos(modelRotationY_ - kDefaultRotationY)
            );

            PlayActionAnimation(kDodgeForwardAnimIndex, 0.5f);

            previousKeyInput_ = currentKeyInput;
            return;
        }
        // WASD押されていない場合は後方回避
        else
        {
            isDodging_ = true;
            actionTimer_ = 0.0f;

            // 後方方向を計算（現在の向きの逆）
            dodgeDirection_ = VGet(
                -std::sin(modelRotationY_ - kDefaultRotationY),
                0.0f,
                -std::cos(modelRotationY_ - kDefaultRotationY)
            );

            PlayActionAnimation(kDodgeBackAnimIndex, 0.5f);

            previousKeyInput_ = currentKeyInput;
            return;
        }
    }

    // スペースキーでジャンプ
    if (spacePressed && !attack_.IsAttacking() && comboStep_ == 0)
    {
        isJumping_ = true;
        actionTimer_ = 0.0f;
        jumpHeight_ = 0.0f;
        PlayActionAnimation(kJumpAnimIndex, 0.8f);

        previousKeyInput_ = currentKeyInput;
        return;
    }

    // 通常移動
    if (isMoving && !isShiftPressed)
    {
        move.x /= length;
        move.z /= length;

        position_.x += move.x * moveSpeed_;
        position_.z += move.z * moveSpeed_;

        const bool isBackward = CheckHitKey(KEY_INPUT_S) != 0;
        if (isBackward)
        {
            modelRotationY_ = static_cast<float>(std::atan2(-move.x, -move.z));
        }
        else
        {
            modelRotationY_ = static_cast<float>(std::atan2(move.x, move.z)) + kDefaultRotationY;
        }
    }

    // 攻撃入力
    int currentAttackInput = 0;
    if (CheckHitKey(KEY_INPUT_Q))
    {
        currentAttackInput |= 1;
    }

    const bool attackPressed = (currentAttackInput & 1) && !(previousMouseInput_ & 1);

    if (attackPressed)
    {
        if (comboStep_ == 0)
        {
            attack_.ExecuteWeakAttack();
            PlayComboSegment(0);
        }
        else if (comboStep_ <= 2)
        {
            pendingCombo_ = true;
        }
    }

    if (!attack_.IsAttacking() && comboStep_ == 0)
    {
        SwitchAnimation(isMoving);
    }

    previousMouseInput_ = currentAttackInput;
    previousKeyInput_ = currentKeyInput;

    attack_.Update();
    UpdateAttackAnimation();
    animationController_.Update();

    if (modelHandle_ >= 0)
    {
        MV1SetPosition(modelHandle_, position_);
        MV1SetRotationXYZ(modelHandle_, VGet(0.0f, modelRotationY_, 0.0f));
    }
}

void Player::Draw() const
{
    if (modelLoaded_ && modelHandle_ >= 0)
    {
        MV1DrawModel(modelHandle_);
    }
    else
    {
        DrawSphere3D(position_, 20.0f, 16, GetColor(220, 80, 80), GetColor(255, 255, 255), TRUE);
    }

    // デバッグ表示：モデルハンドルとアニメーション情報
    DrawFormatString(10, 110, GetColor(100, 200, 255), _T("ModelHandle: %d"), modelHandle_);
    DrawFormatString(10, 130, GetColor(100, 200, 255), _T("TotalAnims: %d"), totalAnimationCount_);
    DrawFormatString(10, 150, GetColor(100, 200, 255), _T("AnimPlaying: %s"), 
                     animationController_.IsPlaying() ? _T("TRUE") : _T("FALSE"));
    DrawFormatString(10, 170, GetColor(255, 200, 100), _T("ComboStep: %d, Pending: %s"), 
                     comboStep_, pendingCombo_ ? _T("YES") : _T("NO"));
    DrawFormatString(10, 190, GetColor(255, 200, 100), _T("AnimTime: %.2f"), 
                     animationController_.GetCurrentTime());
    DrawFormatString(10, 210, GetColor(100, 255, 100), _T("Jump: %s, Dodge: %s"), 
                     isJumping_ ? _T("YES") : _T("NO"), isDodging_ ? _T("YES") : _T("NO"));
    DrawFormatString(10, 230, GetColor(100, 255, 100), _T("JumpHeight: %.2f"), jumpHeight_);
    DrawFormatString(10, 190, GetColor(255, 200, 100), _T("AnimTime: %.2f"), 
                     animationController_.GetCurrentTime());

    attack_.Draw();
}

void Player::Finalize()
{
    animationController_.Finalize();

    if (modelHandle_ >= 0)
    {
        MV1DeleteModel(modelHandle_);
        modelHandle_ = -1;
    }

    modelLoaded_ = false;
    useWalkAnimation_ = false;
    attack_.Finalize();
}

VECTOR Player::GetPosition() const
{
    return position_;
}

AttackType Player::GetCurrentAttack() const
{
    return attack_.GetCurrentAttack();
}

bool Player::IsAttacking() const
{
    return attack_.IsAttacking();
}

float Player::CalculateAnimationSpeed(int targetModelHandle, int animIndex, float targetDuration) const
{
    if (targetModelHandle < 0 || animIndex < 0 || targetDuration <= 0.0f)
    {
        return 1.0f;
    }

    const int animNum = MV1GetAnimNum(targetModelHandle);
    if (animIndex >= animNum)
    {
        return 1.0f;
    }

    const float totalTime = MV1GetAnimTotalTime(targetModelHandle, animIndex);
    if (totalTime <= 0.0f)
    {
        return 1.0f;
    }

    return totalTime / targetDuration;
}

void Player::SwitchAnimation(bool useWalkAnimation)
{
    if (!modelLoaded_)
    {
        return;
    }

    if (useWalkAnimation_ == useWalkAnimation && animationController_.IsPlaying())
    {
        return;
    }

    useWalkAnimation_ = useWalkAnimation;

    const int animIndex = useWalkAnimation_ ? kWalkAnimIndex : kIdleAnimIndex;
    const float animSpeed = useWalkAnimation_ ? walkAnimSpeed_ : idleAnimSpeed_;

    animationController_.StopAnimation();
    animationController_.Initialize(modelHandle_);
    animationController_.PlayAnimation(animIndex, animSpeed, true, 0.0f, 0.0f);  // ループアニメーション
}

void Player::UpdateAttackAnimation()
{
    if (!modelLoaded_ || comboStep_ == 0)
    {
        return;
    }

    // 現在のセグメントが終了したか確認
    if (!animationController_.IsPlaying())
    {
        TCHAR debugMsg[256];
        _stprintf_s(debugMsg, 256, _T("[UpdateAttack] Segment ended: step=%d, pending=%d\n"), comboStep_, pendingCombo_);
        OutputDebugString(debugMsg);

        if (pendingCombo_ && comboStep_ < 3)
        {
            // 次のコンボ段へ
            OutputDebugString(_T("[UpdateAttack] Starting next combo segment\n"));
            pendingCombo_ = false;
            PlayComboSegment(comboStep_);
        }
        else
        {
            // コンボ終了
            OutputDebugString(_T("[UpdateAttack] Combo finished\n"));
            comboStep_ = 0;
            pendingCombo_ = false;
            attack_.CancelAttack();
            SwitchAnimation(false);
        }
    }
}

void Player::PlayComboSegment(int step)
{
    animationController_.StopAnimation();
    animationController_.Initialize(modelHandle_);

    // 各段のアニメーション時間を手動で設定
    // step 0 = 1段目, step 1 = 2段目, step 2 = 3段目
    float start = 0.0f;
    float end = 0.0f;
    float targetDuration = 0.5f;  // デフォルトの再生時間

    if (step == 0)
    {
        // 1段目: 開始〜終了の時間（秒）
        start = 0.0f;
        end = 35.0f;  // ← 実際の値に調整してください
        targetDuration = 0.5f;  // 0.5秒で再生
    }
    else if (step == 1)
    {
        // 2段目: 開始〜終了の時間（秒）
        start = 35.0f;  // ← 実際の値に調整してください
        end = 45.0f;    // ← 実際の値に調整してください
        targetDuration = 0.25f;  // 0.25秒で再生
    }
    else if (step == 2)
    {
        // 3段目: 開始〜終了の時間（秒）
        start = 45.0f;   // ← 実際の値に調整してください
        end = 105.0f;    // ← 実際の値に調整してください
        targetDuration = 0.7f;  // 0.7 秒で再生
    }

    const float segmentLength = end - start;
    const float speed = (targetDuration > 0.0f && segmentLength > 0.0f) 
                       ? (segmentLength / targetDuration) 
                       : 1.0f;

    animationController_.PlayAnimation(kAttackAnimIndex, speed, false, start, end);
    comboStep_ = step + 1;

    // 攻撃状態をリセットして再セット（タイマーを段ごとに管理）
    attack_.ExecuteWeakAttack();

    TCHAR debugMsg[256];
    _stprintf_s(debugMsg, 256, _T("[PlayComboSegment] step=%d, start=%.2f, end=%.2f, speed=%.2f, targetDur=%.2f\n"), 
                step, start, end, speed, targetDuration);
    OutputDebugString(debugMsg);
}

void Player::PlayActionAnimation(int animIndex, float duration)
{
    animationController_.StopAnimation();
    animationController_.Initialize(modelHandle_);

    // アニメーション全体の長さを取得
    const float totalTime = MV1GetAnimTotalTime(modelHandle_, animIndex);

    // 指定時間で再生するための速度を計算
    const float speed = (duration > 0.0f && totalTime > 0.0f) ? (totalTime / duration) : 1.0f;

    animationController_.PlayAnimation(animIndex, speed, false, 0.0f, totalTime);

    TCHAR debugMsg[256];
    _stprintf_s(debugMsg, 256, _T("[PlayActionAnimation] animIndex=%d, totalTime=%.2f, duration=%.2f, speed=%.2f\n"), 
                animIndex, totalTime, duration, speed);
    OutputDebugString(debugMsg);
}