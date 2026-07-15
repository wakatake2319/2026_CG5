#include <Windows.h>
#include "KamataEngine.h"
#include "GameScene.h"
#include <cassert>
#include "Shader.h"
#include "RootSignature.h"
#include "PipelineState.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "WorldTransformEx.h"
#include "imgui.h"

using namespace KamataEngine;

// 関数プロトタイプ宣言
void SetupPipelineState(PipelineState& pipelineState, RootSignature& rs, Shader& vs, Shader& ps) {
	// InputLayoutの作成
#pragma region InputLayout
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[2] = {};
	inputElementDescs[0].SemanticName = "POSITION";                        // 頂点の意味を示す文字列
	inputElementDescs[0].SemanticIndex = 0;                                // 同じ意味の頂点が複数ある場合の識別番号
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;          // データの形式
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT; // 頂点データ内のオフセット
	inputElementDescs[1].SemanticName = "TEXCOORD";                        // 頂点の意味を示す文字列
	inputElementDescs[1].SemanticIndex = 0;                                // 同じ意味の頂点が複数ある場合の識別番号
	inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;                // データの形式
	inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT; // 頂点データ内のオフセット
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	inputLayoutDesc.pInputElementDescs = inputElementDescs;    // 頂点の要素の配列
	inputLayoutDesc.NumElements = _countof(inputElementDescs); // 頂点の要素数
#pragma endregion

	// BlendStateの作成
#pragma region BlendState
	D3D12_BLEND_DESC blendDesc{};
	// 全ての色要素を書き込む
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
#pragma endregion

	// RasterizerStateの作成
#pragma region RasterizerState
	D3D12_RASTERIZER_DESC rasterizerDesc{};
	// 裏面をカリングする
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	// 塗りつぶしモードをソリッドにする(ワイヤーフレームなら D3D12_FILL_MODE_WIREFRAME)
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
#pragma endregion

	// PSOの作成
#pragma region PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	graphicsPipelineStateDesc.pRootSignature = rs.Get();                                                    // ルートシグネチャ
	graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;                                                // InputLayout
	graphicsPipelineStateDesc.VS = {vs.GetDxcBlob()->GetBufferPointer(), vs.GetDxcBlob()->GetBufferSize()}; // 頂点シェーダー
	graphicsPipelineStateDesc.PS = {ps.GetDxcBlob()->GetBufferPointer(), ps.GetDxcBlob()->GetBufferSize()}; // ピクセルシェーダー
	graphicsPipelineStateDesc.BlendState = blendDesc;                                                       // BlendState
	graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;                                             // RasterizerState

	// 書き込むRTVの情報
	graphicsPipelineStateDesc.NumRenderTargets = 1;                            // 描画対象は1つ
	graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // 描画対象のフォーマットを指定

	// 利用するリポジトリ
	graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // 描画する図形の形状を三角形にする

	// どのように画面に色を打ち込むかの設定
	graphicsPipelineStateDesc.SampleDesc.Count = 1; // マルチサンプリングしない
	graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	pipelineState.Create(graphicsPipelineStateDesc);
#pragma endregion
}

// RenderTextureResourceの生成
#pragma region RenderTextureResource
ID3D12Resource* CreateRenderTextureResource(
	ID3D12Device* device, uint32_t width, uint32_t height, 
	DXGI_FORMAT clearFormat, const FLOAT* clearColor) 
{
	// 1.生成するRenderTextureのDescの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(width);
	resourceDesc.Height = UINT(height);
	resourceDesc.MipLevels = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	// 2.利用するヒープの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // VRAM上に作る

	// 3.ClearValueの用意
	D3D12_CLEAR_VALUE clearValue{};
	clearValue.Format = clearFormat; // RTVのフォーマットを指定
	clearValue.Color[0] = clearColor[0]; // クリアカラーのR成分
	clearValue.Color[1] = clearColor[1]; // クリアカラーのG成分
	clearValue.Color[2] = clearColor[2]; // クリアカラーのB成分
	clearValue.Color[3] = clearColor[3]; // クリアカラーのA成分

	// 4.RenderTextureResourceの生成
	ID3D12Resource* resource = nullptr;
	[[maybe_unused]] HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		&clearValue,
		IID_PPV_ARGS(&resource)
	);
	assert(SUCCEEDED(hr));

	return resource;
}
#pragma endregion

