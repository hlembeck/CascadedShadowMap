#include "PipelineObjects.h"

void PipelineObjects::LoadSimplePSO(const D3D12_INPUT_LAYOUT_DESC& layoutDesc) {
    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;
    UINT compileFlags = 0;

    ThrowIfFailed(D3DCompileFromFile(L"LocalIllumination.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS", "vs_5_1", compileFlags, 0, &vertexShader, NULL));
    ThrowIfFailed(D3DCompileFromFile(L"LocalIllumination.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_1", compileFlags, 0, &pixelShader, NULL));

    D3D12_DEPTH_STENCIL_DESC depthDesc = {
        TRUE,
        D3D12_DEPTH_WRITE_MASK_ALL,
        D3D12_COMPARISON_FUNC_LESS,
        FALSE
    };

    D3D12_RASTERIZER_DESC rasterizerDesc = {
        D3D12_FILL_MODE_SOLID,
        D3D12_CULL_MODE_BACK,
        FALSE,
        1,
        1.0f, //clamp
        0.0f, //slope scaled
        TRUE,
        FALSE,
        FALSE,
        0,
        D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {
        m_gRootSignature.Get(),
        CD3DX12_SHADER_BYTECODE(vertexShader.Get()),
        CD3DX12_SHADER_BYTECODE(pixelShader.Get()),
        {},
        {},
        {},
        {},
        CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        UINT_MAX,
        CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
        depthDesc,
        layoutDesc,
        D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        1
    };
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc.Count = 1;
    ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_simplePSO)));
}

void PipelineObjects::LoadShadowPSO(const D3D12_INPUT_LAYOUT_DESC& layoutDesc) {
    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> geometryShader;
    UINT compileFlags = 0;

    ThrowIfFailed(D3DCompileFromFile(L"BasicShadow.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS", "vs_5_1", compileFlags, 0, &vertexShader, NULL));
    ThrowIfFailed(D3DCompileFromFile(L"BasicShadow.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "GS", "gs_5_1", compileFlags, 0, &geometryShader, NULL));

    D3D12_DEPTH_STENCIL_DESC depthDesc = {
        TRUE,
        D3D12_DEPTH_WRITE_MASK_ALL,
        D3D12_COMPARISON_FUNC_LESS_EQUAL,
        FALSE
    };

    D3D12_RASTERIZER_DESC rasterizerDesc = {
        D3D12_FILL_MODE_SOLID,
        D3D12_CULL_MODE_NONE,
        FALSE,
        10000000,
        0.0f, //clamp
        0.0f, //slope scaled
        TRUE,
        FALSE,
        FALSE,
        0,
        D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {
        m_gRootSignature.Get(),
        CD3DX12_SHADER_BYTECODE(vertexShader.Get()),
        {},
        {},
        {},
        CD3DX12_SHADER_BYTECODE(geometryShader.Get()),
        {},
        CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        UINT_MAX,
        rasterizerDesc,
        depthDesc,
        layoutDesc,
        D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        0,
        {},
        DXGI_FORMAT_D32_FLOAT,
        {1,0},
        0,
        {NULL,0},
        D3D12_PIPELINE_STATE_FLAG_NONE
    };
    ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_shadowPSO)));
}

void PipelineObjects::LoadShadowTerrainPSO() {

    D3D12_INPUT_ELEMENT_DESC elemDescs[2] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
    D3D12_INPUT_LAYOUT_DESC layoutDesc = {
        elemDescs,
        1
    };

    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> geometryShader;
    UINT compileFlags = 0;

    ThrowIfFailed(D3DCompileFromFile(L"BasicShadowTerrain.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS", "vs_5_1", compileFlags, 0, &vertexShader, NULL));
    ThrowIfFailed(D3DCompileFromFile(L"BasicShadowTerrain.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "GS", "gs_5_1", compileFlags, 0, &geometryShader, NULL));

    D3D12_DEPTH_STENCIL_DESC depthDesc = {
        TRUE,
        D3D12_DEPTH_WRITE_MASK_ALL,
        D3D12_COMPARISON_FUNC_LESS_EQUAL,
        FALSE
    };

    D3D12_RASTERIZER_DESC rasterizerDesc = {
        D3D12_FILL_MODE_SOLID,
        D3D12_CULL_MODE_NONE,
        FALSE,
        0,
        0.0f, //clamp
        0.0f, //slope scaled
        TRUE,
        FALSE,
        FALSE,
        0,
        D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {
        m_gRootSignature.Get(),
        CD3DX12_SHADER_BYTECODE(vertexShader.Get()),
        {},
        {},
        {},
        CD3DX12_SHADER_BYTECODE(geometryShader.Get()),
        {},
        CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        UINT_MAX,
        rasterizerDesc,
        depthDesc,
        layoutDesc,
        D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        0,
        {},
        DXGI_FORMAT_D32_FLOAT,
        {1,0},
        0,
        {NULL,0},
        D3D12_PIPELINE_STATE_FLAG_NONE
    };
    ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_shadowTerrainPSO)));
}

