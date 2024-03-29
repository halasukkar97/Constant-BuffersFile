#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <dxerr.h>
#include <stdio.h>
#define _XM_NO_INTRINSICS_
#define XM_NO_ALIGNMENT
#include <xnamath.h>
int (WINAPIV * __vsnprintf_s)(char *, size_t, const char*, va_list) = _vsnprintf;

D3D_DRIVER_TYPE         g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL       g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*           g_pD3DDevice = NULL;
ID3D11DeviceContext*    g_pImmediateContext = NULL;
IDXGISwapChain*         g_pSwapChain = NULL;
ID3D11RenderTargetView* g_pBackBufferRTView = NULL;

ID3D11Buffer* g_pVertexBuffer;
ID3D11VertexShader* g_pVertexShader;
ID3D11PixelShader* g_pPixelShader;
ID3D11InputLayout* g_pInputLayout;

ID3D11Buffer*  g_pConstantBuffer0;

//define vertex structure
struct POS_COL_VERTEX
{
	XMFLOAT3 Pos;
	XMFLOAT4 Col;
};

//const buffer structs. pack to 16 bytes. dont let any single element cross a 16 byte boundary 
struct CONSTANT_BUFFER0
{
	float RedAmount; // 4 bytes
	XMFLOAT3 packing_bytes;  //3x4 bytes = 12 bytes
};

HINSTANCE	g_hInst = NULL;
HWND		g_hWnd = NULL;

// Rename for each tutorial
char		g_TutorialName[100] = "Tutorial 01 Exercise 02\0";

//	Forward declarations
HRESULT InitialiseGraphics(void);

HRESULT InitialiseWindow(HINSTANCE hInstance, int nCmdShow);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);


//////////////////////////////////////////////////////////////////////////////////////
// Create D3D device and swap chain
//////////////////////////////////////////////////////////////////////////////////////
HRESULT InitialiseD3D()
{
	HRESULT hr = S_OK;

	RECT rc;
	GetClientRect(g_hWnd, &rc);
	UINT width = rc.right - rc.left;
	UINT height = rc.bottom - rc.top;

	UINT createDeviceFlags = 0;

#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_DRIVER_TYPE driverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE, // comment out this line if you need to test D3D 11.0 functionality on hardware that doesn't support it
		D3D_DRIVER_TYPE_WARP, // comment this out also to use reference device
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT numDriverTypes = ARRAYSIZE(driverTypes);

	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
	};
	UINT numFeatureLevels = ARRAYSIZE(featureLevels);

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = width;
	sd.BufferDesc.Height = height;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = g_hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = true;

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++)
	{
		g_driverType = driverTypes[driverTypeIndex];
		hr = D3D11CreateDeviceAndSwapChain(NULL, g_driverType, NULL,
			createDeviceFlags, featureLevels, numFeatureLevels,
			D3D11_SDK_VERSION, &sd, &g_pSwapChain,
			&g_pD3DDevice, &g_featureLevel, &g_pImmediateContext);
		if (SUCCEEDED(hr))
			break;
	}

	if (FAILED(hr))
		return hr;


	

	// Get pointer to back buffer texture
	ID3D11Texture2D *pBackBufferTexture;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
		(LPVOID*)&pBackBufferTexture);

	if (FAILED(hr)) return hr;

	// Use the back buffer texture pointer to create the render target view
	hr = g_pD3DDevice->CreateRenderTargetView(pBackBufferTexture, NULL,
		&g_pBackBufferRTView);
	pBackBufferTexture->Release();

	if (FAILED(hr)) return hr;

	// Set the render target view
	g_pImmediateContext->OMSetRenderTargets(1, &g_pBackBufferRTView, NULL);

	// Set the viewport
	D3D11_VIEWPORT viewport;

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = width;
	viewport.Height = height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	g_pImmediateContext->RSSetViewports(1, &viewport);


	return S_OK;
}

void RenderFrame(void);

