#include "KamataEngine.h"
#include "IndexBuffer.h"

#include <cassert>
#include <d3d12.h>

using namespace KamataEngine;

// IndexBufferの生成
void IndexBuffer::Create(const UINT size, const UINT stride) {
	// strideの値によって、1つのインデックスのフォーマットを決める
	assert(stride == 2 || stride == 4); // 2バイト（16ビット）か4バイト（32ビット）であることを確認
	DXGI_FORMAT format = (stride == 2) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT; // フォーマットを決定

	// クラス内でdxCommonを利用するために追加
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

#pragma region IndexResource

	// 頂点リソース用のヒープの設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD; // CPUから書き込むヒープ

	// 頂点リソースの設定
	D3D12_RESOURCE_DESC indexResourceDesc{};
	indexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; // バッファリソースであることを示す
	indexResourceDesc.Width = size;                                // 頂点1つあたり4要素(位置)×4バイト(32ビット)×3頂点分

	// バッファの場合はこれらは1にする決まり
	indexResourceDesc.Height = 1;           // バッファリソースなので高さは1
	indexResourceDesc.DepthOrArraySize = 1; // バッファリソースなので深さは1
	indexResourceDesc.MipLevels = 1;        // バッファリソースなのでミップマップレベルは1
	indexResourceDesc.SampleDesc.Count = 1; // マルチサンプリングしない

	// バッファの場合はこれにする決まり
	indexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; // 行優先のメモリ配置

	// 実際にリソースを生成する
	ID3D12Resource* indexResource = nullptr;
	[[maybe_unused]] HRESULT hr = dxCommon->GetDevice()->CreateCommittedResource(
	    &uploadHeapProperties,             // アップロードヒープを指定
	    D3D12_HEAP_FLAG_NONE,              // ヒープフラグ
	    &indexResourceDesc,                // リソースの詳細
	    D3D12_RESOURCE_STATE_GENERIC_READ, // リソースの使用状態
	    nullptr,                           // 最適化されたクリア値（バッファリソースなのでnullptr）
	    IID_PPV_ARGS(&indexResource)); // 生成したリソースへのポインタを受け取る
	assert(SUCCEEDED(hr));

	// 生成したインデックスリソースをとっておく
	indexBuffer_ = indexResource;
#pragma endregion

	// IndexBufferViewの作成
#pragma region IndexBufferView
	D3D12_INDEX_BUFFER_VIEW indexBufferView{};
	// リソースの先頭アドレスから使う
	indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress(); // 頂点リソースのGPU仮想アドレス
	// 使用するリソースのサイズは頂点size分のサイズ
	indexBufferView.SizeInBytes = size; // 頂点リソースのサイズ
	// 1つの頂点のサイズ
	indexBufferView.Format = format;

	// インデックスバッファビューをとっておく
	indexBufferView_ = indexBufferView;
#pragma endregion

}


// 生成したインデックスバッファを返す
ID3D12Resource* IndexBuffer::Get() { return indexBuffer_; }

// 用意済みのインデックスバッファビューを返す
D3D12_INDEX_BUFFER_VIEW* IndexBuffer::GetView() { return &indexBufferView_; }

// コンストラクタ
IndexBuffer::IndexBuffer() {}

IndexBuffer::~IndexBuffer() {
	if (indexBuffer_) {
		indexBuffer_->Release();
		indexBuffer_ = nullptr;
	}
}