// DepthStencilTextureResourceの生成
#pragma region DepthStencilTextureResource
ID3D12Resource* CreateDepthStencilTextureResource(
	ID3D12Device* device, uint32_t width, uint32_t height) {
	// 1.生成するDepthStencilTextureのDescの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width;										// Textureの幅
	resourceDesc.Height = height;									// Textureの高さ
	resourceDesc.MipLevels = 1;                                     // mipmapの数 DepthStencilなので1で十分
	resourceDesc.DepthOrArraySize = 1;                              // Textureの配列数 DepthStencilなので1で十分
	resourceDesc.Format = DXGI_FORMAT_D32_FLOAT;                    // DepthStencilとして利用可能なフォーマット

	resourceDesc.SampleDesc.Count = 1;								// サンプリングカット 1固定
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;	// 2次元
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;	// DepthStencilとして使うためのフラグ

	// 2.利用するヒープの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // VRAM上に作る

	// 深度値のクリア設定
	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f;      // 深度値のクリア値は1.0f（最も遠い）
	depthClearValue.Format = DXGI_FORMAT_D32_FLOAT; // zバッファ形式。resourceと合わせる

	// 3.Resourceの生成
	ID3D12Resource* resource = nullptr;
	[[maybe_unused]] HRESULT hr = device->CreateCommittedResource(
	    &heapProperties,					// ヒープの設定
		D3D12_HEAP_FLAG_NONE,				// ヒープの特殊な設定
		&resourceDesc,						// Resourceの設定
	    D3D12_RESOURCE_STATE_DEPTH_WRITE,	// 深度値を書き込み状態にしておく
	    &depthClearValue,					// Clear最適値
	    IID_PPV_ARGS(&resource)				// 作成するResourceポインタへのポインタ
	);			
	assert(SUCCEEDED(hr));

	return resource;
}								 
#pragma endregion

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	// 初期化処理
	Initialize(L"LE3D_14_タケウチ_ハルカ_CG5");

	int effectType = 0;

	// DirectXCommonインスタンスの取得
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();
	// ImguiManagerのインスタンスを取得
	ImGuiManager* imguiManager_ = ImGuiManager::GetInstance();
	//imguiManager_->Initialize();

	// ウィンドウサイズの取得
	int32_t w = dxCommon->GetBackBufferWidth();
	int32_t h = dxCommon->GetBackBufferHeight();
	DebugText::GetInstance()->ConsolePrintf(std::format("width: {}, height: {}\n", w, h).c_str());

	// DirectXCommonクラスが管理している、コマンドリストの取得
	ID3D12GraphicsCommandList* commandList = dxCommon->GetCommandList();

	// RootSignatureの作成
#pragma region RootSignature
	RootSignature rs;
	rs.Create();
#pragma endregion


	// VertetxShaderをCompileする
#pragma region VertexShader
		// 頂点シェーダの読み込みとコンパイル
	    Shader vs;
	    vs.LoadDxc(L"Resources/Shaders/TestVS.hlsl", L"vs_6_0");
	    assert(vs.GetDxcBlob() != nullptr);
#pragma endregion


	// PixelShaderをCompileする
