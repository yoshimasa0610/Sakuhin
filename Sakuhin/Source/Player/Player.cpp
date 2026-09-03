#include "Player.h"
#include <cmath>
#include <tchar.h>

namespace
{
    // モデル表示と向きの基準
    constexpr float kModelScale = 1.0f;
    constexpr float kDefaultRotationY = 3.14159f;

    // デバッグ床の高さと範囲（床判定にも使用）
    constexpr float kGroundY = 0.0f;
    constexpr float kFloorMinX = -3000.0f;
    constexpr float kFloorMaxX = 3000.0f;
    constexpr float kFloorMinZ = -3000.0f;
    constexpr float kFloorMaxZ = 3000.0f;

    // 空中挙動
    constexpr float kFallingGravityScale = 0.15f;
    constexpr float kJumpRiseAnimDuration = 0.35f;
    constexpr float kJumpFallAnimDuration = 0.70f;
    constexpr float kJumpLandingAnimDuration = 0.20f;
    constexpr float kJumpFallHoldFrame = 30.0f;

    // 既知アニメーション番号（モデル差し替え時の基準値）
    constexpr int kKnownIdleAnimIndex = 10;
    constexpr int kKnownJumpAnimIndex = 9;
    constexpr int kKnownDodgeBackAnimIndex = 4;
    constexpr int kKnownDodgeForwardAnimIndex = 5;

    // 文字列にキーワードが含まれているかを大文字小文字無視で判定
    bool ContainsIgnoreCase(const TCHAR* text, const TCHAR* token)
    {
        if (text == nullptr || token == nullptr)
        {
            return false;
        }

        const int textLen = static_cast<int>(_tcslen(text));
        const int tokenLen = static_cast<int>(_tcslen(token));
        if (tokenLen <= 0 || textLen < tokenLen)
        {
            return false;
        }

        for (int i = 0; i <= textLen - tokenLen; ++i)
        {
            bool match = true;
            for (int j = 0; j < tokenLen; ++j)
            {
                if (_totlower(text[i + j]) != _totlower(token[j]))
                {
                    match = false;
                    break;
                }
            }

            if (match)
            {
                return true;
            }
        }

        return false;
    }
}

// コンストラクタ：初期値設定
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
    , attackAnimIndex_(0)
    , walkAnimIndex_(1)
    , idleAnimIndex_(10)
    , jumpAnimIndex_(9)
    , dodgeBackAnimIndex_(4)
    , dodgeForwardAnimIndex_(5)
    , previousMouseInput_(0)
    , comboStep_(0)
    , pendingCombo_(false)
    , isJumping_(false)
    , isDodging_(false)
    , isFallingAnimActive_(false)
    , actionTimer_(0.0f)
    , jumpHeight_(0.0f)
    , verticalVelocity_(0.0f)
    , gravity_(-650.0f)
    , jumpStartVelocity_(620.0f)
    , isGrounded_(true)
    , previousKeyInput_(0)
{
}

// プレイヤー状態の初期化
void Player::Initialize()
{
    position_ = VGet(0.0f, 0.0f, 0.0f);
    modelRotationY_ = kDefaultRotationY;
    attack_.Initialize();
    animationController_.Initialize(-1);
    useWalkAnimation_ = false;
    comboStep_ = 0;
    pendingCombo_ = false;
    isJumping_ = false;
    isDodging_ = false;
    isFallingAnimActive_ = false;
    actionTimer_ = 0.0f;
    jumpHeight_ = 0.0f;
    verticalVelocity_ = 0.0f;
    gravity_ = -650.0f;
    jumpStartVelocity_ = 620.0f;
    isGrounded_ = true;
    previousKeyInput_ = 0;

    attackAnimIndex_ = 0;
    walkAnimIndex_ = 1;
    idleAnimIndex_ = 10;
    jumpAnimIndex_ = 9;
    dodgeBackAnimIndex_ = 4;
    dodgeForwardAnimIndex_ = 5;
}

