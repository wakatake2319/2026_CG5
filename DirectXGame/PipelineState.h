#pragma once

#include <d3d12.h>

class PipelineState {
public:
	// 初期化
	void Create(D3D12_GRAPHICS_PIPELINE_STATE_DESC desc);

	// ゲッター
	ID3D12PipelineState* Get();

	PipelineState();
	~PipelineState();

private:
	ID3D12PipelineState* pipelineState_ = nullptr;
};