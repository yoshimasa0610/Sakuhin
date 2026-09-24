#include "Player.h"

#include <cmath>
#include <tchar.h>

namespace
{
    // モデル表示と向きの基準
    constexpr float kModelScale = 1.0f;
    constexpr float kDefaultRotationY = 3.14159f;

    // 接地判定と床範囲
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

    // 行動中の微移動量
    constexpr float kDodgeMoveScale = 0.32f;
    constexpr float kAttackMoveScale = 0.18f;

    // 回避後の回避攻撃受付時間
    constexpr float kDodgeAttackGraceDuration = 1.0f;

    // 攻撃後隙
    constexpr float kNormalAttackRecoveryDuration = 0.18f;
    constexpr float kDodgeAttackRecoveryDuration = 0.30f;
}

// コンストラクタ：初期値設定
Player::Player()
    : position_(VGet(0.0f, 0.0f, 0.0f))
    , moveSpeed_(4.0f)
    , modelRotationY_(kDefaultRotationY)
    , modelHandle_(-1)
    , modelLoaded_(false)
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
    , isAirAttackLocked_(false)
    , airAttackLockPosition_(VGet(0.0f, 0.0f, 0.0f))
    , canDodgeAttack_(false)
    , dodgeAttackGraceTimer_(0.0f)
    , attackRecoveryTimer_(0.0f)
    , pendingDodgeAttackRecovery_(false)
    , queuedDodgeAttack_(false)
    , previousKeyInput_(0)
{
}

// プレイヤー状態の初期化
void Player::Initialize()
{
    position_ = VGet(0.0f, 0.0f, 0.0f);
    modelRotationY_ = kDefaultRotationY;
    attack_.Initialize();
    playerAnimation_.Initialize();
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
    isAirAttackLocked_ = false;
    airAttackLockPosition_ = VGet(0.0f, 0.0f, 0.0f);
    canDodgeAttack_ = false;
    dodgeAttackGraceTimer_ = 0.0f;
    attackRecoveryTimer_ = 0.0f;
    pendingDodgeAttackRecovery_ = false;
    queuedDodgeAttack_ = false;
    previousMouseInput_ = 0;
    previousKeyInput_ = 0;
}

// モデル読み込み
bool Player::LoadModel(const TCHAR* modelPath)
{
    Finalize();

    modelHandle_ = MV1LoadModel(modelPath);
    if (modelHandle_ < 0)
    {
        modelLoaded_ = false;
        return false;
    }

    if (!playerAnimation_.BindModel(modelHandle_))
    {
        MV1DeleteModel(modelHandle_);
        modelHandle_ = -1;
        modelLoaded_ = false;
        return false;
    }

    MV1SetScale(modelHandle_, VGet(kModelScale, kModelScale, kModelScale));
    MV1SetPosition(modelHandle_, position_);
    MV1SetRotationXYZ(modelHandle_, VGet(0.0f, modelRotationY_, 0.0f));

    modelLoaded_ = true;
    playerAnimation_.SwitchAnimation(false, modelLoaded_);
    return true;
}

