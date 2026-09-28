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
    constexpr float kFallingGravityScale = 0.30f;
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

    // 三段目コンボの当たり判定遅延（3段目再生中に確実に当たり判定が出る値）
    constexpr float kThirdComboHitDelay = 0.50f;

    // 左クリック長押しでエリアル始動攻撃を出す閾値
    constexpr float kAerialStarterHoldThreshold = 0.30f;

    // 空中攻撃時のふわふわ挙動
    constexpr float kAirAttackRiseGravityScale = 0.42f;
    constexpr float kAirAttackFallGravityScale = 0.12f;
    constexpr float kAirAttackLiftVelocity = 180.0f;
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
    , attackHoldTimer_(0.0f)
    , wasAttackHeld_(false)
    , longPressAttackTriggered_(false)
    , isAerialStarterAttack_(false)
    , isFollowingAerialTarget_(false)
    , aerialTargetY_(0.0f)
    , comboStepElapsedTime_(0.0f)
    , prevComboStepForHitbox_(0)
    , prevComboStepForAirFloat_(0)
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
    attackHoldTimer_ = 0.0f;
    wasAttackHeld_ = false;
    longPressAttackTriggered_ = false;
    isAerialStarterAttack_ = false;
    isFollowingAerialTarget_ = false;
    aerialTargetY_ = 0.0f;
    comboStepElapsedTime_ = 0.0f;
    prevComboStepForHitbox_ = 0;
    prevComboStepForAirFloat_ = 0;
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

    // ヘルメット貫通対策：頭部に関連するフレームを非表示化する
    // ※ 立方体/白板はゲーム内で使うため、ここでは非表示にしない
    {
        const TCHAR* hiddenHeadFrameCandidates[] =
        {
            _T("mixamorig_Head"),
            _T("mixamorig_Neck"),
            _T("mixamorig_LeftEye"),
            _T("mixamorig_RightEye"),
            _T("mixamorig_HeadTop_End"),
            _T("Head"),
            _T("head")
        };

        for (const auto& frameName : hiddenHeadFrameCandidates)
        {
            const int frameIndex = MV1SearchFrame(modelHandle_, frameName);
            if (frameIndex >= 0)
            {
                MV1SetFrameVisible(modelHandle_, frameIndex, FALSE);
            }
        }
    }

    // Blender から混入した不要オブジェクト(白い板)を非表示化する
    {
        const TCHAR* hiddenFrameCandidates[] =
        {
            _T("立方体"),
            _T("Cube"),
            _T("Cube.001")
        };

        for (const auto& frameName : hiddenFrameCandidates)
        {
            const int frameIndex = MV1SearchFrame(modelHandle_, frameName);
            if (frameIndex >= 0)
            {
                MV1SetFrameVisible(modelHandle_, frameIndex, FALSE);
            }
        }
    }

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

    // コンボ段ごとの経過時間を更新（段が変わったら0に戻す）
    if (comboStep_ > 0)
    {
        if (prevComboStepForHitbox_ != comboStep_)
        {
            prevComboStepForHitbox_ = comboStep_;
            comboStepElapsedTime_ = 0.0f;
        }
        else
        {
            comboStepElapsedTime_ += deltaTime;
        }
    }
    else
    {
        prevComboStepForHitbox_ = 0;
        comboStepElapsedTime_ = 0.0f;
    }

    // 空中コンボ段が進むたびに少し浮き上がる
    if (!isGrounded_ && comboStep_ > 0)
    {
        if (prevComboStepForAirFloat_ != comboStep_)
        {
            verticalVelocity_ = kAirAttackLiftVelocity;
            prevComboStepForAirFloat_ = comboStep_;
        }
    }
    else
    {
        prevComboStepForAirFloat_ = 0;
    }

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
            // 空中攻撃中は水平位置を固定しつつ、ふわふわ落下させる
            if (isAirAttackLocked_)
            {
                position_.x = airAttackLockPosition_.x;
                position_.z = airAttackLockPosition_.z;

                const float gravityScale = (verticalVelocity_ < 0.0f)
                    ? kAirAttackFallGravityScale
                    : kAirAttackRiseGravityScale;
                verticalVelocity_ += gravity_ * gravityScale * deltaTime;
                position_.y += verticalVelocity_ * deltaTime;

                ClampToGround();
                if (isGrounded_)
                {
                    isAirAttackLocked_ = false;
                }
                else
                {
                    airAttackLockPosition_.y = position_.y;
                }

                airPhysicsApplied = true;
            }
            else
            {
                ApplyAirPhysics();
                airPhysicsApplied = true;

                // エリアル追従ジャンプ中は目標高度まで到達したら上昇を止める
                if (isFollowingAerialTarget_ && position_.y >= aerialTargetY_)
                {
                    position_.y = aerialTargetY_;
                    verticalVelocity_ = 0.0f;
                    isFollowingAerialTarget_ = false;
                }

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
                    isFollowingAerialTarget_ = false;
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
    const bool attackReleased = !(currentAttackInput & 1) && (previousMouseInput_ & 1);

    // 左クリック長押し時間を計測（エリアル始動判定に使用）
    if (attackHeld)
    {
        if (!wasAttackHeld_)
        {
            attackHoldTimer_ = 0.0f;
            longPressAttackTriggered_ = false;
        }
        attackHoldTimer_ += deltaTime;
    }

    const bool shouldTriggerDodgeAttack =
        canDodgeAttack_
        && attackHeld
        && !isDodging_
        && !attack_.IsAttacking()
        && comboStep_ == 0
        && !isJumping_;

    // 長押しエリアル始動攻撃（1段目のみ）
    const bool canStartAerialStarter =
        attackHeld
        && !longPressAttackTriggered_
        && attackHoldTimer_ >= kAerialStarterHoldThreshold
        && !isDodging_
        && !attack_.IsAttacking()
        && comboStep_ == 0
        && isGrounded_
        && attackRecoveryTimer_ <= 0.0f;

    // 短押し（閾値未満で離した）で通常1段目を開始
    const bool shouldStartNormalComboFromShortClick =
        attackReleased
        && !longPressAttackTriggered_
        && attackHoldTimer_ > 0.0f
        && attackHoldTimer_ < kAerialStarterHoldThreshold
        && !isDodging_
        && !attack_.IsAttacking()
        && comboStep_ == 0
        && attackRecoveryTimer_ <= 0.0f;

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
        isAerialStarterAttack_ = false;

        attack_.ExecuteStrongAttack();
        playerAnimation_.PlayDodgeAttack();
    }
    else if (canStartAerialStarter)
    {
        // 左クリック長押し: コンボ1段目のみを始動（エリアル打ち上げ用）
        pendingDodgeAttackRecovery_ = false;
        isAerialStarterAttack_ = true;
        longPressAttackTriggered_ = true;

        attack_.ExecuteWeakAttack();
        playerAnimation_.PlayComboSegment(0, isGrounded_, position_, isAirAttackLocked_, airAttackLockPosition_, comboStep_, attack_);

        if (!isGrounded_)
        {
            verticalVelocity_ = kAirAttackLiftVelocity;
        }
    }
    else if (attackPressed && attackRecoveryTimer_ <= 0.0f)
    {
        if (isDodging_)
        {
            queuedDodgeAttack_ = true;
        }
        else if (comboStep_ > 0 && comboStep_ <= 2)
        {
            pendingCombo_ = true;
        }
    }
    else if (shouldStartNormalComboFromShortClick)
    {
        pendingDodgeAttackRecovery_ = false;
        isAerialStarterAttack_ = false;
        attack_.ExecuteWeakAttack();
        playerAnimation_.PlayComboSegment(0, isGrounded_, position_, isAirAttackLocked_, airAttackLockPosition_, comboStep_, attack_);

        if (!isGrounded_)
        {
            verticalVelocity_ = kAirAttackLiftVelocity;
        }
    }

    if (!attackHeld)
    {
        attackHoldTimer_ = 0.0f;
        longPressAttackTriggered_ = false;
    }

    wasAttackHeld_ = attackHeld;

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

    // エリアル始動フラグは1段目中のみ保持する
    if (isAerialStarterAttack_ && comboStep_ != 1)
    {
        isAerialStarterAttack_ = false;
    }
    if (!attack_.IsAttacking() && comboStep_ == 0)
    {
        isAerialStarterAttack_ = false;
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
            position_.x = airAttackLockPosition_.x;
            position_.z = airAttackLockPosition_.z;

            const float gravityScale = (verticalVelocity_ < 0.0f)
                ? kAirAttackFallGravityScale
                : kAirAttackRiseGravityScale;
            verticalVelocity_ += gravity_ * gravityScale * deltaTime;
            position_.y += verticalVelocity_ * deltaTime;

            ClampToGround();
            if (isGrounded_)
            {
                isAirAttackLocked_ = false;
            }
            else
            {
                airAttackLockPosition_.y = position_.y;
            }
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
                position_.x = airAttackLockPosition_.x;
                position_.z = airAttackLockPosition_.z;
                airAttackLockPosition_.y = position_.y;
                MV1SetPosition(modelHandle_, position_);
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

// 見た目の向き取得（水平前方向）
VECTOR Player::GetFacingDirection() const
{
    // modelRotationY_ は描画都合で +PI オフセットしているため戻して使う
    const float worldYaw = modelRotationY_ - kDefaultRotationY;
    return VGet(std::sin(worldYaw), 0.0f, std::cos(worldYaw));
}

// 攻撃状態取得
AttackType Player::GetCurrentAttack() const
{
    return attack_.GetCurrentAttack();
}

// 現在コンボ段取得
int Player::GetComboStep() const
{
    return comboStep_;
}

// 攻撃判定有効タイミング判定
bool Player::IsAttackHitboxActive() const
{
    // コンボ段が無いときは判定無し
    if (comboStep_ <= 0)
    {
        return false;
    }

    // 三段目だけ少し遅れて判定を有効化する
    if (comboStep_ == 3 && comboStepElapsedTime_ < kThirdComboHitDelay)
    {
        return false;
    }

    return true;
}

// エリアル始動攻撃中判定
bool Player::IsAerialStarterAttackActive() const
{
    return isAerialStarterAttack_ && comboStep_ == 1 && attack_.IsAttacking();
}

// 敵打ち上げ後の高度へ追従ジャンプ開始
void Player::StartAerialFollowJump(float targetY)
{
    if (targetY <= position_.y + 1.0f)
    {
        return;
    }

    // ジャンプ上昇モーションを開始
    isJumping_ = true;
    isGrounded_ = false;
    isFallingAnimActive_ = false;
    actionTimer_ = 0.0f;

    // 目標高度まで届く上昇速度を計算（v^2 = 2gh）
    const float riseDistance = targetY - position_.y;
    const float g = -gravity_;
    float launchVelocity = (g > 0.0f) ? std::sqrt(2.0f * g * riseDistance) : jumpStartVelocity_;
    if (launchVelocity < jumpStartVelocity_)
    {
        launchVelocity = jumpStartVelocity_;
    }

    verticalVelocity_ = launchVelocity;
    isFollowingAerialTarget_ = true;
    aerialTargetY_ = targetY;

    const int jumpAnimIndex = playerAnimation_.GetJumpAnimIndex();
    const float jumpTotal = playerAnimation_.GetAnimTotalTime(jumpAnimIndex);
    const float jumpHalf = (jumpTotal > 0.0f) ? (jumpTotal * 0.5f) : 0.0f;
    playerAnimation_.PlaySegment(jumpAnimIndex, 0.0f, jumpHalf, kJumpRiseAnimDuration, false);
}

// 攻撃経過時間取得
float Player::GetAttackElapsedTime() const
{
    return attack_.GetAttackDuration();
}

// 攻撃中判定
bool Player::IsAttacking() const
{
    return attack_.IsAttacking();
}