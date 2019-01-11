# Direct3D12-1-初始化
一些简单的初始化工作
1. 窗口创建
```
int __stdcall wWinMain(HISNTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, INT nCmdShow)
{
    return 0;
}
```
2. 创建Render Target（渲染目标）
```
D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
for (UINT i = 0; i < FrameCount; i++)
{
    m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i]));
    m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
    rtvHandle.ptr += m_rtvDescriptorSize;

    m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[i]));
}
```
3. 复制顶点数据
4. 使用CommandList记录(Record)绘制命令 
5. 渲染、呈现
