#include <DxLib.h>
#include "Player/PlayerManager.h"
#include "Enemy/EnemyManager.h"
#include <cmath>

namespace
{
    // DxLib全体の初期化
    bool InitializeDxLib()
    {
        // ウィンドウと描画設定
        ChangeWindowMode(TRUE);
        SetGraphMode(1280, 720, 32);

        SetZBufferBitDepth(24);

        // ライブラリ初期化
        if (DxLib_Init() == -1)
        {
            return false;
        }

        // 3D描画設定
        SetDrawScreen(DX_SCREEN_BACK);
        SetUseZBuffer3D(TRUE);
        SetWriteZBuffer3D(TRUE);
        SetCameraNearFar(1.0f, 50000.0f);

        // ライト設定
        SetLightEnable(TRUE);
        VECTOR lightDir = VGet(1.0f, -1.0f, -1.0f);
        SetLightDirection(lightDir);

        COLOR_F ambientColor = { 1.6f, 1.6f, 1.6f, 1.0f };
        SetLightAmbColor(ambientColor);

        COLOR_F diffuseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        SetLightDifColor(diffuseColor);

        return true;
    }

    // デバッグ用の床を描画
    void DrawDebugFloor()
    {
        const VECTOR minPos = VGet(-3000.0f, -10.0f, -3000.0f);
        const VECTOR maxPos = VGet(3000.0f, 0.0f, 3000.0f);
        const int floorColor = GetColor(255, 255, 255);
        DrawCube3D(minPos, maxPos, floorColor, floorColor, TRUE);
    }

    // ロックオンマーカーを白い点で描画する
    void DrawLockOnMarker(const VECTOR& worldPosition)
    {
        const VECTOR screenPosition = ConvWorldPosToScreenPos(worldPosition);
        const int centerX = static_cast<int>(screenPosition.x);
        const int centerY = static_cast<int>(screenPosition.y);
        const int markerColor = GetColor(255, 255, 255);

        // 小さく見やすい点として描画する
        DrawCircle(centerX, centerY, 3, markerColor, TRUE);
    }
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    // 初期化失敗時は終了
    if (!InitializeDxLib())
    {
        return -1;
    }

    // 管理クラス生成
    PlayerManager playerManager;
    EnemyManager enemyManager;
    playerManager.Initialize();
    enemyManager.Initialize();

    // カメラ制御用パラメータ
    float cameraYaw = 3.14159f;
    float cameraPitch = 0.55f;
    constexpr float kCameraDistance = 520.0f;
    constexpr float kCameraYawSpeed = 0.035f;
    constexpr float kCameraPitchSpeed = 0.015f;
    constexpr float kMouseYawSensitivity = 0.0032f;
    constexpr float kMousePitchSensitivity = 0.0022f;
    constexpr float kCameraTargetHeight = 120.0f;
    constexpr float kCameraMinPitch = 0.20f;
    constexpr float kCameraMaxPitch = 1.20f;

    // ロックオン状態
    bool isLockOnActive = false;
    int previousMouseButtons = 0;

    int prevMouseX = 0;
    int prevMouseY = 0;
    GetMousePoint(&prevMouseX, &prevMouseY);