// モデル読み込みとアニメーション番号解決
bool Player::LoadModel(const TCHAR* modelPath)
{
    Finalize();

    modelHandle_ = MV1LoadModel(modelPath);
    if (modelHandle_ < 0)
    {
        modelLoaded_ = false;
        totalAnimationCount_ = 0;
        return false;
    }

    // モデルに含まれるアニメーション数
    totalAnimationCount_ = MV1GetAnimNum(modelHandle_);

    // 使用するアニメーション番号を名前から解決
    attackAnimIndex_ = -1;
    walkAnimIndex_ = -1;
    idleAnimIndex_ = -1;
    jumpAnimIndex_ = -1;
    dodgeBackAnimIndex_ = -1;
    dodgeForwardAnimIndex_ = -1;

    int genericDodgeAnimA = -1;
    int genericDodgeAnimB = -1;

    for (int i = 0; i < totalAnimationCount_; ++i)
    {
        const TCHAR* animName = MV1GetAnimName(modelHandle_, i);
        if (animName == nullptr)
        {
            continue;
        }

        const bool isGenericDodge = ContainsIgnoreCase(animName, _T("dodge"))
            || ContainsIgnoreCase(animName, _T("evade"))
            || ContainsIgnoreCase(animName, _T("dash"))
            || ContainsIgnoreCase(animName, _T("step"));

        if (isGenericDodge)
        {
            if (genericDodgeAnimA < 0)
            {
                genericDodgeAnimA = i;
            }
            else if (genericDodgeAnimB < 0 && genericDodgeAnimA != i)
            {
                genericDodgeAnimB = i;
            }
        }

        if (attackAnimIndex_ < 0 && (ContainsIgnoreCase(animName, _T("attack")) || ContainsIgnoreCase(animName, _T("atk")) || ContainsIgnoreCase(animName, _T("slash"))))
        {
            attackAnimIndex_ = i;
        }

        if (walkAnimIndex_ < 0 && (ContainsIgnoreCase(animName, _T("walk")) || ContainsIgnoreCase(animName, _T("run")) || ContainsIgnoreCase(animName, _T("move"))))
        {
            walkAnimIndex_ = i;
        }

        if (idleAnimIndex_ < 0 && (ContainsIgnoreCase(animName, _T("idle")) || ContainsIgnoreCase(animName, _T("wait")) || ContainsIgnoreCase(animName, _T("stand"))))
        {
            idleAnimIndex_ = i;
        }

        if (jumpAnimIndex_ < 0 && (ContainsIgnoreCase(animName, _T("jump")) || ContainsIgnoreCase(animName, _T("air"))))
        {
            jumpAnimIndex_ = i;
        }

        if (dodgeBackAnimIndex_ < 0 && (ContainsIgnoreCase(animName, _T("dodge_back"))
            || ContainsIgnoreCase(animName, _T("backstep"))
            || ContainsIgnoreCase(animName, _T("evade_back"))
            || ContainsIgnoreCase(animName, _T("backward"))
            || ContainsIgnoreCase(animName, _T("rear"))
            || ContainsIgnoreCase(animName, _T("retreat"))
            || ContainsIgnoreCase(animName, _T("back"))))
        {
            dodgeBackAnimIndex_ = i;
        }

        if (dodgeForwardAnimIndex_ < 0 && (ContainsIgnoreCase(animName, _T("dodge_front"))
            || ContainsIgnoreCase(animName, _T("dodge_forward"))
            || ContainsIgnoreCase(animName, _T("frontstep"))
            || ContainsIgnoreCase(animName, _T("forward"))
            || ContainsIgnoreCase(animName, _T("front"))
            || ContainsIgnoreCase(animName, _T("rush"))))
        {
            dodgeForwardAnimIndex_ = i;
        }
    }

    auto isValidIndex = [this](int index) { return index >= 0 && index < totalAnimationCount_; };

    if (!isValidIndex(attackAnimIndex_)) attackAnimIndex_ = isValidIndex(0) ? 0 : -1;
    if (!isValidIndex(walkAnimIndex_)) walkAnimIndex_ = isValidIndex(1) ? 1 : attackAnimIndex_;
    if (!isValidIndex(idleAnimIndex_)) idleAnimIndex_ = isValidIndex(kKnownIdleAnimIndex) ? kKnownIdleAnimIndex : walkAnimIndex_;
    if (!isValidIndex(jumpAnimIndex_)) jumpAnimIndex_ = isValidIndex(kKnownJumpAnimIndex) ? kKnownJumpAnimIndex : idleAnimIndex_;
    if (!isValidIndex(dodgeBackAnimIndex_)) dodgeBackAnimIndex_ = isValidIndex(kKnownDodgeBackAnimIndex) ? kKnownDodgeBackAnimIndex : idleAnimIndex_;
    if (!isValidIndex(dodgeForwardAnimIndex_)) dodgeForwardAnimIndex_ = isValidIndex(kKnownDodgeForwardAnimIndex) ? kKnownDodgeForwardAnimIndex : walkAnimIndex_;

    if (isValidIndex(kKnownIdleAnimIndex)) idleAnimIndex_ = kKnownIdleAnimIndex;
    if (isValidIndex(kKnownJumpAnimIndex)) jumpAnimIndex_ = kKnownJumpAnimIndex;
    if (isValidIndex(kKnownDodgeBackAnimIndex)) dodgeBackAnimIndex_ = kKnownDodgeBackAnimIndex;
    if (isValidIndex(kKnownDodgeForwardAnimIndex)) dodgeForwardAnimIndex_ = kKnownDodgeForwardAnimIndex;

    if (dodgeBackAnimIndex_ == dodgeForwardAnimIndex_)
    {
        if (isValidIndex(genericDodgeAnimA) && isValidIndex(genericDodgeAnimB) && genericDodgeAnimA != genericDodgeAnimB)
        {
            dodgeForwardAnimIndex_ = genericDodgeAnimA;
            dodgeBackAnimIndex_ = genericDodgeAnimB;
        }
    }

    MV1SetScale(modelHandle_, VGet(kModelScale, kModelScale, kModelScale));
    MV1SetPosition(modelHandle_, position_);
    MV1SetRotationXYZ(modelHandle_, VGet(0.0f, modelRotationY_, 0.0f));

    walkAnimSpeed_ = CalculateAnimationSpeed(modelHandle_, walkAnimIndex_, 1.5f);
    idleAnimSpeed_ = CalculateAnimationSpeed(modelHandle_, idleAnimIndex_, 4.0f);

    modelLoaded_ = true;
    SwitchAnimation(false);
    return true;
}

