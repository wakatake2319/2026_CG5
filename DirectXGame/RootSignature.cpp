#include "RootSignature.h"
#include "KamataEngine.h"
#include <cassert>

using namespace KamataEngine;

void RootSignature::Create() {
	// 敵にインスタンスがあるなら解放する
	// Createメンバ関数が2度実行されたときの処理
	if (rootSignature_) {
		rootSignature_->Release();
		rootSignature_ = nullptr;
	}

	// クラス内で取得するために追加
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	// RootSignatureの作成
#pragma region RootSignature
	// 構造体にデータを用意する
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	
	// デスクリプタレンジ
	D3D12_DESCRIPTOR_RANGE srvDescRange[1]{};
	srvDescRange[0].BaseShaderRegister = 0; // シェーダー内でのレジスタ番号 0から始まる
	srvDescRange[0].NumDescriptors = 1;     // デスクリプタの数
	srvDescRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;                              // SRV
	srvDescRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // デスクリプタテーブル内のオフセット これも0から始まる

	// ルートパラメータの用意 ※ピクセルシェーダーに読み込ませるために必要
	// 複数設定できるので配列の構造をしている。今回は1つだけなので、長さ1の配列にする
	D3D12_ROOT_PARAMETER rootParameters[1]{};

	rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE; // デスクリプタテーブル
	rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;           // ピクセルシェーダーで見えるようにする
	rootParameters[0].DescriptorTable.pDescriptorRanges = srvDescRange;           // 拡張しやすくする
	rootParameters[0].DescriptorTable.NumDescriptorRanges = _countof(srvDescRange); // レンジテーブル数

	descriptionRootSignature.pParameters = rootParameters; // ルートパラメータの配列
	descriptionRootSignature.NumParameters = _countof(rootParameters); // ルートパラメータの数

	// Samplerの設定
	D3D12_STATIC_SAMPLER_DESC staticSamplers[1]{};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; // バイリニアフィルタ
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // テクスチャの横方向のはみ出し方 0.0～1.0の範囲を繰り返す
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // テクスチャの縦方向のはみ出し方 0.0～1.0の範囲を繰り返す
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // テクスチャの奥方向のはみ出し方 0.0～1.0の範囲を繰り返す
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; // 比較関数は使わない
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;                   // たくさんのmipmapを使う
	staticSamplers[0].ShaderRegister = 0;                           // シェーダー内でのレジスタ番号 0から始まる
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // ピクセルシェーダーで使う

	descriptionRootSignature.pStaticSamplers = staticSamplers; // 静的サンプラーの配列
	descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers); // 静的サンプラーの数

	ID3DBlob* signatureBlob = nullptr; // シグネチャのバイナリデータ
	ID3DBlob* errorBlog = nullptr;     // エラーのバイナリデ
	[[maybe_unused]] HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlog);
	if (FAILED(hr)) {
		DebugText::GetInstance()->ConsolePrintf(reinterpret_cast<char*>(errorBlog->GetBufferPointer()));
		// 失敗
		assert(false);
	}
	// バイナリをもとに生成
	ID3D12RootSignature* rootSignature = nullptr;
	hr = dxCommon->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));

	// signatureBlobはRootSignature生成後解放してもいい
	signatureBlob->Release();

	// 生成したRootSignatureをとっておく
	rootSignature_ = rootSignature;
#pragma endregion


}

// 生成したRootSignatureを返す
ID3D12RootSignature* RootSignature::Get() { return rootSignature_; }

// コンストラクタ
RootSignature::RootSignature() {}

// デストラクタ
RootSignature::~RootSignature() {
	if (rootSignature_) {
		rootSignature_->Release();
		rootSignature_ = nullptr;
	}
}