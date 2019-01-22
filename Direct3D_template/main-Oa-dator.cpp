//--------------------------------------------------------------------------------------
// BTH - Stefan Petersson 2014.
//	   - modified by FLL
//--------------------------------------------------------------------------------------
#include <windows.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

using namespace DirectX;

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3dcompiler.lib")

HWND InitWindow(HINSTANCE hInstance);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

HRESULT CreateDirect3DContext(HWND wndHandle);

#define WIDTH 768.0f
#define HEIGHT 768.0f

// Most directX Objects are COM Interfaces
// https://es.wikipedia.org/wiki/Component_Object_Model
IDXGISwapChain* gSwapChain = nullptr;

// Device and DeviceContext are the most common objects to
// instruct the API what to do. It is handy to have a reference
// to them available for simple applications.
ID3D11Device* gDevice = nullptr;
ID3D11DeviceContext* gDeviceContext = nullptr;

// A "view" of a particular resource (the color buffer)
ID3D11RenderTargetView* gBackbufferRTV = nullptr;

// a resource to store Vertices in the GPU
ID3D11Buffer* gVertexBuffer = nullptr;

ID3D11Buffer* gConstantBuffer = nullptr;

ID3D11InputLayout* gVertexLayout = nullptr;

// resources that represent shaders
ID3D11VertexShader* gVertexShader = nullptr;
ID3D11PixelShader* gPixelShader = nullptr;

float gFloat = 1.0f;
float dist = 0.0f;
float gClearColour[3] = {};

struct PerFrameMatrices {
	XMMATRIX worldMatrix;
};
PerFrameMatrices* gMatricesPerFrame = NULL;
ID3D11Buffer* gMatrixPerFrameBuffer = NULL;

HRESULT CreateShaders()
{
	// Binary Large OBject (BLOB), for compiled shader, and errors.
	ID3DBlob* pVS = nullptr;
	ID3DBlob* errorBlob = nullptr;

	// https://msdn.microsoft.com/en-us/library/windows/desktop/hh968107(v=vs.85).aspx
	HRESULT result = D3DCompileFromFile(
		L"Vertex.hlsl", // filename
		nullptr,		// optional macros
		nullptr,		// optional include files
		"VS_main",		// entry point
		"vs_5_0",		// shader model (target)
		D3DCOMPILE_DEBUG,	// shader compile options (DEBUGGING)
		0,				// IGNORE...DEPRECATED.
		&pVS,			// double pointer to ID3DBlob		
		&errorBlob		// pointer for Error Blob messages.
	);

	// compilation failed?
	if (FAILED(result))
	{
		if (errorBlob)
		{
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
			// release "reference" to errorBlob interface object
			errorBlob->Release();
		}
		if (pVS)
			pVS->Release();
		return result;
	}

	gDevice->CreateVertexShader(
		pVS->GetBufferPointer(), 
		pVS->GetBufferSize(), 
		nullptr, 
		&gVertexShader
	);
	
	// create input layout (verified using vertex shader)
	// Press F1 in Visual Studio with the cursor over the datatype to jump
	// to the documentation online!
	// please read:
	// https://msdn.microsoft.com/en-us/library/windows/desktop/bb205117(v=vs.85).aspx
	D3D11_INPUT_ELEMENT_DESC inputDesc[] = {
		{ 
			"POSITION",		// "semantic" name in shader
			0,				// "semantic" index (not used)
			DXGI_FORMAT_R32G32B32_FLOAT, // size of ONE element (3 floats)
			0,							 // input slot
			0,							 // offset of first element
			D3D11_INPUT_PER_VERTEX_DATA, // specify data PER vertex
			0							 // used for INSTANCING (ignore)
		},
		{ 
			"COLOR", 
			0,				// same slot as previous (same vertexBuffer)
			DXGI_FORMAT_R32G32B32_FLOAT, 
			0, 
			12,							// offset of FIRST element (after POSITION)
			D3D11_INPUT_PER_VERTEX_DATA, 
			0 
		},
	};

	gDevice->CreateInputLayout(inputDesc, ARRAYSIZE(inputDesc), pVS->GetBufferPointer(), pVS->GetBufferSize(), &gVertexLayout);

	// we do not need anymore this COM object, so we release it.
	pVS->Release();

	//create pixel shader
	ID3DBlob* pPS = nullptr;
	if (errorBlob) errorBlob->Release();
	errorBlob = nullptr;

	result = D3DCompileFromFile(
		L"Fragment.hlsl", // filename
		nullptr,		// optional macros
		nullptr,		// optional include files
		"PS_main",		// entry point
		"ps_5_0",		// shader model (target)
		D3DCOMPILE_DEBUG,	// shader compile options
		0,				// effect compile options
		&pPS,			// double pointer to ID3DBlob		
		&errorBlob			// pointer for Error Blob messages.
	);

	// compilation failed?
	if (FAILED(result))
	{
		if (errorBlob)
		{
			OutputDebugStringA((char*)errorBlob->GetBufferPointer());
			// release "reference" to errorBlob interface object
			errorBlob->Release();
		}
		if (pPS)
			pPS->Release();
		return result;
	}

	gDevice->CreatePixelShader(pPS->GetBufferPointer(), pPS->GetBufferSize(), nullptr, &gPixelShader);
	// we do not need anymore this COM object, so we release it.
	pPS->Release();

	return S_OK;
}

