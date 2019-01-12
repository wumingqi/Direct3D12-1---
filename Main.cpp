#include "stdafx.h"

HRESULT hr;

struct Vertex
{
	XMFLOAT3 position;
	XMFLOAT4 color;
};

class D3D12Application
{
	static const UINT FrameCount = 2U;
	UINT m_frameIndex;

	ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	UINT m_rtvDescriptorSize;
	float m_clearColor[4] = { 0.f,0.f,0.f,1.f };	//背景色

	ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	UINT m_vertexCount;
	//D3D12_PRIMITIVE_TOPOLOGY_TYPE m_priTopoType = 
	//	D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	//D3D_PRIMITIVE_TOPOLOGY m_priTopo =
	//	D3D_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;

	D3D12_PRIMITIVE_TOPOLOGY_TYPE m_priTopoType =
		D3D12_PRIMITIVE_TOPOLOGY_TYPE::D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	D3D_PRIMITIVE_TOPOLOGY m_priTopo =
		D3D_PRIMITIVE_TOPOLOGY::D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	ComPtr<ID3D12Device> m_device;
	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<ID3D12GraphicsCommandList> m_commandList;
	ComPtr<ID3D12CommandAllocator> m_commandAllocators[FrameCount];
	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12PipelineState> m_pipelineState;

	ComPtr<ID3D12Fence> m_fence;
	UINT64 m_fenceValues[FrameCount];
	HANDLE m_fenceEvent;

	ComPtr<IDXGISwapChain4> m_swapChain;

private:
	void OnInit()
	{
		LoadPipeline();
		LoadAssets();
/*
		SYSTEMTIME sysTime;
		GetLocalTime(&sysTime);
		srand(sysTime.wMilliseconds);
		for (auto &color : m_clearColor)
		{
			color = rand() % 256 / 255.f;
		}*/
	}

	void LoadPipeline()
	{
		UINT dxgiFlags = 0;
#ifdef _DEBUG
		ComPtr<ID3D12Debug> debugController;
		D3D12GetDebugInterface(IID_PPV_ARGS(&debugController));
		debugController->EnableDebugLayer();

		dxgiFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif // _DEBUG
		ComPtr<IDXGIFactory7> dxgiFactory;
		CreateDXGIFactory2(dxgiFlags, IID_PPV_ARGS(&dxgiFactory));

		ComPtr<IDXGIAdapter> adapter;
		dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&adapter));

		D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_device));

		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue));

		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.Width = m_width;
		swapChainDesc.Height = m_height;
		swapChainDesc.BufferCount = FrameCount;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.Scaling = DXGI_SCALING_NONE;
		swapChainDesc.SampleDesc.Count = 1;

		ComPtr<IDXGISwapChain1> swapChain;
		dxgiFactory->CreateSwapChainForHwnd(m_commandQueue.Get(), m_hWnd, &swapChainDesc, nullptr, nullptr, &swapChain);
		swapChain.As(&m_swapChain);
		m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

		//创建描述符堆
		{
			D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
			rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			rtvHeapDesc.NumDescriptors = FrameCount;
			rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));

			m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		}

		{
			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
			for (UINT i = 0; i < FrameCount; i++)
			{
				m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i]));
				m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
				rtvHandle.ptr += m_rtvDescriptorSize;

				m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[i]));
			}
		}
	}

	void LoadAssets()
	{
		//创建Root Signature
		{
			D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
			rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

			ComPtr<ID3DBlob> signature, error;
			D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);

			m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature));
		}
		//PSO
		{
			ComPtr<ID3DBlob> vertexShader, pixelShader;
#if defined(_DEBUG)
			UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
			UINT compileFlags = 0;
#endif
			auto filename = Utility::GetModulePath().append(L"Shaders.hlsl");
			D3DCompileFromFile(filename.c_str(), nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr);
			D3DCompileFromFile(filename.c_str(), nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr);

			D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
			{
				{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
				{"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
			psoDesc.InputLayout = { inputElementDescs,_countof(inputElementDescs) };
			psoDesc.pRootSignature = m_rootSignature.Get();
			psoDesc.VS = { vertexShader->GetBufferPointer(),vertexShader->GetBufferSize() };
			psoDesc.PS = { pixelShader->GetBufferPointer(),pixelShader->GetBufferSize() };
			psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			psoDesc.DepthStencilState.DepthEnable = FALSE;
			psoDesc.DepthStencilState.StencilEnable = FALSE;
			psoDesc.SampleMask = UINT_MAX;
			psoDesc.PrimitiveTopologyType = m_priTopoType;
			psoDesc.NumRenderTargets = 1;
			psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
			psoDesc.SampleDesc.Count = 1;
			m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState));
		}

		m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[m_frameIndex].Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandList));
		m_commandList->Close();

		//创建顶点数据
		{
			float radius = 0.5f;
			//顶点的顺序必须是按照顺时针排列
			Vertex vertices[] =
			{
				{ { radius, radius*m_aspectRatio, 0.f},		{ 1.0f, 0.0f, 0.0f, 1.0f} },	//v0
				{ { -radius, -radius*m_aspectRatio, 0.f},	{ 0.0f, 0.0f, 1.0f, 1.0f} },	//v2
				{ { -radius, radius*m_aspectRatio, 0.f},	{ 0.0f, 1.0f, 0.0f, 1.0f} },	//v1
				{ { radius, -radius*m_aspectRatio, 0},		{ 0.0f, 1.0f, 1.0f, 1.0f} },	//v3
				{ { -radius, -radius*m_aspectRatio, 0.f},	{ 0.0f, 0.0f, 1.0f, 1.0f} },	//v2
				{ { radius, radius*m_aspectRatio, 0.f},		{ 1.0f, 0.0f, 0.0f, 1.0f} },	//v0
			};
			m_vertexCount = _countof(vertices);

			const UINT vertexBufferSize = sizeof(vertices);
			m_device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&m_vertexBuffer));

			UINT8* pVertexBufferBegin;
			D3D12_RANGE readRange = { 0, 0 };
			m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexBufferBegin));
			memcpy(pVertexBufferBegin, vertices, vertexBufferSize);
			m_vertexBuffer->Unmap(0, nullptr);

			m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
			m_vertexBufferView.StrideInBytes = sizeof(Vertex);
			m_vertexBufferView.SizeInBytes = vertexBufferSize;
		}
		//
		{
			m_device->CreateFence(m_fenceValues[m_frameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
			m_fenceValues[m_frameIndex]++;
			m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			WaitForGpu();
		}
	}

	void OnUpdate()
	{

	}

	void OnRender()
	{
		PopulateCommandList();
		ID3D12CommandList* ppCmdLists[] = { m_commandList.Get() };
		m_commandQueue->ExecuteCommandLists(_countof(ppCmdLists), ppCmdLists);
		m_swapChain->Present(1, 0);

		MoveToNextFrame();
	}

	void PopulateCommandList()
	{
		m_commandAllocators[m_frameIndex]->Reset();
		m_commandList->Reset(m_commandAllocators[m_frameIndex].Get(), m_pipelineState.Get());

		//
		m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
		m_commandList->RSSetViewports(1, &m_viewport);
		m_commandList->RSSetScissorRects(1, &m_scissorRect);

		m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = { m_rtvHeap->GetCPUDescriptorHandleForHeapStart().ptr + (m_frameIndex* m_rtvDescriptorSize) };
		m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

		m_commandList->ClearRenderTargetView(rtvHandle, m_clearColor, 0, nullptr);
		m_commandList->IASetPrimitiveTopology(m_priTopo);
		m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
		m_commandList->DrawInstanced(m_vertexCount, 1, 0, 0);

		m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[m_frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

		m_commandList->Close();
	}

	void OnDestroy()
	{
		WaitForGpu();
		CloseHandle(m_fenceEvent);
	}

	void WaitForGpu()
	{
		m_commandQueue->Signal(m_fence.Get(), m_fenceValues[m_frameIndex]);
		m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent);
		WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
		m_fenceValues[m_frameIndex]++;
	}

	void MoveToNextFrame()
	{
		const UINT64 currentFenceValue = m_fenceValues[m_frameIndex];
		m_commandQueue->Signal(m_fence.Get(), currentFenceValue);
		m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
		if (m_fence->GetCompletedValue() < m_fenceValues[m_frameIndex])
		{
			m_fence->SetEventOnCompletion(m_fenceValues[m_frameIndex], m_fenceEvent);
			WaitForSingleObjectEx(m_fenceEvent, INFINITE, FALSE);
		}
		m_fenceValues[m_frameIndex] = currentFenceValue + 1;
	}

private:
	HINSTANCE m_hInstance;
	HWND m_hWnd;
	UINT m_width, m_height;
	float m_aspectRatio;
	D3D12_VIEWPORT m_viewport;
	D3D12_RECT m_scissorRect;

	static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		auto app = reinterpret_cast<D3D12Application*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
		switch (msg)
		{
		case WM_PAINT:
			app->OnUpdate();
			app->OnRender();
			break;
		case WM_DESTROY:
			app->OnDestroy();
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProc(hWnd, msg, wParam, lParam);
		}
		return 0;
	}
