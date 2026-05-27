#include "PipelineState.h"
#include "KamataEngine.h"

using namespace KamataEngine;

// パイプラインステートの生成
void PipelineState::Create(D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc) {
	// DirectXCommonインスタンスの取得
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	ID3D12PipelineState* graphicsPipelineState = nullptr;
	HRESULT hr = dxCommon->GetDevice()->CreateGraphicsPipelineState(
		&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
	assert(SUCCEEDED(hr));

	// 生成したパイプラインステートをとっておく
	pipelineState_ = graphicsPipelineState;
}

// 生成したパイプラインステートを返す
ID3D12PipelineState* PipelineState::Get() { return pipelineState_; }

// コンストラクタ
PipelineState::PipelineState() {}

// デストラクタ
PipelineState::~PipelineState() {
	if (pipelineState_) {
		pipelineState_->Release();
		pipelineState_ = nullptr;
	}
}