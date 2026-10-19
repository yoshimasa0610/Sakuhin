#pragma once

#include <DxLib.h>
#include "../Player/AnimationController.h"

// エネミー用のアニメーション制御をまとめるクラス
class EnemyAnimation
{
public:
    // アニメーションの役割をまとめる列挙型
    enum class Motion
    {
        Idle,
        Move,
        Roar,
        WeakAttack,
        StrongAttack,
        Hit
    };

public:
    // 初期化
    EnemyAnimation();

    // モデルに紐づける
    bool BindModel(int modelHandle);

    // 毎フレーム更新
    void Update();

    // 終了処理
    void Finalize();

    // 各モーションの再生
    bool PlayIdle();
    bool PlayMove();
    bool PlayRoar();
    bool PlayWeakAttack();
    bool PlayStrongAttack();
    bool PlayHit();

    // 個別の再生時間調整
    void SetIdleDuration(float duration);
    void SetMoveDuration(float duration);
    void SetRoarDuration(float duration);
    void SetWeakAttackDuration(float duration);
    void SetStrongAttackDuration(float duration);
    void SetHitDuration(float duration);

    // まとめて再生時間を調整
    void SetAllDurations(float idleDuration, float moveDuration, float roarDuration, float weakAttackDuration, float strongAttackDuration, float hitDuration);

    // 現在モーションの確認
    Motion GetCurrentMotion() const;
    bool IsActionPlaying() const;

private:
    // ループ再生の共通処理
    bool PlayLoop(Motion motion, int animIndex, float duration);

    // 一発再生の共通処理
    bool PlayOneShot(Motion motion, int animIndex, float duration);

private:
    // DxLib のアニメーション再生本体
    AnimationController animationController_;

    // モデルハンドル
    int modelHandle_;

    // モデル内のアニメーション番号
    int idleAnimIndex_;
    int moveAnimIndex_;
    int roarAnimIndex_;
    int weakAttackAnimIndex_;
    int strongAttackAnimIndex_;
    int hitAnimIndex_;

    // 再生時間の個別設定
    float idleDuration_;
    float moveDuration_;
    float roarDuration_;
    float weakAttackDuration_;
    float strongAttackDuration_;
    float hitDuration_;

    // 現在のモーション
    Motion currentMotion_;
};
