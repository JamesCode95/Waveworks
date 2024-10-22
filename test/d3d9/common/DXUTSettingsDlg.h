// This code contains NVIDIA Confidential Information and is disclosed 
// under the Mutual Non-Disclosure Agreement. 
// 
// Notice 
// ALL NVIDIA DESIGN SPECIFICATIONS AND CODE ("MATERIALS") ARE PROVIDED "AS IS" NVIDIA MAKES 
// NO REPRESENTATIONS, WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO 
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ANY IMPLIED WARRANTIES OF NONINFRINGEMENT, 
// MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. 
// 
// NVIDIA Corporation assumes no responsibility for the consequences of use of such 
// information or for any infringement of patents or other rights of third parties that may 
// result from its use. No license is granted by implication or otherwise under any patent 
// or patent rights of NVIDIA Corporation. No third party distribution is allowed unless 
// expressly authorized by NVIDIA.  Details are subject to change without notice. 
// This code supersedes and replaces all information previously supplied. 
// NVIDIA Corporation products are not authorized for use as critical 
// components in life support devices or systems without express written approval of 
// NVIDIA Corporation. 
// 
// Copyright � 2008- 2013 NVIDIA Corporation. All rights reserved.
//
// NVIDIA Corporation and its licensors retain all intellectual property and proprietary
// rights in and to this software and related documentation and any modifications thereto.
// Any use, reproduction, disclosure or distribution of this software and related
// documentation without an express license agreement from NVIDIA Corporation is
// strictly prohibited.
//
 
 //--------------------------------------------------------------------------------------
// File: DXUTSettingsDlg.cpp
//
// Copyright (c) Microsoft Corporation. All rights reserved
//--------------------------------------------------------------------------------------
#pragma once
#ifndef DXUT_SETTINGS_H
#define DXUT_SETTINGS_H


//--------------------------------------------------------------------------------------
// Control IDs
//--------------------------------------------------------------------------------------
#define DXUTSETTINGSDLG_STATIC                  -1
#define DXUTSETTINGSDLG_OK                      1
#define DXUTSETTINGSDLG_CANCEL                  2
#define DXUTSETTINGSDLG_ADAPTER                 3
#define DXUTSETTINGSDLG_DEVICE_TYPE             4
#define DXUTSETTINGSDLG_WINDOWED                5
#define DXUTSETTINGSDLG_FULLSCREEN              6
#define DXUTSETTINGSDLG_ADAPTER_FORMAT          7
#define DXUTSETTINGSDLG_ADAPTER_FORMAT_LABEL    8
#define DXUTSETTINGSDLG_RESOLUTION              9
#define DXUTSETTINGSDLG_RESOLUTION_LABEL        10
#define DXUTSETTINGSDLG_REFRESH_RATE            11
#define DXUTSETTINGSDLG_REFRESH_RATE_LABEL      12
#define DXUTSETTINGSDLG_BACK_BUFFER_FORMAT      13
#define DXUTSETTINGSDLG_DEPTH_STENCIL           14
#define DXUTSETTINGSDLG_MULTISAMPLE_TYPE        15
#define DXUTSETTINGSDLG_MULTISAMPLE_QUALITY     16
#define DXUTSETTINGSDLG_VERTEX_PROCESSING       17
#define DXUTSETTINGSDLG_PRESENT_INTERVAL        18
#define DXUTSETTINGSDLG_DEVICECLIP              19
#define DXUTSETTINGSDLG_RESOLUTION_SHOW_ALL     20
#define DXUTSETTINGSDLG_WINDOWED_GROUP          0x0100


//--------------------------------------------------------------------------------------
// Dialog for selection of device settings 
// Use DXUTGetSettingsDialog() to access global instance
// To control the contents of the dialog, use the CD3DEnumeration class.
//--------------------------------------------------------------------------------------
class CD3DSettingsDlg
{
public:
    CD3DSettingsDlg();
    ~CD3DSettingsDlg();

    void Init( CDXUTDialogResourceManager* pManager );
    void Init( CDXUTDialogResourceManager* pManager, LPCWSTR szControlTextureFileName );
    void Init( CDXUTDialogResourceManager* pManager, LPCWSTR pszControlTextureResourcename, HMODULE hModule);

    HRESULT OnCreateDevice( IDirect3DDevice9* pd3dDevice );
    HRESULT Refresh();
    HRESULT OnResetDevice();
    HRESULT OnLostDevice();
    HRESULT OnRender( float fElapsedTime );
    HRESULT OnDestroyDevice();

    CDXUTDialog* GetDialogControl() { return &m_Dialog; }
    bool IsActive() { return m_bActive; }
    void SetActive( bool bActive ) { m_bActive = bActive; if( bActive ) Refresh(); }

    LRESULT MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

protected:
    friend CD3DSettingsDlg* DXUTGetSettingsDialog();

    void CreateControls();
    HRESULT SetDeviceSettingsFromUI();

    void OnEvent( UINT nEvent, int nControlID, CDXUTControl* pControl );
    static void WINAPI StaticOnEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserData );

    CD3DEnumAdapterInfo* GetCurrentAdapterInfo();
    CD3DEnumDeviceInfo* GetCurrentDeviceInfo();
    CD3DEnumDeviceSettingsCombo* GetCurrentDeviceSettingsCombo();

    void AddAdapter( const WCHAR* strDescription, UINT iAdapter );
    UINT GetSelectedAdapter();

    void AddDeviceType( D3DDEVTYPE devType );
    D3DDEVTYPE GetSelectedDeviceType();

    void SetWindowed( bool bWindowed );
    bool IsWindowed();

    void AddAdapterFormat( D3DFORMAT format );
    D3DFORMAT GetSelectedAdapterFormat();

    void AddResolution( DWORD dwWidth, DWORD dwHeight );
    void GetSelectedResolution( DWORD* pdwWidth, DWORD* pdwHeight );

    void AddRefreshRate( DWORD dwRate );
    DWORD GetSelectedRefreshRate();

    void AddBackBufferFormat( D3DFORMAT format );
    D3DFORMAT GetSelectedBackBufferFormat();

    void AddDepthStencilBufferFormat( D3DFORMAT format );
    D3DFORMAT GetSelectedDepthStencilBufferFormat();

    void AddMultisampleType( D3DMULTISAMPLE_TYPE type );
    D3DMULTISAMPLE_TYPE GetSelectedMultisampleType();

    void AddMultisampleQuality( DWORD dwQuality );
    DWORD GetSelectedMultisampleQuality();

    void AddVertexProcessingType( DWORD dwType );
    DWORD GetSelectedVertexProcessingType();

    DWORD GetSelectedPresentInterval();

    void SetDeviceClip( bool bDeviceClip );
    bool IsDeviceClip();

    HRESULT OnAdapterChanged();
    HRESULT OnDeviceTypeChanged();
    HRESULT OnWindowedFullScreenChanged();
    HRESULT OnAdapterFormatChanged();
    HRESULT OnResolutionChanged();
    HRESULT OnRefreshRateChanged();
    HRESULT OnBackBufferFormatChanged();
    HRESULT OnDepthStencilBufferFormatChanged();
    HRESULT OnMultisampleTypeChanged();
    HRESULT OnMultisampleQualityChanged();
    HRESULT OnVertexProcessingChanged();
    HRESULT OnPresentIntervalChanged();
    HRESULT OnDeviceClipChanged();

    IDirect3DStateBlock9* m_pStateBlock;
    CDXUTDialog m_Dialog;
    bool m_bActive; 
};


CD3DSettingsDlg* DXUTGetSettingsDialog();



#endif

