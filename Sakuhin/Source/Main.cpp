#include <DxLib.h>
#include "Player/PlayerManager.h"
#include <cmath>

namespace
{
    // DxLib全体の初期化
    bool InitializeDxLib()
    {
        // ウィンドウと描画設定
        ChangeWindowMode(TRUE);
        SetGraphMode(1280, 720, 32);

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

    // デバッグ用の白い床を描画
    void DrawDebugFloor()
    {
        const VECTOR minPos = VGet(-3000.0f, -10.0f, -3000.0f);
        const VECTOR maxPos = VGet(3000.0f, 0.0f, 3000.0f);
        const int floorColor = GetColor(255, 255, 255);
        DrawCube3D(minPos, maxPos, floorColor, floorColor, TRUE);
    }
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    // 初期化失敗時は終了
    if (!InitializeDxLib())
    {
        return -1;
    }

    // プレイヤー管理クラス生成
    PlayerManager playerManager;
    playerManager.Initialize();

    // カメラ制御パラメータ
    float cameraYaw = 3.14159f;
    float cameraPitch = 0.55f;
    constexpr float kCameraDistance = 520.0f;
    constexpr float kCameraYawSpeed = 0.035f;
    constexpr float kCameraPitchSpeed = 0.015f;
    constexpr float kCameraTargetHeight = 120.0f;
    constexpr float kCameraMinPitch = 0.20f;
    constexpr float kCameraMaxPitch = 1.20f;

    // メインループ
    while (ProcessMessage() == 0 && CheckHitKey(KEY_INPUT_ESCAPE) == 0)
    {
        // 矢印キーでカメラ角度を更新
        if (CheckHitKey(KEY_INPUT_LEFT))
        {
            cameraYaw -= kCameraYawSpeed;
        }
        if (CheckHitKey(KEY_INPUT_RIGHT))
        {
            cameraYaw += kCameraYawSpeed;
        }
        if (CheckHitKey(KEY_INPUT_UP))
        {
            cameraPitch += kCameraPitchSpeed;
        }
        if (CheckHitKey(KEY_INPUT_DOWN))
        {
            cameraPitch -= kCameraPitchSpeed;
        }

        // ピッチ角を制限
        if (cameraPitch < kCameraMinPitch) cameraPitch = kCameraMinPitch;
        if (cameraPitch > kCameraMaxPitch) cameraPitch = kCameraMaxPitch;

        // プレイヤー更新（カメラYawを渡す）
        playerManager.Update(cameraYaw);

        // カメラ目標位置（プレイヤー中心）
        const VECTOR playerPos = playerManager.GetPlayerPosition();
        const VECTOR cameraTarget = VGet(playerPos.x, playerPos.y + kCameraTargetHeight, playerPos.z);

        // 球面座標でカメラ位置を算出
        const float horizontalDistance = kCameraDistance * std::cos(cameraPitch);
        const float heightOffset = kCameraDistance * std::sin(cameraPitch);

        const VECTOR cameraPos = VGet(
            cameraTarget.x - std::sin(cameraYaw) * horizontalDistance,
            cameraTarget.y + heightOffset,
            cameraTarget.z - std::cos(cameraYaw) * horizontalDistance);

        // カメラ反映
        SetCameraPositionAndTarget_UpVecY(cameraPos, cameraTarget);

        // 描画
        ClearDrawScreen();
        DrawDebugFloor();
        playerManager.Draw();
        ScreenFlip();
    }

    // 終了処理
    playerManager.Finalize();
    DxLib_End();

    return 0;
}