#pragma region PixelShader	
		// ピクセルシェーダーの読み込みとコンパイル

		// NormalShader
		Shader normalPS;
	    normalPS.LoadDxc(L"Resources/Shaders/Normal.PS.hlsl", L"ps_6_0");
	    assert(normalPS.GetDxcBlob() != nullptr);

		// グレースケール
	    Shader grayps;
	    grayps.LoadDxc(L"Resources/Shaders/TestPS.hlsl", L"ps_6_0");
	    assert(grayps.GetDxcBlob() != nullptr);

		// ヴィネッティング
		Shader vignettePS;
		vignettePS.LoadDxc(L"Resources/Shaders/Vignette.PS.hlsl",L"ps_6_0");
	    assert(vignettePS.GetDxcBlob() != nullptr);

		// セピア調
	    Shader sepiaPS;
	    sepiaPS.LoadDxc(L"Resources/Shaders/SepiaTone.PS.hlsl", L"ps_6_0");
	    assert(sepiaPS.GetDxcBlob() != nullptr);

		// BoxFilter
	    Shader boxFilterPS;
	    boxFilterPS.LoadDxc(L"Resources/Shaders/BoxFilter.PS.hlsl", L"ps_6_0");
	    assert(boxFilterPS.GetDxcBlob() != nullptr);

		// GaussianFilter
	    Shader gaussianFilterPS;
	    gaussianFilterPS.LoadDxc(L"Resources/Shaders/GaussianFilter.PS.hlsl", L"ps_6_0");
	    assert(gaussianFilterPS.GetDxcBlob() != nullptr);

		// RadialBlur
	    Shader radialBlurPS;
	    radialBlurPS.LoadDxc(L"Resources/Shaders/RadialBlur.PS.hlsl", L"ps_6_0");
	    assert(radialBlurPS.GetDxcBlob() != nullptr);

		// Outline
	    Shader outlinePS;
	    outlinePS.LoadDxc(L"Resources/Shaders/OutLine.PS.hlsl", L"ps_6_0");
	    assert(outlinePS.GetDxcBlob() != nullptr);
#pragma endregion

	// PipelineStateの作成
#pragma region PSO
	    //PipelineState pipelineState;
	    //SetupPipelineState(pipelineState, rs, vs, ps);
		
		PipelineState normalPSO;
		PipelineState grayPSO;
	    PipelineState vignettePSO;
	    PipelineState sepiaPSO;
	    PipelineState boxFilterPSO;
	    PipelineState gaussianFilterPSO;
	    PipelineState radialBlurPSO;
	    PipelineState outlinePSO;
		SetupPipelineState(normalPSO, rs, vs, normalPS);
	    SetupPipelineState(grayPSO, rs, vs, grayps);
	    SetupPipelineState(vignettePSO, rs, vs, vignettePS);
	    SetupPipelineState(sepiaPSO, rs, vs, sepiaPS);
	    SetupPipelineState(boxFilterPSO, rs, vs, boxFilterPS);
	    SetupPipelineState(gaussianFilterPSO, rs, vs, gaussianFilterPS);
		SetupPipelineState(radialBlurPSO, rs, vs, radialBlurPS);
	    SetupPipelineState(outlinePSO, rs, vs, outlinePS);
#pragma endregion