void CreateTriangleData()
{
	struct TriangleVertex
	{
		float x, y, z;
		float r, g, b;
	};

	// Array of Structs (AoS)
	TriangleVertex triangleVertices[6] =
	{
		-0.5f, 0.5f, 0.0f,	//v0 pos
		0.0f, 0.0f, 0.0f,	//v0 color

		0.5f, -0.5f, 0.0f,	//v1
		1.0f, 1.0f, 0.0f,	//v1 color

		-0.5f, -0.5f, 0.0f, //v2
		0.0f, 1.0f, 0.0f,	//v2 color

		-0.5f, 0.5f, 0.0f,	//v3 pos
		0.0f, 0.0f, 0.0f,	//v3 color

		0.5f, 0.5f, 0.0f,	//v4 pos
		1.0f, 0.0f, 0.0f,	//v4 color

		0.5f, -0.5f, 0.0f,	//v5 pos
		1.0f, 1.0f, 0.0f,	//v5 color
	};

	// Describe the Vertex Buffer
	D3D11_BUFFER_DESC bufferDesc;
	memset(&bufferDesc, 0, sizeof(bufferDesc));
	// what type of buffer will this be?
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	// what type of usage (press F1, read the docs)
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	// how big in bytes each element in the buffer is.
	bufferDesc.ByteWidth = sizeof(triangleVertices);

	// this struct is created just to set a pointer to the
	// data containing the vertices.
	D3D11_SUBRESOURCE_DATA data;
	data.pSysMem = triangleVertices;

	// create a Vertex Buffer
	gDevice->CreateBuffer(&bufferDesc, &data, &gVertexBuffer);
}

//float Mworld[4][4] = {
//	0.5f, 0.0f, 0.0f, 0.0f,
//	0.0f, gFloat, 0.0f, 0.0f,
//	0.0f, 0.0f, 0.5f, 0.0f,
//	0.0f, 0.0f, 0.0f, 1.0f
//};

XMMATRIX WorldViewProj;

void createConstantBuffer()
{
	// Fill in a buffer description.
	D3D11_BUFFER_DESC cbDesc;
	cbDesc.ByteWidth = sizeof(WorldViewProj);
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	// Fill in the subresource data.
	D3D11_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = &WorldViewProj;
	InitData.SysMemPitch = 0;
	InitData.SysMemSlicePitch = 0;

	// create a Constant Buffer
	gDevice->CreateBuffer(&cbDesc, &InitData, &gConstantBuffer);
}