//////////////////////////////////////////////////////////////////////////////////////
// Clean up D3D objects
//////////////////////////////////////////////////////////////////////////////////////
void ShutdownD3D()
{

	if (g_pVertexBuffer) g_pVertexBuffer->Release(); //03-01
	if(g_pInputLayout)g_pInputLayout->Release(); //03-01
	if(g_pVertexShader)g_pVertexShader->Release(); //03-01
	if(g_pPixelShader)g_pPixelShader->Release(); //03-01


	if (g_pBackBufferRTView) g_pBackBufferRTView->Release();

	if (g_pSwapChain) g_pSwapChain->Release();
	if(g_pConstantBuffer0) g_pConstantBuffer0->Release();
	if (g_pImmediateContext) g_pImmediateContext->Release();
	if (g_pD3DDevice) g_pD3DDevice->Release();
}


HRESULT InitialiseD3D();
void ShutdownD3D();




//////////////////////////////////////////////////////////////////////////////////////
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//////////////////////////////////////////////////////////////////////////////////////
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	if (FAILED(InitialiseWindow(hInstance, nCmdShow)))
	{
		DXTRACE_MSG("Failed to create Window");
		return 0;
	}

	if (FAILED(InitialiseD3D()))
	{
		DXTRACE_MSG("Failed to create Device");
		ShutdownD3D();
		return 0;
	}
	//call initialiserGraphics
	if (FAILED(InitialiseGraphics())) //03-01
	{
		DXTRACE_MSG("Failed to initialise graphics");
		return 0;
	}
	// Main message loop
	MSG msg = { 0 };

	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			// do something
			RenderFrame();
		}
	}

	ShutdownD3D();
	return (int)msg.wParam;
}