// リソースの確保を含め、頂点情報を柔軟に対応できるようにVerteData構造体を新たに作成する
#pragma region VerteData構造体
	// Vertex4 > VertexData に変更して利用する
	struct VertexData {
		Vector4 position; // 頂点の位置
		Vector2 texcoord; // 頂点のテクスチャ座標
	};

	// 頂点データの準備
	VertexData vertices[] = {
	    {{-1.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}}, // 左上
	    {{1.0f,  1.0f,  0.0f, 1.0f}, {1.0f, 0.0f}}, // 右上
	    {{-1.0f, -1.0f,  0.0f, 1.0f}, {0.0f, 1.0f}}, // 左下	
		{{1.0f,  -1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}, // 右下
	};
#pragma endregion


	// VertexBuffer(VertexResource, VertexBufferView)の作成
#pragma region VertexBuffer
	VertexBuffer vb;
	vb.Create(sizeof(vertices), sizeof(vertices[0]));

	// 頂点リソースにデータを書き込む
	VertexData* pGpuVertices = nullptr;
	vb.Get()->Map(0, nullptr, reinterpret_cast<void**>(&pGpuVertices)); // リソースをCPUから書き込めるようにマップする

	for (int i = 0; i < _countof(vertices); ++i) {
		pGpuVertices[i] = vertices[i]; // 頂点データをGPUリソースにコピー
	}
#pragma endregion

	//頂点インデックスデータの準備
#pragma region IndexData
	uint16_t indices[] = {
	    0, 1, 2, 2, 1 ,3// 頂点0、1、2を結ぶ三角形
	};
#pragma endregion

	// IndexBuffer(IndexResource, IndexBufferView)の作成
#pragma region IndexBuffer
	IndexBuffer ib;
	ib.Create(sizeof(indices), sizeof(indices[0]));

	// インデックスリソースにデータを書き込む
	uint16_t* pGpuIndices = nullptr;
	ib.Get()->Map(0, nullptr, reinterpret_cast<void**>(&pGpuIndices)); // リソースをCPUから書き込めるようにマップする
	
	for (int i = 0; i < _countof(indices); ++i) {
		pGpuIndices[i] = indices[i]; // インデックスデータをGPUリソースにコピー
	}
#pragma endregion

	// RenderTextureResourceの作成
#pragma region RenderTextureResource
	ID3D12Device* device = dxCommon->GetDevice();
	[[maybe_unused]] HRESULT hr;

	// 0.RenderTextureResourceの生成
	// 画面クリア色 わかりやすく赤にする
	const FLOAT kRenderTargetClearColor[4] = {1.0f, 0.0f, 0.0f, 1.0f};

	ID3D12Resource* renderTextureResource = CreateRenderTextureResource(
		device, WinApp::kWindowWidth, WinApp::kWindowHeight, 
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, kRenderTargetClearColor);

	// 1.RTV用の、DescriptorHeapを作成する
	ID3D12DescriptorHeap* rtvDescriptorHeap = nullptr;

	D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc{};
	rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // RTV用のDescriptorHeap
	rtvDescriptorHeapDesc.NumDescriptors = 1; // 描画対象は1つ

	hr = device->CreateDescriptorHeap(&rtvDescriptorHeapDesc,
		IID_PPV_ARGS(&rtvDescriptorHeap));
	assert(SUCCEEDED(hr));

	// CPU側からみたHANDLEを取得しておく
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandleCPU = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	// 2.RTV用のViewの生成
	device->CreateRenderTargetView(
	    renderTextureResource,	// 描画対象のリソース
	    nullptr,				// RTVの詳細設定（今回はデフォルトでいいのでnullptr）
								// ※RTVの場合 nullptrにするとDirectX12が自動で推測してくれる
	    rtvHandleCPU			// RTVを指すCPU側のハンドル
	);

#pragma endregion

	// DepthStencilTextureResourceの作成
#pragma region DepthStencilTextureResource
	// 0.DepthStencilTextureResourceの生成
	ID3D12Resource* depthStencilResource = CreateDepthStencilTextureResource(
		device, WinApp::kWindowWidth, WinApp::kWindowHeight);
	// 1.DSV用のDescriptorHeapを作成する
	ID3D12DescriptorHeap* dsvDescriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC dsvDescriptorHeapDesc{};
	dsvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV; // DSV用のDescriptorHeap
	dsvDescriptorHeapDesc.NumDescriptors = 1; // 深度ステンシルビューは1つ
	dsvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // シェーダーからアクセスしないのでフラグはなし

	hr = device->CreateDescriptorHeap(&dsvDescriptorHeapDesc,
		IID_PPV_ARGS(&dsvDescriptorHeap));
	assert(SUCCEEDED(hr));

	// CPU側からみたHANDLEを取得しておく
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandleCPU = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	// 2.DSV用のViewの生成
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT; // DepthStencilのフォーマットを指定
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D; // 2Dテクスチャとして利用する

	// DSVHeapの先頭にDSVを作る
	device->CreateDepthStencilView(
	    depthStencilResource, // 深度ステンシルリソース
	    &dsvDesc,             // DSVの詳細設定
	    dsvHandleCPU          // DSVを指すCPU側のハンドル
	);
#pragma endregion

	// SRV用のDescriptorHeapの生成
#pragma region SRVDescriptorHeap
	// 1.SRV用のDescriptorHeapを作成する
	ID3D12DescriptorHeap* srvDescriptorHeap = nullptr;

	D3D12_DESCRIPTOR_HEAP_DESC srvDescriptorHeapDesc{};
	srvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;		// SRV用のDescriptorHeap
	srvDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;	// シェーダーから見えるようにするフラグ
	srvDescriptorHeapDesc.NumDescriptors = 1;									// SRVは1つ

	hr = device->CreateDescriptorHeap(&srvDescriptorHeapDesc, IID_PPV_ARGS(&srvDescriptorHeap));
	assert(SUCCEEDED(hr));

	// CPU側からみたHANDLE、GPU側から見たHANDLEを取得しておく
	D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU = srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU = srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

	// 2.SRVの生成
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // RenderTextureResourceと同じにする
	srvDesc.Shader4ComponentMapping 
		= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // RGBA値をそのままシェーダーに対応させる
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャとして利用する
	srvDesc.Texture2D.MipLevels = 1;                       // mipmapは1しかない

	device->CreateShaderResourceView(
	    renderTextureResource, // viewと関連付けたいリソース
	    &srvDesc,              // SRVの詳細設定(Desc:Description、構成内容の記述)
	    srvHandleCPU           // SRV用ディスクリプタヒープのCPU側のハンドル
	);
#pragma endregion

	// Depth用のSRVの生成
#pragma region DepthSRV



	//D3D12_SHADER_RESOURCE_VIEW_DESC depthTextureSRVDesc{};
	//// DXGI_FORMAT_D24_UNORM_S8_UINTのDepthを読むときはDXGI_FORMAT_R24_UNORM_X8_TYPELESSに変換する
	//depthTextureSRVDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; // Depthを読むときのフォーマット
	//depthTextureSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // RGBA値をそのままシェーダーに対応させる
	//depthTextureSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;                      // 2Dテクスチャとして利用する
	//depthTextureSRVDesc.Texture2D.MipLevels = 1;                                            // mipmapは1しかない
	//device->CreateShaderResourceView(
	//    depthStencilResource, // viewと関連付けたいリソース
	//    &depthTextureSRVDesc, // SRVの詳細設定(Desc:Description、構成内容の記述)
	//	// 新しいdescriptorHandle
	//
	//    srvHandleCPU // SRV用ディスクリプタヒープのCPU側のハンドル
	//);

#pragma endregion


	// モデル関係
#pragma region モデル関係
	// アプリで使用する3Dモデル
	// 被写体の準備
	Model* model = Model::CreateFromOBJ("terrain");

	WorldTransformEx worldTransform;
	worldTransform.Initialize();
	worldTransform.scale_ = Vector3(1.0f, 1.0f, 1.0f);

	// カメラの準備
	Camera camera;
	camera.Initialize();
	camera.translation_ = Vector3(0.0f, 1.0f, 0.0f);
#pragma endregion


	// ゲームシーンのインスタンス生成
	GameScene* gameScene = new GameScene();
	// ゲームシーンの初期化
	gameScene->Initialize();


	// メインループ
	while (true) {
	// エンジンの処理
		if (Update()) {
			// 終了条件を満たした場合はループを抜ける
			break;
		}


		// world変換行列の定数バッファへの転送
		worldTransform.rotation_.y += 0.005f; // Y軸回りに回転させる
		worldTransform.UpdateMatrix();        // ワールド変換行列の更新

		// cameraの更新と定数バッファへの転送
		camera.UpdateMatrix(); // カメラのビュー行列の更新

		

		// ゲームシーンの更新
		gameScene->Update();

		imguiManager_->Begin();

		ImGui::Begin("PostEffect");

		const char* items[] =
		{
			"Normal",
			"GrayScale",
			"Vignette",
			"Sepia",
			"BoxFilter",
			"GaussianFilter", 
			"RadialBlur",
		    "Outline"
		};

		ImGui::Combo("Effect", &effectType, items, IM_ARRAYSIZE(items));

		ImGui::End();

		imguiManager_->End();

		// TransitionBarrierを SRV => RTV に設定する
#pragma region TransitionBarrierの変換
		D3D12_RESOURCE_BARRIER barrier{};// TransitionBarrierの設定
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION; // TransitionBarrierであることを示す
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;      // バリアのオプション設定
		barrier.Transition.pResource = renderTextureResource;  // バリアをかけるリソース
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE; // バリア前のリソースの状態
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;          // バリア後のリソースの状態
		commandList->ResourceBarrier(1, &barrier);                                   // バリアの発行

		// 描画先のRTVとDSVを設定する
		commandList->OMSetRenderTargets(1, &rtvHandleCPU, FALSE, &dsvHandleCPU); // 描画先のRTVとDSVを設定する

		// Viewportの設定
		D3D12_VIEWPORT viewport{};
		viewport.Width = WinApp::kWindowWidth;
		viewport.Height = WinApp::kWindowHeight;
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		commandList->RSSetViewports(1, &viewport); // Viewportの設定

		// Scissorの設定
		D3D12_RECT scissorRect{};
		// 基本的にビューポートと同じ矩形が構成されるようにする
		scissorRect.left = 0;
		scissorRect.right = WinApp::kWindowWidth;
		scissorRect.top = 0;
		scissorRect.bottom = WinApp::kWindowHeight;

		commandList->RSSetScissorRects(1, &scissorRect); // Scissorの設定

		// 全画面クリア
		commandList->ClearRenderTargetView(rtvHandleCPU, kRenderTargetClearColor, 0, nullptr); // RTVのクリア
		//指定した深度で画面全体をクリアする
		commandList->ClearDepthStencilView(dsvHandleCPU, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr); // DSVのクリア

		// 描画
		Model::PreDraw();
		model->Draw(worldTransform, camera);
		Model::PostDraw();

		// TransitionBarrierを元に戻し、PixelShaderが扱えるようにする
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION; // TransitionBarrierの設定
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;      // バリアのオプション設定
		barrier.Transition.pResource = renderTextureResource;  // バリアをかけるリソース
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;        // バリア前のリソースの状態
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE; // バリア後のリソースの状態
		commandList->ResourceBarrier(1, &barrier);                                  // バリアの発行

#pragma endregion

		imguiManager_->Draw();

		// 描画開始
		dxCommon->PreDraw();

		// ゲームシーンの描画
		gameScene->Draw();

		// コマンドを読む
		commandList->SetGraphicsRootSignature(rs.Get()); // ルートシグネチャの設定
		switch (effectType) {
		case 0:
			commandList->SetPipelineState(normalPSO.Get());
			break;

		case 1:
			commandList->SetPipelineState(grayPSO.Get());
			break;

		case 2:
			commandList->SetPipelineState(vignettePSO.Get());
			break;

		case 3:
			commandList->SetPipelineState(sepiaPSO.Get());
			break;

		case 4:
			commandList->SetPipelineState(boxFilterPSO.Get());
			break;

		case 5:
			commandList->SetPipelineState(gaussianFilterPSO.Get());
			break;

		case 6:
			commandList->SetPipelineState(radialBlurPSO.Get());
			break;
		case 7:
			commandList->SetPipelineState(outlinePSO.Get());
			break;

		} 
		
		// PSOの設定
		commandList->IASetVertexBuffers(0, 1, vb.GetView()); // 頂点バッファビューの設定
		commandList->IASetIndexBuffer(ib.GetView()); // インデックスバッファビューの設定
		// トポロジの設定
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // トポロジの設定（三角形リスト）
		
		// 使用するDescriptorHeapの設定
		commandList->SetDescriptorHeaps(srvDescriptorHeap->GetDesc().NumDescriptors, &srvDescriptorHeap);

		// SRVのDescripterTableの先頭を設定
		commandList->SetGraphicsRootDescriptorTable(0, srvHandleGPU); 
		
		// 頂点数、インデックス数、インデックスの開放位置、インデックスのオフセット
		commandList->DrawIndexedInstanced(_countof(indices), 1, 0, 0, 0); // DrawIndexedInstanced(インデックス数, インスタンス数, 開始インデックス位置, 開始頂点位置, 開始インスタンス位置)

		// 描画終了
		dxCommon->PostDraw();

		//imguiManager_->Finalize();
	}


	// 解放処理
	delete gameScene;
	delete model;

	renderTextureResource->Release();
	srvDescriptorHeap->Release();
	rtvDescriptorHeap->Release();

	depthStencilResource->Release();
	dsvDescriptorHeap->Release();

	// エンジンの終了処理
	Finalize();



	// nullptrの代入
	gameScene = nullptr;



	return 0;
}
