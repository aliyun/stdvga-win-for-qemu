/*
 * Copyright (c) 2026 Alibaba Cloud Computing Ltd.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met :
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and / or other materials provided with the distribution.
 * 3. Neither the names of the copyright holders nor the names of their contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "driver.h"

// Some legacy WDDM wrapper functions remain in this file as dead code after
// the conversion to KMDOD. Suppress "unreferenced function with internal
// linkage has been removed" warnings.
#pragma warning(disable: 4505)

static VOID
DdiTrace(
    _In_ const char* msg
    )
{
#if DBG
    UNICODE_STRING fileName;
    RtlInitUnicodeString(&fileName, L"\\??\\C:\\stdvga_ddi.log");

    OBJECT_ATTRIBUTES oa;
    InitializeObjectAttributes(&oa, &fileName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    IO_STATUS_BLOCK iosb;
    HANDLE hFile = NULL;
    NTSTATUS st = ZwCreateFile(
        &hFile, FILE_APPEND_DATA | SYNCHRONIZE,
        &oa, &iosb, NULL,
        FILE_ATTRIBUTE_NORMAL,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        FILE_OPEN_IF,
        FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE,
        NULL, 0);

    if (NT_SUCCESS(st))
    {
        SIZE_T len = 0;
        const char* p = msg;
        while (*p++) len++;
        ZwWriteFile(hFile, NULL, NULL, NULL, &iosb, (PVOID)msg, (ULONG)len, NULL, NULL);
        char nl = '\n';
        ZwWriteFile(hFile, NULL, NULL, NULL, &iosb, &nl, 1, NULL, NULL);
        ZwClose(hFile);
    }
#else
    UNREFERENCED_PARAMETER(msg);
#endif
}

#pragma code_seg("PAGE")

static NTSTATUS
StdVgaDdiAddDevice(
    _In_ DEVICE_OBJECT* pPhysicalDeviceObject,
    _Outptr_ PVOID*     ppDeviceContext
    )
{
    PAGED_CODE();

    if (ppDeviceContext == NULL || pPhysicalDeviceObject == NULL)
        return STATUS_INVALID_PARAMETER;

    PSTDVGA_DEVICE_CONTEXT pCtx = (PSTDVGA_DEVICE_CONTEXT)ExAllocatePoolWithTag(
        NonPagedPoolNx,
        sizeof(STDVGA_DEVICE_CONTEXT),
        STDVGA_TAG
        );

    if (pCtx == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;
    RtlZeroMemory(pCtx, sizeof(STDVGA_DEVICE_CONTEXT));
    pCtx->pPhysicalDevice = pPhysicalDeviceObject;
    pCtx->AdapterPowerState = PowerDeviceD0;
    pCtx->MonitorPowerState = PowerDeviceD0;

    *ppDeviceContext = pCtx;
    return STATUS_SUCCESS;
}

static NTSTATUS
StdVgaDdiRemoveDevice(
    _In_ PVOID pDeviceContext
    )
{
    PAGED_CODE();

    if (pDeviceContext != NULL)
    {
        ExFreePoolWithTag(pDeviceContext, STDVGA_TAG);
    }
    return STATUS_SUCCESS;
}

static NTSTATUS
StdVgaDdiStartDevice(
    _In_  PVOID              pDeviceContext,
    _In_  DXGK_START_INFO*   pDxgkStartInfo,
    _In_  DXGKRNL_INTERFACE* pDxgkInterface,
    _Out_ PULONG             pNumberOfVideoPresentSources,
    _Out_ PULONG             pNumberOfChildren
    )
{
    PAGED_CODE();
    DdiTrace("DDI:StartDevice");
    return StdVgaStartDevice(
        (PSTDVGA_DEVICE_CONTEXT)pDeviceContext,
        pDxgkStartInfo,
        pDxgkInterface,
        pNumberOfVideoPresentSources,
        pNumberOfChildren
        );
}

static NTSTATUS
StdVgaDdiStopDevice(
    _In_ PVOID pDeviceContext
    )
{
    PAGED_CODE();
    DdiTrace("DDI:StopDevice");
    return StdVgaStopDevice((PSTDVGA_DEVICE_CONTEXT)pDeviceContext);
}

#pragma code_seg()

static VOID
StdVgaDdiResetDevice(
    _In_ PVOID pDeviceContext
    )
{
    StdVgaResetDevice((PSTDVGA_DEVICE_CONTEXT)pDeviceContext);
}


#pragma code_seg("PAGE")

static NTSTATUS
StdVgaDdiDispatchIoRequest(
    _In_ PVOID               pDeviceContext,
    _In_ ULONG               VidPnSourceId,
    _In_ PVIDEO_REQUEST_PACKET pVideoRequestPacket
    )
{
    PAGED_CODE();
    DdiTrace("DDI:DispatchIoRequest");
    return StdVgaDispatchIoRequest(
        (PSTDVGA_DEVICE_CONTEXT)pDeviceContext,
        VidPnSourceId,
        pVideoRequestPacket
        );
}

static NTSTATUS
StdVgaDdiSetPowerState(
    _In_ PVOID              pDeviceContext,
    _In_ ULONG              DeviceUid,
    _In_ DEVICE_POWER_STATE DevicePowerState,
    _In_ POWER_ACTION       ActionType
    )
{
    PAGED_CODE();
    DdiTrace("DDI:SetPowerState");
    return StdVgaSetPowerState(
        (PSTDVGA_DEVICE_CONTEXT)pDeviceContext,
        DeviceUid,
        DevicePowerState,
        ActionType
        );
}

static NTSTATUS
StdVgaDdiQueryChildRelations(
    _In_    PVOID                   pDeviceContext,
    _Inout_ PDXGK_CHILD_DESCRIPTOR pChildRelations,
    _In_    ULONG                   ChildRelationsSize
    )
{
    PAGED_CODE();
    DdiTrace("DDI:QueryChildRelations");
    return StdVgaQueryChildRelations(
        (PSTDVGA_DEVICE_CONTEXT)pDeviceContext,
        pChildRelations,
        ChildRelationsSize
        );
}

static NTSTATUS
StdVgaDdiQueryChildStatus(
    _In_    PVOID               pDeviceContext,
    _Inout_ PDXGK_CHILD_STATUS  pChildStatus,
    _In_    BOOLEAN             NonDestructiveOnly
    )
{
    PAGED_CODE();
    DdiTrace("DDI:QueryChildStatus");
    return StdVgaQueryChildStatus(
        (PSTDVGA_DEVICE_CONTEXT)pDeviceContext,
        pChildStatus,
        NonDestructiveOnly
        );
}

static NTSTATUS
StdVgaDdiQueryDeviceDescriptor(
    _In_    PVOID                   pDeviceContext,
    _In_    ULONG                   ChildUid,
    _Inout_ PDXGK_DEVICE_DESCRIPTOR pDeviceDescriptor
    )
{
    PAGED_CODE();
    DdiTrace("DDI:QueryDeviceDescriptor");
    return StdVgaQueryDeviceDescriptor(
        (PSTDVGA_DEVICE_CONTEXT)pDeviceContext,
        ChildUid,
        pDeviceDescriptor
        );
}

static NTSTATUS
StdVgaDdiQueryAdapterInfo(
    _In_ PVOID                          pDeviceContext,
    _In_ CONST DXGKARG_QUERYADAPTERINFO* pQueryAdapterInfo
    )
{
    PAGED_CODE();
    DdiTrace("DDI:QueryAdapterInfo");
    return StdVgaQueryAdapterInfo(
        (PSTDVGA_DEVICE_CONTEXT)pDeviceContext,
        pQueryAdapterInfo
        );
}

static NTSTATUS
StdVgaDdiSetPointerPosition(
    _In_ PVOID                                  pDeviceContext,
    _In_ CONST DXGKARG_SETPOINTERPOSITION*     pSetPointerPosition
    )
{
    PAGED_CODE();
    DdiTrace("DDI:SetPointerPosition");
    return StdVgaSetPointerPosition(
        (PSTDVGA_DEVICE_CONTEXT)pDeviceContext,
        pSetPointerPosition
        );
}

static NTSTATUS
StdVgaDdiSetPointerShape(
    _In_ PVOID                              pDeviceContext,
    _In_ CONST DXGKARG_SETPOINTERSHAPE*    pSetPointerShape
    )
{
    PAGED_CODE();
    DdiTrace("DDI:SetPointerShape");
    return StdVgaSetPointerShape(
        (PSTDVGA_DEVICE_CONTEXT)pDeviceContext,
        pSetPointerShape
        );
}

static NTSTATUS
StdVgaDdiEscape(
    _In_ PVOID                      pDeviceContext,
    _In_ CONST DXGKARG_ESCAPE*     pEscape
    )
{
    PAGED_CODE();
    DdiTrace("DDI:Escape");
    return StdVgaEscape(
        (PSTDVGA_DEVICE_CONTEXT)pDeviceContext,
        pEscape
        );
}

static NTSTATUS
StdVgaDdiIsSupportedVidPn(
    _In_    PVOID                       pDeviceContext,
    _Inout_ DXGKARG_ISSUPPORTEDVIDPN*  pIsSupportedVidPn
    )
{
    PAGED_CODE();
    DdiTrace("DDI:IsSupportedVidPn");
    return StdVgaIsSupportedVidPn(
        (PSTDVGA_DEVICE_CONTEXT)pDeviceContext,
        pIsSupportedVidPn
        );
}

static NTSTATUS
StdVgaDdiRecommendFunctionalVidPn(
    _In_ PVOID                                      pDeviceContext,
    _In_ CONST DXGKARG_RECOMMENDFUNCTIONALVIDPN*    pRecommendFunctionalVidPn
    )
{
    PAGED_CODE();
    DdiTrace("DDI:RecommendFunctionalVidPn");
    return StdVgaRecommendFunctionalVidPn(
        (PSTDVGA_DEVICE_CONTEXT)pDeviceContext,
        pRecommendFunctionalVidPn
        );
}

static NTSTATUS
StdVgaDdiRecommendMonitorModes(
    _In_ PVOID                                  pDeviceContext,
    _In_ CONST DXGKARG_RECOMMENDMONITORMODES*  pRecommendMonitorModes
    )
{
    PAGED_CODE();
    DdiTrace("DDI:RecommendMonitorModes");
    return StdVgaRecommendMonitorModes(
        (PSTDVGA_DEVICE_CONTEXT)pDeviceContext,
        pRecommendMonitorModes
        );
}

static NTSTATUS
StdVgaDdiEnumVidPnCofuncModality(
    _In_ PVOID                                      pDeviceContext,
    _In_ CONST DXGKARG_ENUMVIDPNCOFUNCMODALITY*    pEnumCofuncModality
    )
{
    PAGED_CODE();
    DdiTrace("DDI:EnumVidPnCofuncModality");
    return StdVgaEnumVidPnCofuncModality(
        (PSTDVGA_DEVICE_CONTEXT)pDeviceContext,
        pEnumCofuncModality
        );
}

static NTSTATUS
StdVgaDdiSetVidPnSourceVisibility(
    _In_ PVOID                                      pDeviceContext,
    _In_ CONST DXGKARG_SETVIDPNSOURCEVISIBILITY*   pSetVidPnSourceVisibility
    )
{
    PAGED_CODE();
    DdiTrace("DDI:SetVidPnSourceVisibility");
    return StdVgaSetVidPnSourceVisibility(
        (PSTDVGA_DEVICE_CONTEXT)pDeviceContext,
        pSetVidPnSourceVisibility
        );
}

static NTSTATUS
StdVgaDdiCommitVidPn(
    _In_ PVOID                          pDeviceContext,
    _In_ CONST DXGKARG_COMMITVIDPN*    pCommitVidPn
    )
{
    PAGED_CODE();
    DdiTrace("DDI:CommitVidPn");
    return StdVgaCommitVidPn(
        (PSTDVGA_DEVICE_CONTEXT)pDeviceContext,
        pCommitVidPn
        );
}

static NTSTATUS
StdVgaDdiUpdateActiveVidPnPresentPath(
    _In_ PVOID                                          pDeviceContext,
    _In_ CONST DXGKARG_UPDATEACTIVEVIDPNPRESENTPATH*   pUpdateActiveVidPnPresentPath
    )
{
    PAGED_CODE();
    DdiTrace("DDI:UpdateActiveVidPnPresentPath");
    return StdVgaUpdateActiveVidPnPresentPath(
        (PSTDVGA_DEVICE_CONTEXT)pDeviceContext,
        pUpdateActiveVidPnPresentPath
        );
}

static NTSTATUS
StdVgaDdiQueryVidPnHWCapability(
    _In_    PVOID                               pDeviceContext,
    _Inout_ DXGKARG_QUERYVIDPNHWCAPABILITY*    pVidPnHWCaps
    )
{
    PAGED_CODE();
    DdiTrace("DDI:QueryVidPnHWCapability");
    return StdVgaQueryVidPnHWCapability(
        (PSTDVGA_DEVICE_CONTEXT)pDeviceContext,
        pVidPnHWCaps
        );
}

static NTSTATUS
StdVgaDdiStopDeviceAndReleasePostDisplayOwnership(
    _In_  PVOID                              pDeviceContext,
    _In_  D3DDDI_VIDEO_PRESENT_TARGET_ID    TargetId,
    _Out_ DXGK_DISPLAY_INFORMATION*         pDisplayInfo
    )
{
    PAGED_CODE();
    DdiTrace("DDI:StopDeviceAndReleasePostDisplayOwnership");
    return StdVgaStopDeviceAndReleasePostDisplayOwnership(
        (PSTDVGA_DEVICE_CONTEXT)pDeviceContext,
        TargetId,
        pDisplayInfo
        );
}

//
// Full WDDM miniport DDI wrappers
//

static NTSTATUS
StdVgaDdiCreateDevice(
    _In_ PVOID pDeviceContext,
    _Inout_ DXGKARG_CREATEDEVICE* pCreateDevice
    )
{
    PAGED_CODE();
    DdiTrace("DDI:CreateDevice");
    return StdVgaCreateDevice((PSTDVGA_DEVICE_CONTEXT)pDeviceContext, pCreateDevice);
}

static NTSTATUS
StdVgaDdiDestroyDevice(
    _In_ PVOID pDeviceContext
    )
{
    PAGED_CODE();
    DdiTrace("DDI:DestroyDevice");
    return StdVgaDestroyDevice(pDeviceContext);
}

static NTSTATUS
StdVgaDdiCreateAllocation(
    _In_ PVOID pDeviceContext,
    _Inout_ DXGKARG_CREATEALLOCATION* pCreateAllocation
    )
{
    PAGED_CODE();
    DdiTrace("DDI:CreateAllocation");
    return StdVgaCreateAllocation((PSTDVGA_DEVICE_CONTEXT)pDeviceContext, pCreateAllocation);
}

static NTSTATUS
StdVgaDdiDestroyAllocation(
    _In_ PVOID pDeviceContext,
    _In_ CONST DXGKARG_DESTROYALLOCATION* pDestroyAllocation
    )
{
    PAGED_CODE();
    DdiTrace("DDI:DestroyAllocation");
    return StdVgaDestroyAllocation((PSTDVGA_DEVICE_CONTEXT)pDeviceContext, pDestroyAllocation);
}

static NTSTATUS
StdVgaDdiDescribeAllocation(
    _In_ PVOID pDeviceContext,
    _Inout_ DXGKARG_DESCRIBEALLOCATION* pDescribeAllocation
    )
{
    PAGED_CODE();
    DdiTrace("DDI:DescribeAllocation");
    return StdVgaDescribeAllocation((PSTDVGA_DEVICE_CONTEXT)pDeviceContext, pDescribeAllocation);
}

static NTSTATUS
StdVgaDdiGetStandardAllocationDriverData(
    _In_ PVOID pDeviceContext,
    _Inout_ DXGKARG_GETSTANDARDALLOCATIONDRIVERDATA* pStdAllocData
    )
{
    PAGED_CODE();
    DdiTrace("DDI:GetStandardAllocationDriverData");
    return StdVgaGetStandardAllocationDriverData(
        (PSTDVGA_DEVICE_CONTEXT)pDeviceContext, pStdAllocData);
}

static NTSTATUS
StdVgaDdiOpenAllocation(
    _In_ PVOID pDeviceContext,
    _In_ CONST DXGKARG_OPENALLOCATION* pOpenAllocation
    )
{
    PAGED_CODE();
    DdiTrace("DDI:OpenAllocation");
    return StdVgaOpenAllocation((PSTDVGA_DEVICE_CONTEXT)pDeviceContext, pOpenAllocation);
}

static NTSTATUS
StdVgaDdiCloseAllocation(
    _In_ PVOID pDeviceContext,
    _In_ CONST DXGKARG_CLOSEALLOCATION* pCloseAllocation
    )
{
    PAGED_CODE();
    DdiTrace("DDI:CloseAllocation");
    return StdVgaCloseAllocation((PSTDVGA_DEVICE_CONTEXT)pDeviceContext, pCloseAllocation);
}

static NTSTATUS
StdVgaDdiBuildPagingBuffer(
    _In_ PVOID pDeviceContext,
    _Inout_ DXGKARG_BUILDPAGINGBUFFER* pBuildPagingBuffer
    )
{
    PAGED_CODE();
    DdiTrace("DDI:BuildPagingBuffer");
    return StdVgaBuildPagingBuffer((PSTDVGA_DEVICE_CONTEXT)pDeviceContext, pBuildPagingBuffer);
}

static NTSTATUS
StdVgaDdiSubmitCommand(
    _In_ PVOID pDeviceContext,
    _In_ CONST DXGKARG_SUBMITCOMMAND* pSubmitCommand
    )
{
    DdiTrace("DDI:SubmitCommand");
    return StdVgaSubmitCommand((PSTDVGA_DEVICE_CONTEXT)pDeviceContext, pSubmitCommand);
}

static NTSTATUS
StdVgaDdiPatch(
    _In_ PVOID pDeviceContext,
    _In_ CONST DXGKARG_PATCH* pPatch
    )
{
    DdiTrace("DDI:Patch");
    return StdVgaPatch((PSTDVGA_DEVICE_CONTEXT)pDeviceContext, pPatch);
}

static NTSTATUS
StdVgaDdiPreemptCommand(
    _In_ PVOID pDeviceContext,
    _In_ CONST DXGKARG_PREEMPTCOMMAND* pPreemptCommand
    )
{
    DdiTrace("DDI:PreemptCommand");
    return StdVgaPreemptCommand((PSTDVGA_DEVICE_CONTEXT)pDeviceContext, pPreemptCommand);
}

static NTSTATUS
StdVgaDdiQueryCurrentFence(
    _In_ PVOID pDeviceContext,
    _Inout_ DXGKARG_QUERYCURRENTFENCE* pQueryCurrentFence
    )
{
    DdiTrace("DDI:QueryCurrentFence");
    return StdVgaQueryCurrentFence((PSTDVGA_DEVICE_CONTEXT)pDeviceContext, pQueryCurrentFence);
}

static NTSTATUS
StdVgaDdiPresent(
    _In_ PVOID pDeviceContext,
    _Inout_ DXGKARG_PRESENT* pPresent
    )
{
    DdiTrace("DDI:Present");
    return StdVgaPresent((PSTDVGA_DEVICE_CONTEXT)pDeviceContext, pPresent);
}

static NTSTATUS
StdVgaDdiRender(
    _In_ PVOID pDeviceContext,
    _Inout_ DXGKARG_RENDER* pRender
    )
{
    UNREFERENCED_PARAMETER(pDeviceContext);
    UNREFERENCED_PARAMETER(pRender);
    return STATUS_NOT_SUPPORTED;
}

static NTSTATUS
StdVgaDdiRenderKm(
    _In_ PVOID pDeviceContext,
    _Inout_ DXGKARG_RENDER* pRender
    )
{
    DdiTrace("DDI:Render");
    return StdVgaRender((PSTDVGA_DEVICE_CONTEXT)pDeviceContext, pRender);
}

static NTSTATUS
StdVgaDdiSetVidPnSourceAddress(
    _In_ PVOID pDeviceContext,
    _In_ CONST DXGKARG_SETVIDPNSOURCEADDRESS* pSetVidPnSourceAddress
    )
{
    DdiTrace("DDI:SetVidPnSourceAddress");
    return StdVgaSetVidPnSourceAddress(
        (PSTDVGA_DEVICE_CONTEXT)pDeviceContext, pSetVidPnSourceAddress);
}

static NTSTATUS
StdVgaDdiResetFromTimeout(
    _In_ PVOID pDeviceContext
    )
{
    DdiTrace("DDI:ResetFromTimeout");
    return StdVgaResetFromTimeout((PSTDVGA_DEVICE_CONTEXT)pDeviceContext);
}

static NTSTATUS
StdVgaDdiRestartFromTimeout(
    _In_ PVOID pDeviceContext
    )
{
    DdiTrace("DDI:RestartFromTimeout");
    return StdVgaRestartFromTimeout((PSTDVGA_DEVICE_CONTEXT)pDeviceContext);
}

static NTSTATUS
StdVgaDdiCollectDbgInfo(
    _In_ PVOID pDeviceContext,
    _In_ CONST DXGKARG_COLLECTDBGINFO* pCollectDbgInfo
    )
{
    DdiTrace("DDI:CollectDbgInfo");
    return StdVgaCollectDbgInfo((PSTDVGA_DEVICE_CONTEXT)pDeviceContext, pCollectDbgInfo);
}

static NTSTATUS
StdVgaDdiAcquireSwizzlingRange(
    _In_ PVOID pDeviceContext,
    _Inout_ DXGKARG_ACQUIRESWIZZLINGRANGE* pAcquireSwizzlingRange
    )
{
    DdiTrace("DDI:AcquireSwizzlingRange");
    return StdVgaAcquireSwizzlingRange(
        (PSTDVGA_DEVICE_CONTEXT)pDeviceContext, pAcquireSwizzlingRange);
}

static NTSTATUS
StdVgaDdiReleaseSwizzlingRange(
    _In_ PVOID pDeviceContext,
    _In_ CONST DXGKARG_RELEASESWIZZLINGRANGE* pReleaseSwizzlingRange
    )
{
    DdiTrace("DDI:ReleaseSwizzlingRange");
    return StdVgaReleaseSwizzlingRange(
        (PSTDVGA_DEVICE_CONTEXT)pDeviceContext, pReleaseSwizzlingRange);
}

#pragma code_seg()

static BOOLEAN
StdVgaDdiInterruptRoutine(
    _In_ PVOID pDeviceContext,
    _In_ ULONG MessageNumber
    )
{
    DdiTrace("DDI:InterruptRoutine");
    return StdVgaInterruptRoutine((PSTDVGA_DEVICE_CONTEXT)pDeviceContext, MessageNumber);
}

static VOID
StdVgaDdiDpcRoutine(
    _In_ PVOID pDeviceContext
    )
{
    StdVgaDpcRoutine((PSTDVGA_DEVICE_CONTEXT)pDeviceContext);
}

static NTSTATUS
StdVgaDdiSystemDisplayEnable(
    _In_  PVOID                              pDeviceContext,
    _In_  D3DDDI_VIDEO_PRESENT_TARGET_ID    TargetId,
    _In_  PDXGKARG_SYSTEM_DISPLAY_ENABLE_FLAGS Flags,
    _Out_ PUINT                             pWidth,
    _Out_ PUINT                             pHeight,
    _Out_ D3DDDIFORMAT*                     pColorFormat
    )
{
    DdiTrace("DDI:SystemDisplayEnable");
    return StdVgaSystemDisplayEnable(
        (PSTDVGA_DEVICE_CONTEXT)pDeviceContext,
        TargetId,
        Flags,
        pWidth,
        pHeight,
        pColorFormat
        );
}

static VOID
StdVgaDdiSystemDisplayWrite(
    _In_ PVOID  pDeviceContext,
    _In_ PVOID  pSource,
    _In_ UINT   SourceWidth,
    _In_ UINT   SourceHeight,
    _In_ UINT   SourceStride,
    _In_ UINT   PositionX,
    _In_ UINT   PositionY
    )
{
    StdVgaSystemDisplayWrite(
        (PSTDVGA_DEVICE_CONTEXT)pDeviceContext,
        pSource,
        SourceWidth,
        SourceHeight,
        SourceStride,
        PositionX,
        PositionY
        );
}

static VOID
StdVgaDdiUnload(VOID)
{
    DdiTrace("DDI:Unload");
}

static NTSTATUS
StdVgaDdiQueryInterface(
    _In_ PVOID pDeviceContext,
    _In_ PQUERY_INTERFACE pQueryInterface
    )
{
    UNREFERENCED_PARAMETER(pDeviceContext);
    DdiTrace("DDI:QueryInterface");
    if (pQueryInterface != NULL && pQueryInterface->InterfaceType != NULL)
    {
        // Print first 4 bytes of GUID as hex
        ULONG g0 = ((ULONG*)pQueryInterface->InterfaceType)[0];
        ULONG g1 = ((ULONG*)pQueryInterface->InterfaceType)[1];
        ULONG g2 = ((ULONG*)pQueryInterface->InterfaceType)[2];
        ULONG g3 = ((ULONG*)pQueryInterface->InterfaceType)[3];
        char buf[80];
        const char* hex = "0123456789ABCDEF";
        SIZE_T i = 0;
        const char* p = "DDI:QI g=";
        while (*p) buf[i++] = *p++;
        ULONG vals[4] = { g0, g1, g2, g3 };
        for (int v = 0; v < 4; v++)
        {
            for (int s = 28; s >= 0; s -= 4) buf[i++] = hex[(vals[v] >> s) & 0xF];
            buf[i++] = ' ';
        }
        buf[i] = 0;
        DdiTrace(buf);
    }
    return STATUS_NOT_SUPPORTED;
}

//
// KMDOD PresentDisplayOnly: dxgkrnl asks us to copy dirty rectangles from
// system memory to the framebuffer. Defer to the real implementation in
// device.cpp.
//
static NTSTATUS
StdVgaDdiPresentDisplayOnly(
    _In_ PVOID                          pDeviceContext,
    _In_ CONST DXGKARG_PRESENT_DISPLAYONLY* pPresentDisplayOnly
    )
{
    DdiTrace("DDI:PresentDisplayOnly");
    return StdVgaPresentDisplayOnly(
        (PSTDVGA_DEVICE_CONTEXT)pDeviceContext,
        pPresentDisplayOnly);
}

static NTSTATUS
StdVgaDdiNotifyAcpiEvent(
    _In_  PVOID                  pDeviceContext,
    _In_  DXGK_EVENT_TYPE        EventType,
    _In_  ULONG                  Event,
    _In_  PVOID                  Argument,
    _Out_ PULONG                 pAcpiFlags
    )
{
    UNREFERENCED_PARAMETER(pDeviceContext);
    UNREFERENCED_PARAMETER(EventType);
    UNREFERENCED_PARAMETER(Event);
    UNREFERENCED_PARAMETER(Argument);
    DdiTrace("DDI:NotifyAcpiEvent");
    if (pAcpiFlags) *pAcpiFlags = 0;
    return STATUS_SUCCESS;
}

static VOID
StdVgaDdiControlEtwLogging(
    _In_ BOOLEAN Enable,
    _In_ ULONG   Flags,
    _In_ UCHAR   Level
    )
{
    UNREFERENCED_PARAMETER(Enable);
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(Level);
}

static NTSTATUS
StdVgaDdiSetPalette(
    _In_ PVOID                       pDeviceContext,
    _In_ CONST DXGKARG_SETPALETTE*  pSetPalette
    )
{
    UNREFERENCED_PARAMETER(pDeviceContext);
    UNREFERENCED_PARAMETER(pSetPalette);
    DdiTrace("DDI:SetPalette");
    return STATUS_SUCCESS;
}

//
// DriverEntry - KMDOD (Display-only) Miniport via DxgkInitializeDisplayOnlyDriver
//
#pragma code_seg("INIT")

extern "C"
NTSTATUS
DriverEntry(
    _In_ DRIVER_OBJECT*  pDriverObject,
    _In_ UNICODE_STRING* pRegistryPath
    )
{
    KMDDOD_INITIALIZATION_DATA InitData = {};

    InitData.Version = DXGKDDI_INTERFACE_VERSION_WDDM1_3;

    // Adapter lifecycle
    InitData.DxgkDdiAddDevice                    = StdVgaDdiAddDevice;
    InitData.DxgkDdiStartDevice                  = StdVgaDdiStartDevice;
    InitData.DxgkDdiStopDevice                   = StdVgaDdiStopDevice;
    InitData.DxgkDdiRemoveDevice                 = StdVgaDdiRemoveDevice;
    InitData.DxgkDdiDispatchIoRequest            = StdVgaDdiDispatchIoRequest;
    InitData.DxgkDdiInterruptRoutine             = StdVgaDdiInterruptRoutine;
    InitData.DxgkDdiDpcRoutine                   = StdVgaDdiDpcRoutine;

    // Child/monitor
    InitData.DxgkDdiQueryChildRelations          = StdVgaDdiQueryChildRelations;
    InitData.DxgkDdiQueryChildStatus             = StdVgaDdiQueryChildStatus;
    InitData.DxgkDdiQueryDeviceDescriptor        = StdVgaDdiQueryDeviceDescriptor;

    InitData.DxgkDdiSetPowerState                = StdVgaDdiSetPowerState;
    InitData.DxgkDdiNotifyAcpiEvent              = StdVgaDdiNotifyAcpiEvent;
    InitData.DxgkDdiResetDevice                  = StdVgaDdiResetDevice;
    InitData.DxgkDdiUnload                       = StdVgaDdiUnload;
    InitData.DxgkDdiQueryInterface               = StdVgaDdiQueryInterface;
    InitData.DxgkDdiControlEtwLogging            = StdVgaDdiControlEtwLogging;

    InitData.DxgkDdiQueryAdapterInfo             = StdVgaDdiQueryAdapterInfo;
    InitData.DxgkDdiSetPalette                   = StdVgaDdiSetPalette;
    InitData.DxgkDdiSetPointerPosition           = StdVgaDdiSetPointerPosition;
    InitData.DxgkDdiSetPointerShape              = StdVgaDdiSetPointerShape;
    InitData.DxgkDdiEscape                       = StdVgaDdiEscape;
    InitData.DxgkDdiCollectDbgInfo               = StdVgaDdiCollectDbgInfo;

    // VidPn
    InitData.DxgkDdiIsSupportedVidPn             = StdVgaDdiIsSupportedVidPn;
    InitData.DxgkDdiRecommendFunctionalVidPn     = StdVgaDdiRecommendFunctionalVidPn;
    InitData.DxgkDdiEnumVidPnCofuncModality      = StdVgaDdiEnumVidPnCofuncModality;
    InitData.DxgkDdiSetVidPnSourceVisibility     = StdVgaDdiSetVidPnSourceVisibility;
    InitData.DxgkDdiCommitVidPn                  = StdVgaDdiCommitVidPn;
    InitData.DxgkDdiUpdateActiveVidPnPresentPath = StdVgaDdiUpdateActiveVidPnPresentPath;
    InitData.DxgkDdiRecommendMonitorModes        = StdVgaDdiRecommendMonitorModes;
    InitData.DxgkDdiQueryVidPnHWCapability       = StdVgaDdiQueryVidPnHWCapability;

    // KMDOD-specific
    InitData.DxgkDdiPresentDisplayOnly           = StdVgaDdiPresentDisplayOnly;
    InitData.DxgkDdiStopDeviceAndReleasePostDisplayOwnership =
        StdVgaDdiStopDeviceAndReleasePostDisplayOwnership;
    InitData.DxgkDdiSystemDisplayEnable          = StdVgaDdiSystemDisplayEnable;
    InitData.DxgkDdiSystemDisplayWrite           = StdVgaDdiSystemDisplayWrite;

    NTSTATUS status = DxgkInitializeDisplayOnlyDriver(
        pDriverObject,
        pRegistryPath,
        &InitData
        );

    return status;
}
