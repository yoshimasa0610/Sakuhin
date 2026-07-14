#pragma once

#include "DxLib.h"

namespace MyMath
{
	// ベクトル関連の計算関数
	VECTOR VecCreate(VECTOR startPos, VECTOR endPos);
	VECTOR VecAdd(VECTOR vecA, VECTOR vecB);
	VECTOR VecScale(VECTOR vecA, float scale);
	float VecDot(VECTOR vecA, VECTOR vecB);
	float VecCross2D(VECTOR vecA, VECTOR vecB);
	VECTOR VecCross3D(VECTOR vecA, VECTOR vecB);
	float VecLong(VECTOR vec);
	VECTOR VecNormalize(VECTOR vec);
	VECTOR VecForwardZX(float rotY);
	// 行列関連の計算関数
	MATRIX MatCreate();
	MATRIX MatAdd(MATRIX matA, MATRIX matB);
	MATRIX MatSubt(MATRIX matA, MATRIX matB);
	MATRIX MatScale(MATRIX mat, float scale);
	MATRIX MatMult(MATRIX matA, MATRIX matB);
	MATRIX MatTransposition(MATRIX mat);
	MATRIX MatTranslation(VECTOR trans);
	MATRIX MatScale(VECTOR scale);
	MATRIX MatRotationPitch(float pitch);
	MATRIX MatRotationYaw(float pitch);
	MATRIX MatRotationRoll(float pitch);
	VECTOR MatTransform(MATRIX transMat, VECTOR pos);
};
