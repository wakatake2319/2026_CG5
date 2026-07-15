#include "VertexBuffer.h"
#include "KamataEngine.h"

#include <cassert>
#include <d3d12.h>

using namespace KamataEngine;

// VertexBufferの生成
void VertexBuffer::Create(const UINT size, const UINT stride) {
	// DirectXCommonインスタンスの取得
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();


#pragma region VertexResource

	// 頂点リソース用のヒープの設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD; // CPUから書き込むヒープ

	// 頂点リソースの設定
	D3D12_RESOURCE_DESC vertexResourceDesc{};
	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; // バッファリソースであることを示す
	vertexResourceDesc.Width = size;                 // 頂点1つあたり4要素(位置)×4バイト(32ビット)×3頂点分

	// バッファの場合はこれらは1にする決まり
	vertexResourceDesc.Height = 1;           // バッファリソースなので高さは1
	vertexResourceDesc.DepthOrArraySize = 1; // バッファリソースなので深さは1
	vertexResourceDesc.MipLevels = 1;        // バッファリソースなのでミップマップレベルは1
	vertexResourceDesc.SampleDesc.Count = 1; // マルチサンプリングしない

	// バッファの場合はこれにする決まり
	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; // 行優先のメモリ配置

	// 実際にリソースを生成する
	ID3D12Resource* vertexResource = nullptr;
	[[maybe_unused]] HRESULT hr = dxCommon->GetDevice()->CreateCommittedResource(
	    &uploadHeapProperties,             // アップロードヒープを指定
	    D3D12_HEAP_FLAG_NONE,              // ヒープフラグ
	    &vertexResourceDesc,               // リソースの詳細
	    D3D12_RESOURCE_STATE_GENERIC_READ, // リソースの使用状態
	    nullptr,                           // 最適化されたクリア値（バッファリソースなのでnullptr）
	    IID_PPV_ARGS(&vertexResource));    // 生成したリソースへのポインタを受け取る
	assert(SUCCEEDED(hr));

	// 頂点リソースをとっておく
	vertexBuffer_ = vertexResource;
#pragma endregion


	// VertexBufferViewの作成
#pragma region VertexBufferView
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	// リソースの先頭アドレスから使う
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress(); // 頂点リソースのGPU仮想アドレス
	// 使用するリソースのサイズは頂点size分のサイズ
	vertexBufferView.SizeInBytes = size; // 頂点リソースのサイズ
	// 1つの頂点のサイズ
	vertexBufferView.StrideInBytes = stride;  

	// 頂点バッファビューをとっておく
	vertexBufferView_ = vertexBufferView;
#pragma endregion
}

// 生成したVertexBufferを返す
ID3D12Resource* VertexBuffer::Get() { return vertexBuffer_; }

// 生成したVertexBufferViewを返す
D3D12_VERTEX_BUFFER_VIEW* VertexBuffer::GetView() { return &vertexBufferView_; }

// コンストラクタ
VertexBuffer::VertexBuffer() {}
// デストラクタ
VertexBuffer::~VertexBuffer() {
	if (vertexBuffer_) {
		vertexBuffer_->Release();
		vertexBuffer_ = nullptr;
	}
}