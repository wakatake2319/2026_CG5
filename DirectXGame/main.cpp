#include <Windows.h>
#include "KamataEngine.h"
#include "GameScene.h"
#include <cassert>
#include <d3dcompiler.h>

using namespace KamataEngine;

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
	// 構造体にデータを用意する
	D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	ID3DBlob* signatureBlob = nullptr; // シグネチャのバイナリデータ
	ID3DBlob* errorBlog = nullptr;     // エラーのバイナリデ
	HRESULT hr = D3D12SerializeRootSignature(&descriptionRootSignature, 
		D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlog);
	if (FAILED(hr)) {
		DebugText::GetInstance()->ConsolePrintf(reinterpret_cast<char*>(errorBlog->GetBufferPointer()));
		// 失敗
		assert(false);
	}
	// バイナリをもとに生成
	ID3D12RootSignature* rootSignature = nullptr;
	hr = dxCommon->GetDevice()->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	assert(SUCCEEDED(hr));
#pragma endregion

	// InputLayoutの作成
#pragma region InputLayout
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[1] = {};
	inputElementDescs[0].SemanticName = "POSITION"; // 頂点の意味を示す文字列
	inputElementDescs[0].SemanticIndex = 0;                       // 同じ意味の頂点が複数ある場合の識別番号
	inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT; // データの形式
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

	// VertetxShaderをCompileする