// 毎フレーム更新（入力・物理・アニメ・モデル同期）
void Player::Update(float cameraYaw)
{
    const float deltaTime = 1.0f / 60.0f;
    bool animationUpdatedInAction = false;
    bool airPhysicsApplied = false;
    bool landedThisFrame = false;

    // カメラ向きに合わせてプレイヤーの向きを更新（回避中は維持）
    if (!isDodging_)
    {
        modelRotationY_ = cameraYaw + kDefaultRotationY;
    }

    // 接地判定と着地イベント更新
    auto ClampToGround = [this, &landedThisFrame]()
    {
        const bool onFloorArea = (position_.x >= kFloorMinX && position_.x <= kFloorMaxX
            && position_.z >= kFloorMinZ && position_.z <= kFloorMaxZ);

        const bool wasGrounded = isGrounded_;
        if (onFloorArea && position_.y <= kGroundY)
        {
            position_.y = kGroundY;
            verticalVelocity_ = 0.0f;
            isGrounded_ = true;
            jumpHeight_ = 0.0f;
            isJumping_ = false;
            if (!wasGrounded)
            {
                landedThisFrame = true;
            }
        }
        else
        {
            isGrounded_ = false;
            jumpHeight_ = (position_.y > kGroundY) ? (position_.y - kGroundY) : 0.0f;
        }
    };

    // ジャンプ後半（落下区間）を再生して30F付近で停止
    auto PlayJumpFallHalf = [this]()
    {
        if (modelHandle_ < 0)
        {
            return;
        }

        const float jumpTotal = MV1GetAnimTotalTime(modelHandle_, jumpAnimIndex_);
        const float jumpHalf = (jumpTotal > 0.0f) ? (jumpTotal * 0.5f) : 0.0f;
        float fallStop = kJumpFallHoldFrame;
        if (fallStop < jumpHalf)
        {
            fallStop = jumpHalf;
        }
        if (fallStop > jumpTotal)
        {
            fallStop = jumpTotal;
        }

        const float segmentLength = fallStop - jumpHalf;
        const float speed = (segmentLength > 0.0f && kJumpFallAnimDuration > 0.0f)
            ? (segmentLength / kJumpFallAnimDuration)
            : 1.0f;

        animationController_.StopAnimation();
        animationController_.Initialize(modelHandle_);
        animationController_.PlayAnimation(jumpAnimIndex_, speed, false, jumpHalf, fallStop);
        isFallingAnimActive_ = true;
    };

    // 30F保持から着地残りモーションを再生
    auto PlayJumpLandingFromHold = [this]() -> bool
    {
        if (modelHandle_ < 0)
        {
            return false;
        }

        const float jumpTotal = MV1GetAnimTotalTime(modelHandle_, jumpAnimIndex_);
        const float landingStart = (kJumpFallHoldFrame < jumpTotal) ? kJumpFallHoldFrame : jumpTotal;
        const float landingLength = jumpTotal - landingStart;
        if (landingLength <= 0.0f)
        {
            return false;
        }

        const float speed = (kJumpLandingAnimDuration > 0.0f)
            ? (landingLength / kJumpLandingAnimDuration)
            : 1.0f;

        animationController_.StopAnimation();
        animationController_.Initialize(modelHandle_);
        animationController_.PlayAnimation(jumpAnimIndex_, speed, false, landingStart, jumpTotal);
        return true;
    };

    // 空中物理（上昇/落下）
    auto ApplyAirPhysics = [this, deltaTime]()
    {
        const float gravityScale = (verticalVelocity_ < 0.0f) ? kFallingGravityScale : 1.0f;
        verticalVelocity_ += gravity_ * gravityScale * deltaTime;
        position_.y += verticalVelocity_ * deltaTime;
    };

    // 床範囲外なら落下開始
    if (isGrounded_)
    {
        const bool onFloorArea = (position_.x >= kFloorMinX && position_.x <= kFloorMaxX
            && position_.z >= kFloorMinZ && position_.z <= kFloorMaxZ);
        if (!onFloorArea)
        {
            isGrounded_ = false;
        }
    }

    // ジャンプ中・回避中のアクション処理
    if (isJumping_ || isDodging_)
    {
        actionTimer_ += deltaTime;

        if (isJumping_)
        {
            ApplyAirPhysics();
            airPhysicsApplied = true;

            if (!isGrounded_ && verticalVelocity_ < -1.0f && !isFallingAnimActive_ && modelHandle_ >= 0)
            {
                PlayJumpFallHalf();
            }

            ClampToGround();

            if (isGrounded_)
            {
                actionTimer_ = 0.0f;
            }
        }
        else if (isDodging_)
        {
            const float animDuration = 0.5f;

            if (modelHandle_ >= 0)
            {
                position_ = MV1GetPosition(modelHandle_);
                ClampToGround();
            }

            if (actionTimer_ >= animDuration)
            {
                isDodging_ = false;
                actionTimer_ = 0.0f;
                if (isGrounded_)
                {
                    SwitchAnimation(false);
                }
            }
        }

        animationController_.Update();
        animationUpdatedInAction = true;

        // モデル位置の設定
        if (modelHandle_ >= 0)
        {
            if (isJumping_)
            {
                MV1SetPosition(modelHandle_, position_);
                MV1SetRotationXYZ(modelHandle_, VGet(0.0f, modelRotationY_, 0.0f));
            }
            else if (isDodging_)
            {
                MV1SetRotationXYZ(modelHandle_, VGet(0.0f, modelRotationY_, 0.0f));
                return;
            }
        }
    }

    // 通常移動入力（カメラ基準）
    VECTOR move = VGet(0.0f, 0.0f, 0.0f);
    const bool isShiftPressed = CheckHitKey(KEY_INPUT_LSHIFT) || CheckHitKey(KEY_INPUT_RSHIFT);

    const VECTOR cameraForward = VGet(std::sin(cameraYaw), 0.0f, std::cos(cameraYaw));
    const VECTOR cameraRight = VGet(std::cos(cameraYaw), 0.0f, -std::sin(cameraYaw));

    if (CheckHitKey(KEY_INPUT_W))
    {
        move.x += cameraForward.x;
        move.z += cameraForward.z;
    }
    if (CheckHitKey(KEY_INPUT_S))
    {
        move.x -= cameraForward.x;
        move.z -= cameraForward.z;
    }
    if (CheckHitKey(KEY_INPUT_A))
    {
        move.x -= cameraRight.x;
        move.z -= cameraRight.z;
    }
    if (CheckHitKey(KEY_INPUT_D))
    {
        move.x += cameraRight.x;
        move.z += cameraRight.z;
    }

    const float length = std::sqrt(move.x * move.x + move.z * move.z);
    const bool isMoving = (length > 0.0f);

    // ジャンプ入力
    int currentKeyInput = 0;
    if (CheckHitKey(KEY_INPUT_SPACE)) currentKeyInput |= 1;
    if (CheckHitKey(KEY_INPUT_W)) currentKeyInput |= 2;
    if (CheckHitKey(KEY_INPUT_S)) currentKeyInput |= 4;
    if (CheckHitKey(KEY_INPUT_A)) currentKeyInput |= 8;
    if (CheckHitKey(KEY_INPUT_D)) currentKeyInput |= 16;

    const bool spacePressed = (currentKeyInput & 1) && !(previousKeyInput_ & 1);

    // Shift単体で後方回避、WASD+Shiftで前方回避
    if (isShiftPressed && !attack_.IsAttacking() && comboStep_ == 0 && isGrounded_)
    {
        if (isMoving)
        {
            isDodging_ = true;
            actionTimer_ = 0.0f;

            if (modelHandle_ >= 0)
            {
                MV1SetPosition(modelHandle_, position_);
            }

            PlayActionAnimation(dodgeForwardAnimIndex_, 0.5f);

            previousKeyInput_ = currentKeyInput;
            return;
        }
        else
        {
            isDodging_ = true;
            actionTimer_ = 0.0f;

            if (modelHandle_ >= 0)
            {
                MV1SetPosition(modelHandle_, position_);
            }

            PlayActionAnimation(dodgeBackAnimIndex_, 0.5f);

            previousKeyInput_ = currentKeyInput;
            return;
        }
    }

    // ジャンプ開始処理
    if (spacePressed && !attack_.IsAttacking() && comboStep_ == 0 && isGrounded_)
    {
        isJumping_ = true;
        isGrounded_ = false;
        isFallingAnimActive_ = false;
        actionTimer_ = 0.0f;
        verticalVelocity_ = jumpStartVelocity_;
        jumpHeight_ = 0.0f;

        if (modelHandle_ >= 0)
        {
            const float jumpTotal = MV1GetAnimTotalTime(modelHandle_, jumpAnimIndex_);
            const float jumpHalf = (jumpTotal > 0.0f) ? (jumpTotal * 0.5f) : 0.0f;
            const float segmentLength = jumpHalf;
            const float speed = (segmentLength > 0.0f && kJumpRiseAnimDuration > 0.0f)
                ? (segmentLength / kJumpRiseAnimDuration)
                : 1.0f;

            animationController_.StopAnimation();
            animationController_.Initialize(modelHandle_);
            animationController_.PlayAnimation(jumpAnimIndex_, speed, false, 0.0f, jumpHalf);
        }

        previousKeyInput_ = currentKeyInput;
    }

    // 地上移動
    if (isMoving && !isShiftPressed && !isJumping_)
    {
        move.x /= length;
        move.z /= length;

        position_.x += move.x * moveSpeed_;
        position_.z += move.z * moveSpeed_;

        modelRotationY_ = static_cast<float>(std::atan2(move.x, move.z)) + kDefaultRotationY;
    }

    // 攻撃入力とコンボ予約
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

    // 非攻撃時の空中アニメ維持 / 地上アニメ復帰
    if (!attack_.IsAttacking() && comboStep_ == 0)
    {
        if (!isGrounded_ && !isDodging_)
        {
            if (isFallingAnimActive_
                && !animationController_.IsPlaying()
                && animationController_.GetCurrentAnimIndex() == jumpAnimIndex_
                && modelHandle_ >= 0)
            {
                animationController_.SetCurrentTime(kJumpFallHoldFrame);
            }
        }
        else
        {
            if (isFallingAnimActive_)
            {
                if (PlayJumpLandingFromHold())
                {
                    isFallingAnimActive_ = false;
                }
                else
                {
                    isFallingAnimActive_ = false;
                    SwitchAnimation(isMoving);
                }
            }
            else if (!(animationController_.GetCurrentAnimIndex() == jumpAnimIndex_ && animationController_.IsPlaying()))
            {
                SwitchAnimation(isMoving);
            }
        }
    }

    // 入力履歴更新
    previousMouseInput_ = currentAttackInput;
    previousKeyInput_ = currentKeyInput;

    // サブシステム更新
    attack_.Update();
    UpdateAttackAnimation();
    if (!animationUpdatedInAction)
    {
        animationController_.Update();
    }

    // アクション外での空中物理
    if (!isGrounded_ && !airPhysicsApplied)
    {
        ApplyAirPhysics();
        ClampToGround();
    }

    // 着地イベント時のアニメ遷移
    if (landedThisFrame && !attack_.IsAttacking() && comboStep_ == 0 && !isDodging_)
    {
        if (isFallingAnimActive_)
        {
            if (PlayJumpLandingFromHold())
            {
                isFallingAnimActive_ = false;
            }
            else
            {
                isFallingAnimActive_ = false;
                SwitchAnimation(isMoving);
            }
        }
    }

    // 着地モーション完了後に通常アニメへ復帰
    if (isGrounded_
        && !isFallingAnimActive_
        && !isJumping_
        && !attack_.IsAttacking() && comboStep_ == 0
        && animationController_.GetCurrentAnimIndex() == jumpAnimIndex_
        && !animationController_.IsPlaying())
    {
        SwitchAnimation(isMoving);
    }

    // ルートモーションと物理座標の同期
    if (modelHandle_ >= 0)
    {
        const bool isAttackAnimating = attack_.IsAttacking() || comboStep_ > 0;
        if (isAttackAnimating)
        {
            VECTOR modelPos = MV1GetPosition(modelHandle_);
            position_.x = modelPos.x;
            position_.z = modelPos.z;

            if (isGrounded_)
            {
                position_.y = modelPos.y;
                ClampToGround();
            }
            else
            {
                modelPos.y = position_.y;
                MV1SetPosition(modelHandle_, modelPos);
            }
        }
        else
        {
            MV1SetPosition(modelHandle_, position_);
        }

        MV1SetRotationXYZ(modelHandle_, VGet(0.0f, modelRotationY_, 0.0f));
    }
}