void transform()
{
	XMMATRIX World = DirectX::XMMatrixScaling(gFloat, 1.0, 1.0);
	XMMATRIX View = DirectX::XMMatrixTranslation(0.0, 0.0, dist);
	//XMVECTOR CamPos = XMVectorSet(0.0, 0.0, 0.0, 0.0);
	//XMVECTOR LookAt = XMVectorSet(0.0, 0.0, 0.0, 0.0);
	//XMVECTOR Up = XMVectorSet(0.0, 1.0, 0.0, 0.0);
	//XMMATRIX View = XMMatrixLookAtLH(CamPos, LookAt, Up);
	//View = XMMatrixTranspose(View);
	//XMMATRIX Projection = XMMatrixPerspectiveFovLH(0.45f * DirectX::XM_PI, 1.0f, 0.1, 10.0f);
	//Projection = XMMatrixTranspose(Projection);

	//XMMATRIX WorldView = XMMatrixMultiply(View, World);
	//WorldViewProj = XMMatrixMultiply(Projection, WorldView);
	WorldViewProj = XMMatrixMultiply(View, World);
	//WorldViewProj =
	//{
	//	0.5f, 0.0f, 0.0f, 0.0f,
	//	0.0f, gFloat, 0.0f, 0.0f,
	//	0.0f, 0.0f, 0.5f, 0.0f,
	//	0.0f, 0.0f, 0.0f, 1.0f
	//};
}

void SetViewport()
{
	D3D11_VIEWPORT vp;
	vp.Width = (float)WIDTH;
	vp.Height = (float)HEIGHT;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	gDeviceContext->RSSetViewports(1, &vp);
}

void Render()
{
	// clear the back buffer to a deep blue
	//float clearColor[] = { 0, 0, 0, 1 };
	gClearColour[3] = 1.0;

	// use DeviceContext to talk to the API
	gDeviceContext->ClearRenderTargetView(gBackbufferRTV, gClearColour);

	// specifying NULL or nullptr we are disabling that stage
	// in the pipeline
	gDeviceContext->VSSetShader(gVertexShader, nullptr, 0);
	gDeviceContext->HSSetShader(nullptr, nullptr, 0);
	gDeviceContext->DSSetShader(nullptr, nullptr, 0);
	gDeviceContext->GSSetShader(nullptr, nullptr, 0);
	gDeviceContext->PSSetShader(gPixelShader, nullptr, 0);

	UINT32 vertexSize = sizeof(float) * 6;
	UINT32 offset = 0;
	// specify which vertex buffer to use next.
	gDeviceContext->IASetVertexBuffers(0, 1, &gVertexBuffer, &vertexSize, &offset);

	// specify the topology to use when drawing
	gDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// specify the IA Layout (how is data passed)
	gDeviceContext->IASetInputLayout(gVertexLayout);

	//ConstantBuffer
	gDeviceContext->VSSetConstantBuffers(0, 1, &gConstantBuffer);

	// issue a draw call of 3 vertices (similar to OpenGL)
	gDeviceContext->Draw(6, 0);
}