#pragma region VertexShader
	    ID3DBlob* vsBlob = nullptr; // 頂点シェーダーをコンパイルする
	    ID3DBlob* psBlob = nullptr; // ピクセルシェーダーをコンパイルする
	    ID3D10Blob* errorBlob = nullptr; // エラーオブジェクト

		// 頂点シェーダーの読み込みとコンパイル
	    std::wstring vsFile = L"Resources/Shaders/TestVS.hlsl";
	    hr = D3DCompileFromFile(
	        vsFile.c_str(), // シェーダーファイル名
			nullptr, 
			D3D_COMPILE_STANDARD_FILE_INCLUDE, // インクルード可能にする
	        "main", "vs_5_0",                  // エントリーポイント名、シェーダーモデル指定
	        D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグ用のフラグ
			0, &vsBlob, &errorBlob);

		if (FAILED(hr)) {
		    DebugText::GetInstance()->ConsolePrintf(
				std::system_category().message(hr).c_str());
		    if (errorBlob) {
			DebugText::GetInstance()->ConsolePrintf(
				reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
			}
		    assert(false);
	    }
#pragma endregion

	// PixelShaderをCompileする
#pragma region PixelShader
		// ピクセルシェーダーの読み込みとコンパイル
		std::wstring psFile = L"Resources/Shaders/TestPS.hlsl";
		hr = D3DCompileFromFile(
			psFile.c_str(), // シェーダーファイル名
			nullptr, 
			D3D_COMPILE_STANDARD_FILE_INCLUDE, // インクルード可能にする
			"main", "ps_5_0",                  // エントリーポイント名、シェーダーモデル指定
			D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグ用のフラグ
	        0, &psBlob, &errorBlob);
	    if (FAILED(hr)) {
			DebugText::GetInstance()->ConsolePrintf(
				std::system_category().message(hr).c_str());
		    if (errorBlob) {
				DebugText::GetInstance()->ConsolePrintf(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
			}
		    assert(false);
	    }
#pragma endregion

	// PSOの作成
#pragma region PSO
	    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	    graphicsPipelineStateDesc.pRootSignature = rootSignature; // ルートシグネチャ
	    graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;  // InputLayout
	    graphicsPipelineStateDesc.VS = {vsBlob->GetBufferPointer(), vsBlob->GetBufferSize()}; // 頂点シェーダー
		graphicsPipelineStateDesc.PS = {psBlob->GetBufferPointer(), psBlob->GetBufferSize()}; // ピクセルシェーダー
		graphicsPipelineStateDesc.BlendState = blendDesc;        // BlendState
		graphicsPipelineStateDesc.RasterizerState = rasterizerDesc; // RasterizerState
		
		// 書き込むRTVの情報
		graphicsPipelineStateDesc.NumRenderTargets = 1;         // 描画対象は1つ
		graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // 描画対象のフォーマットを指定

		// 利用するリポジトリ
		graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; // 描画する図形の形状を三角形にする

		// どのように画面に色を打ち込むかの設定
	    graphicsPipelineStateDesc.SampleDesc.Count = 1;                       // マルチサンプリングしない
		graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;        // 全てのサンプルを有効にする

		// PSOの生成
	    ID3D12PipelineState* pipelineState = nullptr;
	    hr = dxCommon->GetDevice()->CreateGraphicsPipelineState(
			&graphicsPipelineStateDesc, IID_PPV_ARGS(&pipelineState));
	    assert(SUCCEEDED(hr));
#pragma endregion

	// VertexResourceの作成
#pragma region VertexResource
		// 頂点リソース用のヒープの設定
	    D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	    uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;		// CPUから書き込むヒープ

		// 頂点リソースの設定
		D3D12_RESOURCE_DESC vertexResourceDesc{};
		vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; // バッファリソースであることを示す
		vertexResourceDesc.Width = sizeof(Vector4) * 3; // 頂点1つあたり4要素(位置)×4バイト(32ビット)×3頂点分
		
		// バッファの場合はこれらは1にする決まり
		vertexResourceDesc.Height = 1;          // バッファリソースなので高さは1
		vertexResourceDesc.DepthOrArraySize = 1; // バッファリソースなので深さは1
		vertexResourceDesc.MipLevels = 1;       // バッファリソースなのでミップマップレベルは1
		vertexResourceDesc.SampleDesc.Count = 1; // マルチサンプリングしない
	    
		// バッファの場合はこれにする決まり
		vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; // 行優先のメモリ配置

		// 実際にリソースを生成する
	    ID3D12Resource* vertexResource = nullptr;
		hr = dxCommon->GetDevice()->CreateCommittedResource(
			&uploadHeapProperties, // アップロードヒープを指定
			D3D12_HEAP_FLAG_NONE, // ヒープフラグ
			&vertexResourceDesc,  // リソースの詳細
			D3D12_RESOURCE_STATE_GENERIC_READ, // リソースの使用状態
			nullptr,              // 最適化されたクリア値（バッファリソースなのでnullptr）
			IID_PPV_ARGS(&vertexResource)); // 生成したリソースへのポインタを受け取る
	    assert(SUCCEEDED(hr));
#pragma endregion

	// VertexBufferViewの作成
#pragma region VertexBufferView
		D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
		// リソースの先頭アドレスから使う
		vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress(); // 頂点リソースのGPU仮想アドレス
		// 使用するリソースのサイズは3つ分のサイズ
		vertexBufferView.SizeInBytes = sizeof(Vector4) * 3; // 頂点リソースのサイズ
	    // 1つの頂点のサイズ
		vertexBufferView.StrideInBytes = sizeof(Vector4);                         // 頂点1つあたりのサイズ
#pragma endregion

	// Resourceにデータを書き込む
#pragma region WriteVertexData
	    Vector4* vertexData = nullptr;
	    vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData)); // リソースをCPUから書き込めるようにマップする
	    vertexData[0] = {-0.5f, -0.5f, 0.0f, 1.0f};                               // 頂点1の位置
	    vertexData[1] = {0.0f, 0.5f, 0.0f, 1.0f};                               // 頂点2の位置
		vertexData[2] = {0.5f, -0.5f, 0.0f, 1.0f};                               // 頂点3の位置
		// 頂点リソースのマップを解除する
	    vertexResource->Unmap(0, nullptr);
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
		commandList->SetGraphicsRootSignature(rootSignature); // ルートシグネチャの設定
		commandList->SetPipelineState(pipelineState);         // PSOの設定
		commandList->IASetVertexBuffers(0, 1, &vertexBufferView); // 頂点バッファビューの設定

		// トポロジの設定
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // トポロジの設定（三角形リスト）
		// 頂点数、インデックス数、インデックスの開放位置、インデックスのオフセット
		commandList->DrawInstanced(3, 1, 0, 0); // DrawInstanced(頂点数, インスタンス数, 開始頂点位置, 開始インスタンス位置)



		// 描画終了
		dxCommon->PostDraw();
	}


	// 解放処理
	delete gameScene;

	vertexResource->Release();
	pipelineState->Release();
	signatureBlob->Release();
	if (errorBlob) {
		errorBlob->Release();
	}
	rootSignature->Release();
	vsBlob->Release();
	psBlob->Release();

	// エンジンの終了処理
	Finalize();



	// nullptrの代入
	gameScene = nullptr;



	return 0;
}

// シェーダーコンパイル関数
// filePath:	シェーダーファイルのパス		例: L"Resources/Shaders/TestVS.hlsl"
// shaderModel: シェーダーモデル			例: "vs_5_0"
ID3DBlob* CompileShader(const std::wstring& filePath, const std::string& shaderModel)
{
	ID3DBlob* shaderBlob = nullptr;
	ID3DBlob* errorBlob = nullptr;

	HRESULT hr = D3DCompileFromFile(
		filePath.c_str(), // シェーダーファイル名
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE, // インクルード可能にする
		"main", shaderModel.c_str(),       // エントリーポイント名、シェーダーモデル指定
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, // デバッグ用のフラグ
	    0, &shaderBlob, &errorBlob);

	// エラーが発生した場合、止める
	if (FAILED(hr)) {
		if (errorBlob) {
			OutputDebugStringA(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
			errorBlob->Release();
		}
		assert(false);
	}
	// 発生したshaderBlobを返す
	return shaderBlob;
}