// 描画
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

    attack_.Draw();
}

// 終了処理
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

// 位置取得
VECTOR Player::GetPosition() const
{
    return position_;
}

// 攻撃状態取得
AttackType Player::GetCurrentAttack() const
{
    return attack_.GetCurrentAttack();
}

// 攻撃中判定
bool Player::IsAttacking() const
{
    return attack_.IsAttacking();
}

// アニメ速度計算
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

// 待機/移動アニメ切り替え
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

    const int animIndex = useWalkAnimation_ ? walkAnimIndex_ : idleAnimIndex_;
    const float animSpeed = useWalkAnimation_ ? walkAnimSpeed_ : idleAnimSpeed_;

    animationController_.StopAnimation();
    animationController_.Initialize(modelHandle_);
    animationController_.PlayAnimation(animIndex, animSpeed, true, 0.0f, 0.0f);  // ループアニメーション
}

// 攻撃コンボアニメ更新
void Player::UpdateAttackAnimation()
{
    if (!modelLoaded_ || comboStep_ == 0)
    {
        return;
    }

    // 現在のセグメントが終了したか確認
    if (!animationController_.IsPlaying())
    {
        if (pendingCombo_ && comboStep_ < 3)
        {
            // 次のコンボ段へ
            pendingCombo_ = false;
            PlayComboSegment(comboStep_);
        }
        else
        {
            // コンボ終了
            if (modelHandle_ >= 0)
            {
                position_ = MV1GetPosition(modelHandle_);
            }
            comboStep_ = 0;
            pendingCombo_ = false;
            attack_.CancelAttack();

            if (isGrounded_)
            {
                isFallingAnimActive_ = false;
                SwitchAnimation(false);
            }
            else
            {
                const float jumpTotal = MV1GetAnimTotalTime(modelHandle_, jumpAnimIndex_);
                const float jumpHalf = (jumpTotal > 0.0f) ? (jumpTotal * 0.5f) : 0.0f;
                const float segmentLength = jumpTotal - jumpHalf;
                const float speed = (segmentLength > 0.0f && kJumpFallAnimDuration > 0.0f)
                    ? (segmentLength / kJumpFallAnimDuration)
                    : 1.0f;
                animationController_.StopAnimation();
                animationController_.Initialize(modelHandle_);
                animationController_.PlayAnimation(jumpAnimIndex_, speed, false, jumpHalf, jumpTotal);
                isFallingAnimActive_ = true;
            }
        }
    }
}

