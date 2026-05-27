#pragma once

#include <d3d12.h>

class RootSignature {
public:

	// 生成
	void Create();

	// ゲッター
	ID3D12RootSignature* Get();

	RootSignature();
	~RootSignature();

private:
	ID3D12RootSignature* rootSignature_ = nullptr;
};
