#include <DxLib.h>
#include "Player/PlayerManager.h"

namespace
{
    bool InitializeDxLib()
    {
        ChangeWindowMode(TRUE);
        SetGraphMode(1280, 720, 32);

        if (DxLib_Init() == -1)
        {
            return false;
        }

        SetDrawScreen(DX_SCREEN_BACK);
        SetUseZBuffer3D(TRUE);
        SetWriteZBuffer3D(TRUE);
        SetCameraNearFar(1.0f, 50000.0f);

        SetLightEnable(TRUE);
        
        VECTOR lightDir = VGet(1.0f, -1.0f, -1.0f);
        SetLightDirection(lightDir);

        COLOR_F ambientColor = { 1.6f, 1.6f, 1.6f, 1.0f };
        SetLightAmbColor(ambientColor);

        COLOR_F diffuseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
        SetLightDifColor(diffuseColor);

        return true;
    }
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    if (!InitializeDxLib())
    {
        return -1;
    }

    PlayerManager playerManager;
    playerManager.Initialize();

    while (ProcessMessage() == 0 && CheckHitKey(KEY_INPUT_ESCAPE) == 0)
    {
        playerManager.Update();

        const VECTOR playerPos = playerManager.GetPlayerPosition();
        const VECTOR cameraPos = VGet(playerPos.x, playerPos.y + 320.0f, playerPos.z - 520.0f);
        const VECTOR cameraTarget = VGet(playerPos.x, playerPos.y + 120.0f, playerPos.z);

        SetCameraPositionAndTarget_UpVecY(cameraPos, cameraTarget);

        ClearDrawScreen();

        playerManager.Draw();

        DrawFormatString(10, 10, GetColor(255, 255, 255), _T("WASD: Move  ESC: Exit"));
        DrawFormatString(10, 30, GetColor(255, 255, 255), _T("Player Pos X:%.1f Y:%.1f Z:%.1f"), playerPos.x, playerPos.y, playerPos.z);
        DrawFormatString(10, 50, playerManager.IsPlayerModelLoaded() ? GetColor(120, 255, 120) : GetColor(255, 120, 120),
            _T("Model Load: %s"), playerManager.IsPlayerModelLoaded() ? _T("Success") : _T("Failed"));
        DrawFormatString(10, 70, GetColor(220, 220, 220), _T("Model Path: %s"), playerManager.GetLoadedModelPath());

        ScreenFlip();
    }

    playerManager.Finalize();
    DxLib_End();

    return 0;
}
