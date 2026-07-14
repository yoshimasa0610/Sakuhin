#include "MyMath.h"
#include <math.h>

namespace MyMath
{
	// ベクトルを作る
	// startPos	作るベクトルの始点
	// endPos	作るベクトルの終点
	VECTOR VecCreate(VECTOR startPos, VECTOR endPos)
	{
		VECTOR result;

		result.x = endPos.x - startPos.x;
		result.y = endPos.y - startPos.y;
		result.z = endPos.z - startPos.z;

		return result;
	}

	// ベクトルの足し算
	// vecA ＋ vecB
	VECTOR VecAdd(VECTOR vecA, VECTOR vecB)
	{
		VECTOR result;

		result.x = vecA.x + vecB.x;
		result.y = vecA.y + vecB.y;
		result.z = vecA.z + vecB.z;

		return result;
	}

	// ベクトルのスカラー倍
	// vecA * scale
	VECTOR VecScale(VECTOR vecA, float scale)
	{
		VECTOR result;

		result.x = vecA.x * scale;
		result.y = vecA.y * scale;
		result.z = vecA.z * scale;

		return result;
	}

	// ベクトルの内積
	// vecA ・ vecB
	float VecDot(VECTOR vecA, VECTOR vecB)
	{
		return vecA.x * vecB.x + vecA.y * vecB.y + vecA.z * vecB.z;
	}

	// ベクトルの外積(2D)
	// vecA × vecB
	float VecCross2D(VECTOR vecA, VECTOR vecB)
	{
		return vecA.x * vecB.y - vecA.y * vecB.x;
	}

	// ベクトルの外積(3D)
	VECTOR VecCross3D(VECTOR vecA, VECTOR vecB)
	{
		VECTOR result;

		result.x = vecA.y * vecB.z - vecA.z * vecB.y;
		result.y = vecA.x * vecB.z - vecA.z * vecB.x;
		result.z = vecA.y * vecB.x - vecA.x * vecB.y;

		return result;
	}

	// ベクトルの長さを計算する
	float VecLong(VECTOR vec)
	{
		return sqrtf(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
	}

	// ベクトルを正規化する
	VECTOR VecNormalize(VECTOR vec)
	{
		float vecLong = VecLong(vec);

		// 長さ0の場合は0ベクトルを返す
		if (vecLong == 0.0f) return VGet(0.0f, 0.0f, 0.0f);

		return VecScale(vec, 1.0f / vecLong);
	}

	// ZX平面上の前方ベクトル(長さ１)を取得する
	VECTOR VecForwardZX(float rotY)
	{
		VECTOR result;

		// X成分はsin(Θ)
		result.x = sinf(rotY);
		// ZX平面なのでYは無視
		result.y = 0.0f;
		// Z成分はcos(Θ)
		result.z = cosf(rotY);

		return result;
	}

	// 単位行列を生成する
	MATRIX MatCreate()
	{
		MATRIX result = { 0 };

		for (int i = 0; i < 4; i++)
		{
			result.m[i][i] = 1.0f;
		}

		return result;
	}

	// 行列同士の足し算
	MATRIX MatAdd(MATRIX matA, MATRIX matB)
	{
		MATRIX result = { 0 };

		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				result.m[i][j] = matA.m[i][j] + matB.m[i][j];
			}
		}

		return result;
	}

	// 行列同士の引き算
	MATRIX MatSubt(MATRIX matA, MATRIX matB)
	{
		MATRIX result = { 0 };

		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				result.m[i][j] = matA.m[i][j] - matB.m[i][j];
			}
		}

		return result;
	}

	// 行列のスカラー倍
	MATRIX MatScale(MATRIX mat, float scale)
	{
		MATRIX result = { 0 };

		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				result.m[i][j] = mat.m[i][j] * scale;
			}
		}

		return result;
	}

	// 行列同士の掛け算
	MATRIX MatMult(MATRIX matA, MATRIX matB)
	{
		MATRIX result = { 0 };

		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				for (int k = 0; k < 4; k++)
				{
					result.m[i][j] += matA.m[i][k] * matB.m[k][j];
				}
			}
		}

		return result;
	}

	// 行列の転置
	MATRIX MatTransposition(MATRIX mat)
	{
		MATRIX result = { 0 };

		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				result.m[i][j] = mat.m[j][i];
			}
		}

		return result;
	}

	// 平行移動行列を取得
	MATRIX MatTranslation(VECTOR trans)
	{
		MATRIX result = MatCreate();

		result.m[0][3] = trans.x;
		result.m[1][3] = trans.y;
		result.m[2][3] = trans.z;

		return result;
	}

	// 拡縮行列を取得
	MATRIX MatScale(VECTOR scale)
	{
		MATRIX result = MatCreate();

		result.m[0][0] = scale.x;
		result.m[1][1] = scale.y;
		result.m[2][2] = scale.z;

		return result;
	}

	// ピッチ行列を取得
	MATRIX MatRotationPitch(float pitch)
	{
		MATRIX result = MatCreate();

		result.m[1][1] = cosf(pitch);
		result.m[1][2] = -sinf(pitch);
		result.m[2][1] = sinf(pitch);
		result.m[2][2] = cosf(pitch);

		return result;
	}

	// ヨー行列を取得
	MATRIX MatRotationYaw(float pitch)
	{
		MATRIX result = MatCreate();

		result.m[0][0] = cosf(pitch);
		result.m[0][2] = sinf(pitch);
		result.m[2][0] = -sinf(pitch);
		result.m[2][2] = cosf(pitch);

		return result;
	}

	// ロール行列を取得
	MATRIX MatRotationRoll(float pitch)
	{
		MATRIX result = MatCreate();

		result.m[0][0] = cosf(pitch);
		result.m[0][1] = -sinf(pitch);
		result.m[1][0] = sinf(pitch);
		result.m[1][1] = cosf(pitch);

		return result;
	}

	// 座標に変換行列を掛けて変換後の座標を計算する
	VECTOR MatTransform(MATRIX transMat, VECTOR pos)
	{
		VECTOR result = { 0 };


		// 計算をfor文で回したいため、いったん配列に格納する
		float posBuffer[4] = { pos.x, pos.y, pos.z, 1.0f };
		// 計算結果もいったん配列に格納する
		float resultBuffer[4] = { 0 };

		// 行列変換の計算
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				resultBuffer[i] += transMat.m[i][j] * posBuffer[j];
			}
		}

		// 計算結果をベクトルに格納
		result.x = resultBuffer[0];
		result.y = resultBuffer[1];
		result.z = resultBuffer[2];

		return result;
	}


}