public:
	D3D12Application(UINT width, UINT height, HINSTANCE hInstance = nullptr) :
		m_hInstance(hInstance),
		m_width(width),
		m_height(height),
		m_aspectRatio(static_cast<float>(width) / static_cast<float>(height)),
		m_fenceValues{},
		m_viewport{ 0.f,0.f,static_cast<float>(width) , static_cast<float>(height), 0.f,1.f },
		m_scissorRect{0,0,static_cast<LONG>(width) , static_cast<LONG>(height) }
	{}

	int Run(int nCmdShow)
	{
		WNDCLASSEX wc = { sizeof(WNDCLASSEX) };
		wc.style = CS_VREDRAW | CS_HREDRAW;
		wc.lpfnWndProc = WndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = m_hInstance;
		wc.hIcon = LoadIcon(m_hInstance, IDI_APPLICATION);
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
		wc.lpszMenuName = nullptr;
		wc.lpszClassName = L"HelloDirect3D";
		wc.hIconSm = nullptr;

		RegisterClassEx(&wc);

		LONG clientWidth = 640; LONG clientHeight = 480;
		DWORD style = WS_OVERLAPPEDWINDOW; DWORD styleEx = 0;
		RECT rc = { 0,0,clientWidth,clientHeight };
		AdjustWindowRectEx(&rc, style, false, styleEx);
		auto cx = GetSystemMetrics(SM_CXSCREEN);
		auto cy = GetSystemMetrics(SM_CYFULLSCREEN);
		auto w = rc.right - rc.left;
		auto h = rc.bottom - rc.top;
		auto x = (cx - w) / 2;
		auto y = (cy - h) / 2;

		m_hWnd = CreateWindowEx(WS_EX_NOREDIRECTIONBITMAP, wc.lpszClassName, L"Direct3D 12初始化", WS_OVERLAPPEDWINDOW,
			x, y, w, h, nullptr, nullptr, m_hInstance, nullptr);

		SetWindowLongPtr(m_hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
		OnInit();

		ShowWindow(m_hWnd, nCmdShow);
		MSG msg = {};
		while (GetMessage(&msg, nullptr, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		return static_cast<int>(msg.wParam);
	}
};

int __stdcall wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	return D3D12Application(640, 480, hInstance).Run(nCmdShow);
}