//////////////////////////////////////////////////////////////////////////////////////
// Register class and create window
//////////////////////////////////////////////////////////////////////////////////////
HRESULT InitialiseWindow(HINSTANCE hInstance, int nCmdShow)
{
	// Give your app window your own name
	char Name[100] = "Tutorial 03 Exercise 02\0";

	// Register class
	WNDCLASSEX wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = hInstance;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	//   wcex.hbrBackground = (HBRUSH )( COLOR_WINDOW + 1); // Needed for non-D3D apps
	wcex.lpszClassName = Name;

	if (!RegisterClassEx(&wcex)) return E_FAIL;

	// Create window
	g_hInst = hInstance;
	RECT rc = { 0, 0, 640, 480 };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	g_hWnd = CreateWindow(Name, g_TutorialName, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left,
		rc.bottom - rc.top, NULL, NULL, hInstance, NULL);
	if (!g_hWnd)
		return E_FAIL;

	ShowWindow(g_hWnd, nCmdShow);

	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////
// Called every time the application receives a message
//////////////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
			DestroyWindow(g_hWnd);
		return 0;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//init graphics
////////////////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT InitialiseGraphics()//03-01
{
	HRESULT hr = S_OK;

	//create constant buffer //04-1
	D3D11_BUFFER_DESC constant_buffer_desc;
	ZeroMemory(&constant_buffer_desc, sizeof(constant_buffer_desc));
	constant_buffer_desc.Usage = D3D11_USAGE_DEFAULT;  //can use updateSubresource() to update
	constant_buffer_desc.ByteWidth = 16;  //must be a multiple of 16, calculate from CB struct
	constant_buffer_desc.BindFlags =D3D11_BIND_CONSTANT_BUFFER;  //use as a constant buffer

	hr = g_pD3DDevice->CreateBuffer(&constant_buffer_desc, NULL, &g_pConstantBuffer0);

	if (FAILED(hr))
		return hr;


	//define vertices of a triangle - screen coordinates -1.0 to +1.0
	POS_COL_VERTEX vertices[]=
	{
		{XMFLOAT3(0.9f, 0.9f, 0.0f),XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)},
		{XMFLOAT3(0.9f, -0.9f, 0.0f),XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f) },
		{XMFLOAT3(-0.9f, -0.9f, 0.0f),XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f) }
	};

	//set up and create vertex buffer
	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.Usage = D3D11_USAGE_DYNAMIC;   //used by CPU and GPU
	bufferDesc.ByteWidth = sizeof(POS_COL_VERTEX) * 3;  //total size of buffer, 3 vertices
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER; //use as vertexbuffer
	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; //allow CPU access
	hr =g_pD3DDevice->CreateBuffer(&bufferDesc, NULL, &g_pVertexBuffer);//create the buffer

	if (FAILED(hr))// return error code on dailure
	{
		return hr;
	}

	//copy the vertices into the buffer
	D3D11_MAPPED_SUBRESOURCE ms;

	//lock the buffer to allow writting
	g_pImmediateContext->Map(g_pVertexBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);

	//copy the data
	memcpy(ms.pData, vertices, sizeof(vertices));

	//unlock the buffer
	g_pImmediateContext->Unmap(g_pVertexBuffer, NULL);

	//load and compile pixel and vertex shaders -use vs_5_0 to target DX11 hardware only
	ID3DBlob *VS, *PS, *error;

	hr = D3DX11CompileFromFile("shaders.hlsl", 0, 0, "VShader", "vs_4_0", 0, 0, 0, &VS, &error, 0);
	if (error != 0)//check for shader compilation error
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr)) //dont fail if error is just a warning
		{
			return hr;
		};
	}

	hr=D3DX11CompileFromFile("shaders.hlsl", 0, 0, "PShader", "ps_4_0", 0, 0, 0, &PS, &error, 0);
	if (error != 0)//check for shader compilation error
	{
		OutputDebugStringA((char*)error->GetBufferPointer());
		error->Release();
		if (FAILED(hr)) //dont fail if error is just a warning
		{
			return hr;
		};
	}


		//create shader objects
	hr = g_pD3DDevice->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &g_pVertexShader);

	if (FAILED(hr))
	{
		return hr;
	}


	hr = g_pD3DDevice->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), NULL, &g_pPixelShader);

	if (FAILED(hr))
	{
		return hr;
	}



	//set the shader objects as active
	g_pImmediateContext->VSSetShader(g_pVertexShader, 0, 0);
	g_pImmediateContext->PSSetShader(g_pPixelShader, 0, 0);

	//create and set the input layout object
	D3D11_INPUT_ELEMENT_DESC iedesc[] =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
		{"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA,0},
	};

	hr = g_pD3DDevice->CreateInputLayout(iedesc,2,VS->GetBufferPointer(),VS->GetBufferSize(),&g_pInputLayout);

	if (FAILED(hr))
	{
		return hr;
	}


	g_pImmediateContext->IASetInputLayout(g_pInputLayout);
	

	CONSTANT_BUFFER0 cb0_values;
	cb0_values.RedAmount =2.0f; //50% OF VERTEX red value

	//upload the new values for the constant buffer
	g_pImmediateContext->UpdateSubresource(g_pConstantBuffer0, 0, 0, &cb0_values, 0, 0);

	g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pConstantBuffer0);
	return S_OK;
}

// Render frame
void RenderFrame(void)
{
	// Clear the back buffer - choose a colour you like
	float rgba_clear_colour[4] = { 1.0f, 0.2f, 0.6f, 1.0f };
	g_pImmediateContext->ClearRenderTargetView(g_pBackBufferRTView, rgba_clear_colour);


	// RENDER HERE
	//set vertex buffer //03-01
	UINT stride = sizeof(POS_COL_VERTEX);
	UINT offset = 0;
	g_pImmediateContext->IASetVertexBuffers(0, 1, &g_pVertexBuffer, &stride, &offset);

	//select which primtive type to use //03-01
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//draw the vertex buffer to the back buffer //03-01
	g_pImmediateContext->Draw(3, 0);

	// Display what has just been rendered
	g_pSwapChain->Present(0, 0);
}