int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
	MSG msg = { 0 };
	HWND wndHandle = InitWindow(hInstance); //1. Skapa fönster
	
	if (wndHandle)
	{
		CreateDirect3DContext(wndHandle); //2. Skapa och koppla SwapChain, Device och Device Context

		SetViewport(); //3. Sätt viewport

		CreateShaders(); //4. Skapa vertex- och pixel-shaders

		CreateTriangleData(); //5. Definiera triangelvertiser, 6. Skapa vertex buffer, 7. Skapa input layout
		transform();
		createConstantBuffer();

		ShowWindow(wndHandle, nCmdShow);

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); // (void)io;
		ImGui_ImplWin32_Init(wndHandle);
		ImGui_ImplDX11_Init(gDevice, gDeviceContext);
		ImGui::StyleColorsDark();

		//float dist = 0.1f;

		while (WM_QUIT != msg.message)
		{
			if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				Render(); //8. Rendera

				ImGui_ImplDX11_NewFrame();
				ImGui_ImplWin32_NewFrame();
				ImGui::NewFrame();

				ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.
				ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
				ImGui::SliderFloat("float", &gFloat, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f    
				ImGui::SliderFloat("dist", &dist, -10.0f, 10.0f);
				ImGui::ColorEdit3("clear color", (float*)&gClearColour); // Edit 3 floats representing a color
				ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
				ImGui::End();

				if (dist == 0.0f)
					dist += 0.0001f;

				//XMMATRIX WorldViewProj =
				//{
				//	0.5f, 0.0f, 0.0f, 0.0f,
				//	0.0f, gFloat, 0.0f, 0.0f,
				//	0.0f, 0.0f, 0.5f, 0.0f,
				//	0.0f, 0.0f, 0.0f, 1.0f
				//};
				transform();
				D3D11_MAPPED_SUBRESOURCE mappedMemory;
				gDeviceContext->Map(gConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedMemory);
				memcpy(mappedMemory.pData, &WorldViewProj, sizeof(WorldViewProj));
				gDeviceContext->Unmap(gConstantBuffer, 0);

				ImGui::Render();
				ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

				gSwapChain->Present(0, 0); //9. Växla front- och back-buffer
			}
		}

		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();

		gVertexBuffer->Release();
		gConstantBuffer->Release();

		gVertexLayout->Release();
		gVertexShader->Release();
		gPixelShader->Release();

		gBackbufferRTV->Release();
		gSwapChain->Release();
		gDevice->Release();
		gDeviceContext->Release();
		DestroyWindow(wndHandle);
	}

	return (int) msg.wParam;
}

HWND InitWindow(HINSTANCE hInstance)
{
	WNDCLASSEX wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEX); 
	wcex.style          = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc    = WndProc;
	wcex.hInstance      = hInstance;
	wcex.lpszClassName = L"BTH_D3D_DEMO";
	if (!RegisterClassEx(&wcex))
		return false;

	RECT rc = { 0, 0, (int)WIDTH, (int)HEIGHT };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);

	HWND handle = CreateWindow(
		L"BTH_D3D_DEMO",
		L"BTH Direct3D Demo",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rc.right - rc.left,
		rc.bottom - rc.top,
		nullptr,
		nullptr,
		hInstance,
		nullptr);

	return handle;
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wparam, LPARAM lParam);
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	// check if IMGUI can handle the message (when we click INSIDE ImGui
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;

	switch (message) 
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;		
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

HRESULT CreateDirect3DContext(HWND wndHandle)
{
	// create a struct to hold information about the swap chain
	DXGI_SWAP_CHAIN_DESC scd;

	// clear out the struct for use
	ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

	// fill the swap chain description struct
	scd.BufferCount = 1;                                    // one back buffer
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;     // use 32-bit color
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;      // how swap chain is to be used
	scd.OutputWindow = wndHandle;                           // the window to be used
	scd.SampleDesc.Count = 1;                               // how many multisamples
	scd.Windowed = TRUE;                                    // windowed/full-screen mode

	// create a device, device context and swap chain using the information in the scd struct
	HRESULT hr = D3D11CreateDeviceAndSwapChain(NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		NULL,
		NULL,
		NULL,
		D3D11_SDK_VERSION,
		&scd,
		&gSwapChain,
		&gDevice,
		NULL,
		&gDeviceContext);

	if (SUCCEEDED(hr))
	{
		// get the address of the back buffer
		ID3D11Texture2D* pBackBuffer = nullptr;
		gSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);

		// use the back buffer address to create the render target
		gDevice->CreateRenderTargetView(pBackBuffer, NULL, &gBackbufferRTV);
		pBackBuffer->Release();

		// set the render target as the back buffer
		gDeviceContext->OMSetRenderTargets(1, &gBackbufferRTV, NULL);
	}
	return hr;
}