// 毎フレーム更新（入力・物理・アニメ・モデル同期）
void Player::Update(float cameraYaw)
{
    const float deltaTime = 1.0f / 60.0f;
    bool animationUpdatedInAction = false;
    bool airPhysicsApplied = false;
    bool landedThisFrame = false;
    const bool wasAttackingAtFrameStart = attack_.IsAttacking() || comboStep_ > 0;

    if (attackRecoveryTimer_ > 0.0f)
    {
        attackRecoveryTimer_ -= deltaTime;
        if (attackRecoveryTimer_ < 0.0f)
        {
            attackRecoveryTimer_ = 0.0f;
        }
    }

    if (canDodgeAttack_)
    {
        dodgeAttackGraceTimer_ -= deltaTime;
        if (dodgeAttackGraceTimer_ <= 0.0f)
        {
            canDodgeAttack_ = false;
            dodgeAttackGraceTimer_ = 0.0f;
            queuedDodgeAttack_ = false;
        }
    }

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
        const int jumpAnimIndex = playerAnimation_.GetJumpAnimIndex();
        const float jumpTotal = playerAnimation_.GetAnimTotalTime(jumpAnimIndex);
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

        // 落下区間を指定時間で再生
        playerAnimation_.PlaySegment(jumpAnimIndex, jumpHalf, fallStop, kJumpFallAnimDuration, false);
        isFallingAnimActive_ = true;
    };

    // 30F保持から着地残りモーションを再生
    auto PlayJumpLandingFromHold = [this]() -> bool
    {
        const int jumpAnimIndex = playerAnimation_.GetJumpAnimIndex();
        const float jumpTotal = playerAnimation_.GetAnimTotalTime(jumpAnimIndex);
        const float landingStart = (kJumpFallHoldFrame < jumpTotal) ? kJumpFallHoldFrame : jumpTotal;
        const float landingLength = jumpTotal - landingStart;
        if (landingLength <= 0.0f)
        {
            return false;
        }

        // 30Fから終端までを短時間で再生
        return playerAnimation_.PlaySegment(jumpAnimIndex, landingStart, jumpTotal, kJumpLandingAnimDuration, false);
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
            // 空中攻撃中は位置固定し、落下させない
            if (isAirAttackLocked_)
            {
                verticalVelocity_ = 0.0f;
                position_ = airAttackLockPosition_;
            }
            else
            {
                ApplyAirPhysics();
                airPhysicsApplied = true;

                // 落下アニメは非攻撃時のみ開始する
                if (!isGrounded_
                    && verticalVelocity_ < -1.0f
                    && !isFallingAnimActive_
                    && !attack_.IsAttacking()
                    && comboStep_ == 0
                    && modelHandle_ >= 0)
                {
                    PlayJumpFallHalf();
                }

                ClampToGround();

                if (isGrounded_)
                {
                    actionTimer_ = 0.0f;
                }
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
                canDodgeAttack_ = true;
                dodgeAttackGraceTimer_ = kDodgeAttackGraceDuration;
                bool startedDodgeAttack = false;

                if (queuedDodgeAttack_
                    && attackRecoveryTimer_ <= 0.0f
                    && !attack_.IsAttacking()
                    && comboStep_ == 0
                    && !isJumping_)
                {
                    queuedDodgeAttack_ = false;
                    canDodgeAttack_ = false;
                    dodgeAttackGraceTimer_ = 0.0f;
                    pendingCombo_ = false;
                    isAirAttackLocked_ = false;
                    pendingDodgeAttackRecovery_ = true;

                    attack_.ExecuteStrongAttack();
                    playerAnimation_.PlayDodgeAttack();
                    startedDodgeAttack = true;
                }

                if (isGrounded_ && !startedDodgeAttack)
                {
                    playerAnimation_.SwitchAnimation(false, modelLoaded_);
                }
            }
        }

        playerAnimation_.Update();
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
    if (CheckHitKey(KEY_INPUT_LSHIFT) || CheckHitKey(KEY_INPUT_RSHIFT)) currentKeyInput |= 32;

    const bool spacePressed = (currentKeyInput & 1) && !(previousKeyInput_ & 1);
    const bool hasMoveKeyInput = (currentKeyInput & (2 | 4 | 8 | 16)) != 0;
    const bool shiftPressed = (currentKeyInput & 32) && !(previousKeyInput_ & 32);

    int currentAttackInput = 0;
    if ((GetMouseInput() & MOUSE_INPUT_LEFT) != 0)
    {
        currentAttackInput |= 1;
    }
    const bool attackPressed = (currentAttackInput & 1) && !(previousMouseInput_ & 1);
    const bool attackHeld = (currentAttackInput & 1) != 0;

    const bool shouldTriggerDodgeAttack =
        canDodgeAttack_
        && attackHeld
        && !isDodging_
        && !attack_.IsAttacking()
        && comboStep_ == 0
        && !isJumping_;

    // 攻撃入力とコンボ予約
    if (shouldTriggerDodgeAttack)
    {
        queuedDodgeAttack_ = false;
        canDodgeAttack_ = false;
        dodgeAttackGraceTimer_ = 0.0f;
        pendingCombo_ = false;
        comboStep_ = 0;
        isAirAttackLocked_ = false;
        pendingDodgeAttackRecovery_ = true;

        attack_.ExecuteStrongAttack();
        playerAnimation_.PlayDodgeAttack();
    }
    else if (attackPressed && attackRecoveryTimer_ <= 0.0f)
    {
        if (isDodging_)
        {
            queuedDodgeAttack_ = true;
        }
        else if (!attack_.IsAttacking() && comboStep_ == 0)
        {
            pendingDodgeAttackRecovery_ = false;
            attack_.ExecuteWeakAttack();
            playerAnimation_.PlayComboSegment(0, isGrounded_, position_, isAirAttackLocked_, airAttackLockPosition_, comboStep_, attack_);
        }
        else if (comboStep_ > 0 && comboStep_ <= 2)
        {
            pendingCombo_ = true;
        }
    }

    // 今フレームで攻撃中かどうかをまとめて判定
    const bool isAttackAnimating = attack_.IsAttacking() || comboStep_ > 0 || attackPressed;

    // 空中・行動中の微移動
    {
        // アクション中でも違和感が出ない範囲で少しだけ移動を許可
        if (isMoving && !isJumping_ && !isAirAttackLocked_ && (isDodging_ || isAttackAnimating))
        {
            move.x /= length;
            move.z /= length;

            const float actionMoveScale = isDodging_ ? kDodgeMoveScale : kAttackMoveScale;
            position_.x += move.x * moveSpeed_ * actionMoveScale;
            position_.z += move.z * moveSpeed_ * actionMoveScale;
        }
    }

    // Shift単体で後方回避、WASD入力+Shiftで前方回避
    if (shiftPressed && !isDodging_ && !attack_.IsAttacking() && comboStep_ == 0 && isGrounded_ && attackRecoveryTimer_ <= 0.0f)
    {
        queuedDodgeAttack_ = false;
        canDodgeAttack_ = false;
        dodgeAttackGraceTimer_ = 0.0f;

        if (hasMoveKeyInput)
        {
            isDodging_ = true;
            actionTimer_ = 0.0f;

            // 入力方向(カメラ基準)へ向きを合わせてから前方回避する
            if (length > 0.0f)
            {
                const float moveX = move.x / length;
                const float moveZ = move.z / length;
                modelRotationY_ = static_cast<float>(std::atan2(moveX, moveZ)) + kDefaultRotationY;
            }

            if (modelHandle_ >= 0)
            {
                MV1SetPosition(modelHandle_, position_);
                MV1SetRotationXYZ(modelHandle_, VGet(0.0f, modelRotationY_, 0.0f));
            }

            // 回避開始直後の一歩を入れて、ラグ感を減らす
            if (isMoving)
            {
                const float moveX = move.x / length;
                const float moveZ = move.z / length;
                position_.x += moveX * moveSpeed_ * (kDodgeMoveScale * 1.2f);
                position_.z += moveZ * moveSpeed_ * (kDodgeMoveScale * 1.2f);
            }

            playerAnimation_.PlayActionAnimation(playerAnimation_.GetDodgeForwardAnimIndex(), 0.5f);

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
                MV1SetRotationXYZ(modelHandle_, VGet(0.0f, modelRotationY_, 0.0f));
            }

            playerAnimation_.PlayActionAnimation(playerAnimation_.GetDodgeBackAnimIndex(), 0.5f);

            previousKeyInput_ = currentKeyInput;
            return;
        }
    }

    // ジャンプ開始処理
    if (spacePressed && !attack_.IsAttacking() && comboStep_ == 0 && isGrounded_ && attackRecoveryTimer_ <= 0.0f)
    {
        isJumping_ = true;
        isGrounded_ = false;
        isFallingAnimActive_ = false;
        actionTimer_ = 0.0f;
        verticalVelocity_ = jumpStartVelocity_;
        jumpHeight_ = 0.0f;

        const int jumpAnimIndex = playerAnimation_.GetJumpAnimIndex();
        const float jumpTotal = playerAnimation_.GetAnimTotalTime(jumpAnimIndex);
        const float jumpHalf = (jumpTotal > 0.0f) ? (jumpTotal * 0.5f) : 0.0f;
        playerAnimation_.PlaySegment(jumpAnimIndex, 0.0f, jumpHalf, kJumpRiseAnimDuration, false);

        previousKeyInput_ = currentKeyInput;
    }

    // 地上・空中移動
    if (isMoving && !isShiftPressed && !isDodging_ && !isAttackAnimating && attackRecoveryTimer_ <= 0.0f)
    {
        move.x /= length;
        move.z /= length;

        position_.x += move.x * moveSpeed_;
        position_.z += move.z * moveSpeed_;

        modelRotationY_ = static_cast<float>(std::atan2(move.x, move.z)) + kDefaultRotationY;
    }

    // 非攻撃時の空中アニメ維持 / 地上アニメ復帰
    if (!attack_.IsAttacking() && comboStep_ == 0)
    {
        const int jumpAnimIndex = playerAnimation_.GetJumpAnimIndex();
        if (!isGrounded_ && !isDodging_)
        {
            if (isFallingAnimActive_
                && !playerAnimation_.IsPlaying()
                && playerAnimation_.GetCurrentAnimIndex() == jumpAnimIndex
                && modelHandle_ >= 0)
            {
                playerAnimation_.SetCurrentTime(kJumpFallHoldFrame);
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
                    playerAnimation_.SwitchAnimation(isMoving, modelLoaded_);
                }
            }
            else if (!(playerAnimation_.GetCurrentAnimIndex() == jumpAnimIndex && playerAnimation_.IsPlaying()))
            {
                playerAnimation_.SwitchAnimation(isMoving, modelLoaded_);
            }
        }
    }

    // 入力履歴更新
    previousMouseInput_ = currentAttackInput;
    previousKeyInput_ = currentKeyInput;

    // サブシステム更新
    attack_.Update();
    playerAnimation_.UpdateAttackAnimation(modelLoaded_, modelHandle_, position_, isGrounded_, isFallingAnimActive_, isAirAttackLocked_, airAttackLockPosition_, comboStep_, pendingCombo_, attack_);
    if (!animationUpdatedInAction)
    {
        playerAnimation_.Update();
    }

    // 攻撃後隙タイマー更新
    const bool isAttackingAfterUpdate = attack_.IsAttacking() || comboStep_ > 0;
    if (wasAttackingAtFrameStart && !isAttackingAfterUpdate)
    {
        attackRecoveryTimer_ = pendingDodgeAttackRecovery_ ? kDodgeAttackRecoveryDuration : kNormalAttackRecoveryDuration;
        pendingDodgeAttackRecovery_ = false;
    }

    // アクション外での空中物理
    if (!isGrounded_ && !airPhysicsApplied)
    {
        if (!isAirAttackLocked_)
        {
            ApplyAirPhysics();
            ClampToGround();
        }
        else
        {
            verticalVelocity_ = 0.0f;
            position_ = airAttackLockPosition_;
        }
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
                playerAnimation_.SwitchAnimation(isMoving, modelLoaded_);
            }
        }
    }

    // 着地モーション完了後に通常アニメへ復帰
    if (isGrounded_
        && !isFallingAnimActive_
        && !isJumping_
        && !attack_.IsAttacking() && comboStep_ == 0)
    {
        const int jumpAnimIndex = playerAnimation_.GetJumpAnimIndex();
        if (playerAnimation_.GetCurrentAnimIndex() == jumpAnimIndex
            && !playerAnimation_.IsPlaying())
        {
            playerAnimation_.SwitchAnimation(isMoving, modelLoaded_);
        }
    }

    // ルートモーションと物理座標の同期
    if (modelHandle_ >= 0)
    {
        if (isAttackAnimating)
        {
            if (isAirAttackLocked_)
            {
                position_ = airAttackLockPosition_;
                MV1SetPosition(modelHandle_, airAttackLockPosition_);
            }
            else
            {
                if (isGrounded_)
                {
                    ClampToGround();
                }

                MV1SetPosition(modelHandle_, position_);
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
    playerAnimation_.Finalize();

    if (modelHandle_ >= 0)
    {
        MV1DeleteModel(modelHandle_);
        modelHandle_ = -1;
    }

    modelLoaded_ = false;
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