#include <Windows.h>
#include "KamataEngine.h"
#include "GameScene.h"
#include <cassert>
#include "Shader.h"
#include "RootSignature.h"
#include "PipelineState.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"

using namespace KamataEngine;

// 関数プロトタイプ宣言
void SetupPipelineState(PipelineState& pipelineState, RootSignature& rs, Shader& vs, Shader& ps) {
	// InputLayoutの作成
#pragma region InputLayout
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[1] = {};
	inputElementDescs[0].SemanticName = "POSITION";                        // 頂点の意味を示す文字列
	inputElementDescs[0].SemanticIndex = 0;                                // 同じ意味の頂点が複数ある場合の識別番号
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;          // データの形式
	inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT; // 頂点データ内のオフセット
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

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {
	// 初期化処理
	Initialize(L"LE3D_14_タケウチ_ハルカ_CG5");

	// DirectXCommonインスタンスの取得
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

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
	    Shader ps;
	    ps.LoadDxc(L"Resources/Shaders/TestPS.hlsl", L"ps_6_0");
	    assert(ps.GetDxcBlob() != nullptr);
#pragma endregion


	// PipelineStateの作成
#pragma region PSO
	    PipelineState pipelineState;
	    SetupPipelineState(pipelineState, rs, vs, ps);
#pragma endregion


// リソースの確保を含め、頂点情報を柔軟に対応できるようにVerteData構造体を新たに作成する
#pragma region VerteData構造体
	// Vertex4 > VertexData に変更して利用する
	struct VertexData {
		Vector4 position; // 頂点の位置
	};

	// 頂点データの準備
	VertexData vertices[] = {
		{-1.0f,  1.0f,  0.0f, 1.0f}, // 左上
	    {1.0f,  1.0f,  0.0f, 1.0f}, // 右上
	    {-1.0f, -1.0f,  0.0f, 1.0f}, // 左下	
		{1.0f,  -1.0f, 0.0f, 1.0f}, // 右下
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

		// ゲームシーンの更新
		gameScene->Update();


		// 描画開始
		dxCommon->PreDraw();

		// ゲームシーンの描画
		gameScene->Draw();

		// コマンドを読む
		commandList->SetGraphicsRootSignature(rs.Get()); // ルートシグネチャの設定
		commandList->SetPipelineState(pipelineState.Get());         // PSOの設定
		commandList->IASetVertexBuffers(0, 1, vb.GetView()); // 頂点バッファビューの設定
		commandList->IASetIndexBuffer(ib.GetView()); // インデックスバッファビューの設定
		// トポロジの設定
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // トポロジの設定（三角形リスト）
		// 頂点数、インデックス数、インデックスの開放位置、インデックスのオフセット
		commandList->DrawIndexedInstanced(_countof(indices), 1, 0, 0, 0); // DrawIndexedInstanced(インデックス数, インスタンス数, 開始インデックス位置, 開始頂点位置, 開始インスタンス位置)


		// 描画終了
		dxCommon->PostDraw();
	}


	// 解放処理
	//delete gameScene;


	// エンジンの終了処理
	Finalize();



	// nullptrの代入
	gameScene = nullptr;



	return 0;
}
