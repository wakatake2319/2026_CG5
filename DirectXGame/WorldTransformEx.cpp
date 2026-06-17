#include "WorldTransformEx.h"

using namespace KamataEngine;
using namespace KamataEngine::MathUtility;// Make～Matrix4x4同士の積(+)の利用

// Scale、Rotation、Translation行列からworld行列を計算、定数バッファに転送する
void WorldTransformEx::UpdateMatrix() {
	// world行列の生成
	matWorld_ = MakeAffineMatrix();
	// 定数バッファへの転送
	TransferMatrix();
}

// アフェイン変換行列を作る
Matrix4x4 WorldTransformEx::MakeAffineMatrix() {
	// スケール行列の生成
	Matrix4x4 matScale = MakeScaleMatrix(scale_);

	// 回転行列の生成
	// X軸回りの回転行列の生成
	Matrix4x4 matRotX = MakeRotateXMatrix(rotation_.x);
	// Y軸回りの回転行列の生成
	Matrix4x4 matRotY = MakeRotateYMatrix(rotation_.y);
	// Z軸回りの回転行列の生成
	Matrix4x4 matRotZ = MakeRotateZMatrix(rotation_.z);
	Matrix4x4 matRot = matRotZ * matRotX * matRotY;

	// 平行移動行列の生成
	Matrix4x4 matTrans = MakeTranslateMatrix(translation_);

	// world Matrix
	Matrix4x4 matWorld = matScale * matRot * matTrans;

	// ワールド行列の生成
	return matWorld;
}