    // メインループ
    while (ProcessMessage() == 0 && CheckHitKey(KEY_INPUT_ESCAPE) == 0)
    {
        int mouseX = 0;
        int mouseY = 0;
        GetMousePoint(&mouseX, &mouseY);

        const int mouseButtons = GetMouseInput();
        const bool middleMousePressed = (mouseButtons & MOUSE_INPUT_MIDDLE) != 0;
        const bool middleMouseTriggered = middleMousePressed && !(previousMouseButtons & MOUSE_INPUT_MIDDLE);

        const int mouseDeltaX = mouseX - prevMouseX;
        const int mouseDeltaY = mouseY - prevMouseY;

        // ロックオン中以外は通常のマウス視点操作を行う
        if (!isLockOnActive)
        {
            cameraYaw += static_cast<float>(mouseDeltaX) * kMouseYawSensitivity;
            cameraPitch += static_cast<float>(mouseDeltaY) * kMousePitchSensitivity;
        }

        prevMouseX = mouseX;
        prevMouseY = mouseY;
        previousMouseButtons = mouseButtons;

        // ホイールクリックでロックオンを切り替える
        if (middleMouseTriggered)
        {
            if (isLockOnActive)
            {
                isLockOnActive = false;
            }
            else if (enemyManager.HasLockOnTarget())
            {
                isLockOnActive = true;
            }
        }

        // 敵がいなくなったら自動でロックを外す
        if (isLockOnActive && !enemyManager.HasLockOnTarget())
        {
            isLockOnActive = false;
        }

        // キーによるカメラ操作はロックオン中は無効にする
        if (!isLockOnActive)
        {
            if (CheckHitKey(KEY_INPUT_LEFT)) cameraYaw -= kCameraYawSpeed;
            if (CheckHitKey(KEY_INPUT_RIGHT)) cameraYaw += kCameraYawSpeed;
            if (CheckHitKey(KEY_INPUT_UP)) cameraPitch += kCameraPitchSpeed;
            if (CheckHitKey(KEY_INPUT_DOWN)) cameraPitch -= kCameraPitchSpeed;
        }

        // ピッチ角を制御
        if (cameraPitch < kCameraMinPitch) cameraPitch = kCameraMinPitch;
        if (cameraPitch > kCameraMaxPitch) cameraPitch = kCameraMaxPitch;

        // 更新
        playerManager.Update(cameraYaw);
        enemyManager.Update(
            playerManager.GetPlayerPosition(),
            playerManager.GetPlayerFacingDirection(),
            playerManager.GetPlayerCurrentAttack(),
            playerManager.GetPlayerComboStep(),
            playerManager.IsPlayerAttackHitboxActive(),
            playerManager.IsPlayerAttacking(),
            playerManager.IsPlayerAerialStarterAttackActive(),
            playerManager.GetPlayerAttackElapsedTime());

        float aerialFollowTargetY = 0.0f;
        if (enemyManager.ConsumeAerialFollowJumpRequest(aerialFollowTargetY))
        {
            playerManager.ApplyAerialFollowJump(aerialFollowTargetY);
        }

        const VECTOR playerPos = playerManager.GetPlayerPosition();
        VECTOR cameraTarget = VGet(playerPos.x, playerPos.y + kCameraTargetHeight, playerPos.z);
        VECTOR cameraPos = VGet(0.0f, 0.0f, 0.0f);

        // ロックオン中は敵を画面中央へ固定するカメラに切り替える
        if (isLockOnActive && enemyManager.HasLockOnTarget())
        {
            const VECTOR lockOnTarget = enemyManager.GetLockOnTargetPosition();
            VECTOR toEnemy = VSub(lockOnTarget, playerPos);
            toEnemy.y = 0.0f;

            const float flatDistance = std::sqrt(toEnemy.x * toEnemy.x + toEnemy.z * toEnemy.z);
            VECTOR flatDir = VGet(0.0f, 0.0f, 1.0f);
            if (flatDistance > 0.0001f)
            {
                flatDir.x = toEnemy.x / flatDistance;
                flatDir.z = toEnemy.z / flatDistance;
            }

            const float horizontalDistance = kCameraDistance * std::cos(cameraPitch);
            const float heightOffset = kCameraDistance * std::sin(cameraPitch);

            // 敵を見る形で、プレイヤーの背後側にカメラを置く
            cameraPos = VGet(
                playerPos.x - flatDir.x * horizontalDistance,
                playerPos.y + kCameraTargetHeight + heightOffset,
                playerPos.z - flatDir.z * horizontalDistance);
            cameraTarget = lockOnTarget;

            // プレイヤー移動の向き計算にも使うため、ロックオン中の角度へ同期する
            cameraYaw = std::atan2(flatDir.x, flatDir.z);
            const VECTOR viewDirection = VSub(cameraTarget, cameraPos);
            const float viewFlatLength = std::sqrt(viewDirection.x * viewDirection.x + viewDirection.z * viewDirection.z);
            if (viewFlatLength > 0.0001f)
            {
                cameraPitch = std::atan2(viewDirection.y, viewFlatLength);
            }
        }
        else
        {
            // 通常時はプレイヤー中心の追従カメラ
            const float horizontalDistance = kCameraDistance * std::cos(cameraPitch);
            const float heightOffset = kCameraDistance * std::sin(cameraPitch);
            cameraPos = VGet(
                cameraTarget.x - std::sin(cameraYaw) * horizontalDistance,
                cameraTarget.y + heightOffset,
                cameraTarget.z - std::cos(cameraYaw) * horizontalDistance);
        }

        SetCameraPositionAndTarget_UpVecY(cameraPos, cameraTarget);

        // 描画
        ClearDrawScreen();
        DrawDebugFloor();
        enemyManager.Draw();
        playerManager.Draw();

        // ロックオン中は敵の中心付近に白い円を描く
        if (isLockOnActive && enemyManager.HasLockOnTarget())
        {
            DrawLockOnMarker(enemyManager.GetLockOnTargetPosition());
        }

        ScreenFlip();
    }

    // 終了処理
    enemyManager.Finalize();
    playerManager.Finalize();
    DxLib_End();

    return 0;
}