// コンボ段の再生区間設定
void Player::PlayComboSegment(int step)
{
    animationController_.StopAnimation();
    animationController_.Initialize(modelHandle_);

    float start = 0.0f;
    float end = 0.0f;
    float targetDuration = 0.5f;

    if (step == 0)
    {
        start = 0.0f;
        end = 35.0f;
        targetDuration = 0.5f;
    }
    else if (step == 1)
    {
        start = 35.0f;
        end = 45.0f;
        targetDuration = 0.25f;
    }
    else if (step == 2)
    {
        start = 45.0f;
        end = 105.0f;
        targetDuration = 0.7f;
    }

    const float segmentLength = end - start;
    const float speed = (targetDuration > 0.0f && segmentLength > 0.0f)
        ? (segmentLength / targetDuration)
        : 1.0f;

    animationController_.PlayAnimation(attackAnimIndex_, speed, false, start, end);
    comboStep_ = step + 1;

    attack_.ExecuteWeakAttack();
}

// 単発アクション再生
void Player::PlayActionAnimation(int animIndex, float duration)
{
    if (modelHandle_ < 0)
    {
        return;
    }

    const int animCount = MV1GetAnimNum(modelHandle_);
    if (animIndex < 0 || animIndex >= animCount)
    {
        return;
    }

    animationController_.StopAnimation();
    animationController_.Initialize(modelHandle_);

    const float totalTime = MV1GetAnimTotalTime(modelHandle_, animIndex);
    const float speed = (duration > 0.0f && totalTime > 0.0f) ? (totalTime / duration) : 1.0f;

    animationController_.PlayAnimation(animIndex, speed, false, 0.0f, totalTime);
}