void PipelineObjects::LoadBlockTerrainGenPSO() {
    D3D12_INPUT_ELEMENT_DESC elemDescs[2] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
    D3D12_INPUT_LAYOUT_DESC layoutDesc = {
        elemDescs,
        1
    };

    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;
    UINT compileFlags = 0;

    ThrowIfFailed(D3DCompileFromFile(L"BlockTerrainGen.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS", "vs_5_1", compileFlags, 0, &vertexShader, NULL));
    ThrowIfFailed(D3DCompileFromFile(L"BlockTerrainGen.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_1", compileFlags, 0, &pixelShader, NULL));

    D3D12_DEPTH_STENCIL_DESC depthDesc = {
        TRUE,
        D3D12_DEPTH_WRITE_MASK_ALL,
        D3D12_COMPARISON_FUNC_LESS,
        FALSE
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {
        m_gRootSignature.Get(),
        CD3DX12_SHADER_BYTECODE(vertexShader.Get()),
        CD3DX12_SHADER_BYTECODE(pixelShader.Get()),
        {},
        {},
        {},
        {},
        CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        UINT_MAX,
        CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
        depthDesc,
        layoutDesc,
        D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        1,
        {},
        DXGI_FORMAT_UNKNOWN,
        {1,0},
        0,
        {NULL,0},
        D3D12_PIPELINE_STATE_FLAG_NONE
    };
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R32G32B32A32_FLOAT;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_blockTerrainGen)));
}

void PipelineObjects::LoadChunkTerrainRenderPSO() {
    D3D12_INPUT_ELEMENT_DESC elemDescs[2] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
    D3D12_INPUT_LAYOUT_DESC layoutDesc = {
        elemDescs,
        1
    };

    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;
    UINT compileFlags = 0;

    ThrowIfFailed(D3DCompileFromFile(L"ChunkTerrainRender.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS", "vs_5_1", compileFlags, 0, &vertexShader, NULL));
    ThrowIfFailed(D3DCompileFromFile(L"ChunkTerrainRender.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_1", compileFlags, 0, &pixelShader, NULL));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {
        m_gRootSignature.Get(),
        CD3DX12_SHADER_BYTECODE(vertexShader.Get()),
        CD3DX12_SHADER_BYTECODE(pixelShader.Get()),
        {},
        {},
        {},
        {},
        CD3DX12_BLEND_DESC(D3D12_DEFAULT),
        UINT_MAX,
        CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
        CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
        layoutDesc,
        D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        1,
        {},
        DXGI_FORMAT_UNKNOWN,
        {1,0},
        0,
        {NULL,0},
        D3D12_PIPELINE_STATE_FLAG_NONE
    };
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_chunkTerrainRender)));
}

//void PipelineObjects::LoadClipmapUpsample() {
//    D3D12_INPUT_ELEMENT_DESC elemDescs[2] = { 
//        {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
//        {"TPOS", 0, DXGI_FORMAT_R32G32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
//    };
//    D3D12_INPUT_LAYOUT_DESC layoutDesc = {
//        elemDescs,
//        2
//    };
//
//    ComPtr<ID3DBlob> vertexShader;
//    ComPtr<ID3DBlob> pixelShader;
//    UINT compileFlags = 0;
//
//    ThrowIfFailed(D3DCompileFromFile(L"ClipmapUpsample.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS", "vs_5_1", compileFlags, 0, &vertexShader, NULL));
//    ThrowIfFailed(D3DCompileFromFile(L"ClipmapUpsample.hlsl", NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_1", compileFlags, 0, &pixelShader, NULL));
//
//    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {
//        m_gRootSignature.Get(),
//        CD3DX12_SHADER_BYTECODE(vertexShader.Get()),
//        CD3DX12_SHADER_BYTECODE(pixelShader.Get()),
//        {},
//        {},
//        {},
//        {},
//        CD3DX12_BLEND_DESC(D3D12_DEFAULT),
//        UINT_MAX,
//        CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT),
//        CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT),
//        layoutDesc,
//        D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
//        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
//        1,
//        {},
//        DXGI_FORMAT_UNKNOWN,
//        {1,0},
//        0,
//        {NULL,0},
//        D3D12_PIPELINE_STATE_FLAG_NONE
//    };
//    psoDesc.RTVFormats[0] = DXGI_FORMAT_R32_FLOAT;
//    ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_clipmapUpsample)));
//}

void PipelineObjects::OnInit() {
    RootSignatures::OnInit();

    D3D12_INPUT_LAYOUT_DESC layoutDesc;
    D3D12_INPUT_ELEMENT_DESC inputElemDesc[MAXVERTEXINPUTS] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };
    layoutDesc.pInputElementDescs = inputElemDesc;

    layoutDesc.NumElements = 2;
    LoadShadowPSO(layoutDesc);
    LoadShadowTerrainPSO();

    layoutDesc.NumElements = 3;
    inputElemDesc[2] = {
        "TEXCOORDS",
        0,
        DXGI_FORMAT_R32G32_FLOAT,
        1,
        D3D12_APPEND_ALIGNED_ELEMENT,
        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
        0
    };
    LoadSimplePSO(layoutDesc);
    LoadBlockTerrainGenPSO();
    LoadChunkTerrainRenderPSO();
    //LoadClipmapUpsample();

    //Debug
    m_simplePSO->SetName(L"Simple PSO");
    m_shadowPSO->SetName(L"Shadow PSO");
    m_shadowTerrainPSO->SetName(L"Shadow Terrain PSO");
    m_chunkTerrainRender->SetName(L"Chunk Terrain Render PSO");
    m_blockTerrainGen->SetName(L"Block Terrain Gen PSO");
    //m_clipmapUpsample->SetName(L"Clipmap Upsample PSO");
}