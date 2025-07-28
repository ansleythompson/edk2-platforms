#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Protocol/DevicePathToText.h>
#include <Protocol/PciIo.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <IndustryStandard/Usb.h>
#include "DellUsbExt.h"

USB_EXT_PROTOCOL ProtocolInstance;

// STEp 1 : Entry Point

// things added:##################
EFI_EVENT Event = NULL;
VOID                        *mUsbIoNotifyReg = NULL;
VOID                        *mBlockIoNotifyReg = NULL;
// USB_INFO_EVENT_PARAMETER    *mEventParams = NULL;
USB_EXT_PROTOCOL            *mUsbExtProtocol = NULL;
CONST UINT16                mBmUsbLangId = 0x0409; // English
VOID EFIAPI CheckUsbIoProtocol(IN EFI_EVENT Event, IN VOID *Context);
EFI_STATUS GetUsbDevInfo(EFI_USB_IO_PROTOCOL *UsbIo, EFI_DEVICE_PATH_PROTOCOL *DevicePath);

// ###############################


EFI_STATUS EFIAPI DellUsbExtDxeEntryPoint(IN EFI_HANDLE ImageHandle, 
                                          IN EFI_SYSTEM_TABLE *SystemTable) {
  EFI_STATUS Status;
  EFI_EVENT   Event;
  EFI_HANDLE Handle = NULL;

  
  // entering the event
  DEBUG((DEBUG_INFO, "Entering the DellUsbExtDxeEntryPoint...\n"));
  mUsbExtProtocol = AllocateRuntimeZeroPool(sizeof(USB_EXT_PROTOCOL));
  if (!mUsbExtProtocol) {
    DEBUG((DEBUG_ERROR, "Allocate Runtime memory fail\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  // create event, check UsbIoProtocol
  Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_NOTIFY, CheckUsbIoProtocol, NULL, &Event);
  DEBUG((DEBUG_INFO, "CreateEvent for CheckUsbIoProtocol Status: %r\n", Status));

  // RegisterprotocolNotify
  if (!EFI_ERROR(Status)) {
     Status = gBS->RegisterProtocolNotify(&gEfiUsbIoProtocolGuid, Event, &mUsbIoNotifyReg);
     DEBUG((DEBUG_INFO, "RegisterProtocolNotify for gEfiUsbIoProtocolGuid Status: %r\n", Status));
  }

    // ###### Kabir - Install the USB_EXT_PROTOCOL from header file

    Status = gBS->InstallProtocolInterface(
        &Handle,
        &gUsbExtProtocolGuid,
        EFI_NATIVE_INTERFACE,
        mUsbExtProtocol
    );
    if (EFI_ERROR(Status)) 
    {
    DEBUG((DEBUG_INFO, "Installing the USB_EXT_PRTOCOL failed.\n"));
    return Status;
    }


  return EFI_SUCCESS;
}

CHAR16 *
DevicePathToStr(IN EFI_DEVICE_PATH_PROTOCOL *DevPath)
{
    EFI_STATUS                          Status;
    CHAR16                              *ToText;
    EFI_DEVICE_PATH_TO_TEXT_PROTOCOL    *DevPathToText;

    if (DevPath == NULL)
    {
        return NULL;
    }

    Status = gBS->LocateProtocol(&gEfiDevicePathToTextProtocolGuid,
                                 NULL,
                                 (VOID **) &DevPathToText);
    ASSERT_EFI_ERROR(Status);
    ToText = DevPathToText->ConvertDevicePathToText(DevPath,
                                                    FALSE,
                                                    TRUE);
    ASSERT(ToText != NULL);
    return ToText;
}


EFI_STATUS
GetPortInfo(IN EFI_DEVICE_PATH_PROTOCOL *DevicePath,
            IN OUT PORT_INFO            *PortInfo)
{
    EFI_STATUS                  Status;
    EFI_HANDLE                  TempHandle;
    EFI_PCI_IO_PROTOCOL         *PciIo;
    EFI_DEVICE_PATH_PROTOCOL    *Node;
    USB_DEVICE_PATH             *Usb;
    UINTN                       SegNumber, BusNumber, DevNumber, FunNumber;

    Node = DevicePath;
    PortInfo->i = 0;

    if (DevicePath == NULL)
    {
        return (EFI_NOT_FOUND);
    }

    Status = gBS->LocateDevicePath(&gEfiPciIoProtocolGuid, &Node, &TempHandle);
    if (EFI_ERROR(Status))
    {
        return Status;
    }

    Status = gBS->HandleProtocol(TempHandle, &gEfiPciIoProtocolGuid, (VOID**)&PciIo);
    if (EFI_ERROR(Status))
    {
        return Status;
    }
    
    Status = PciIo->GetLocation(PciIo,
                                &SegNumber,
                                &BusNumber,
                                &DevNumber,
                                &FunNumber);
    if (EFI_ERROR(Status))
    {
        return Status;
    }

    // Currently Node should be the remaining device path after Pci device path.
    // So the next Device Path Node should be Usb device path
    if (IsDevicePathEnd(Node))
    {
        return EFI_NOT_FOUND;
    }
    
    Usb = (USB_DEVICE_PATH *) Node;
    if ((DevicePathType(Usb) == MESSAGING_DEVICE_PATH) && (DevicePathSubType(Usb) == MSG_USB_DP))
    {
        PortInfo->a.Bus  = (UINT32)BusNumber;
        PortInfo->a.Dev  = (UINT8)DevNumber;
        PortInfo->a.Fn   = (UINT8)FunNumber;
        PortInfo->a.Port = Usb->ParentPortNumber;
        PortInfo->a.Interface = Usb->InterfaceNumber;
    }
    else
    {
        return EFI_NOT_FOUND;
    }
    
    DEBUG((DEBUG_INFO, "PortInfo %llx\n", PortInfo->i));
    return EFI_SUCCESS;
}

// step 2: checksbio protocol
VOID
CheckUsbIoProtocol(IN EFI_EVENT Event,
                   IN VOID      *Context)
{
    EFI_STATUS                  Status;
    EFI_USB_IO_PROTOCOL         *UsbIo = NULL;
    EFI_DEVICE_PATH_PROTOCOL    *DevicePath = NULL;
    UINTN                       BufferSize = 0;
    EFI_HANDLE                  Handle = NULL;

    DEBUG((EFI_D_INFO, "[%a] Entry\n", __FUNCTION__));

 

    while (TRUE)
    {
        BufferSize = sizeof(EFI_HANDLE);

        Status = gBS->LocateHandle(ByRegisterNotify, NULL, mUsbIoNotifyReg, &BufferSize, &Handle);

        if (EFI_ERROR(Status))
        {
            //
            // If no more notification events exist
            //
            break;
        }

        Status = gBS->HandleProtocol(Handle,
                                     &gEfiUsbIoProtocolGuid,
                                     (VOID **)&UsbIo);


        if (EFI_ERROR(Status))
        {
            DEBUG((DEBUG_INFO, "Fail to handle UsbIo protocol %x\n", Handle));
            continue;
        }

        Status = gBS->HandleProtocol(Handle,
                                     &gEfiDevicePathProtocolGuid,
                                     (VOID **)&DevicePath);

        if (EFI_ERROR(Status))
        {
            DEBUG((DEBUG_INFO, "Fail to handle DevicePath protocol %x\n", Handle));
            continue;
        }


        GetUsbDevInfo(UsbIo, DevicePath);
        // ######  Kabir - Send it to the USBmenu aka use signal event
        
        if (mUsbExtProtocol != NULL && mUsbExtProtocol->UpdateEvent != NULL) 
        {
            DEBUG((DEBUG_INFO, "Signaling UpdateEvent to notify USBMenu driver.....\n"));
            gBS->SignalEvent(mUsbExtProtocol->UpdateEvent);
        }
        else
        {
            DEBUG((DEBUG_INFO, "mUsbExtProtocol is NULL\n"));
        }
    }
    DEBUG((EFI_D_INFO, "[%a] Exit\n", __FUNCTION__));
    return;
}


// step 3: get usb dev info
EFI_STATUS
GetUsbDevInfo(EFI_USB_IO_PROTOCOL       *UsbIo,
              EFI_DEVICE_PATH_PROTOCOL  *DevicePath)
{
    EFI_STATUS                      Status;
    EFI_USB_DEVICE_DESCRIPTOR       *DeviceDescriptor;
    // EFI_USB_INTERFACE_DESCRIPTOR    *InterfaceDescriptor;
    CHAR16                          *Manufacturer = NULL;
    CHAR16                          *Product = NULL;
    CHAR16                          *SerialNumber = NULL;


    DEBUG((DEBUG_INFO, "DellUsbExtDxe: GetUsbDevInfo\n"));

    DeviceDescriptor = &mUsbExtProtocol->USBDevInfo.DeviceDescriptor;
    // InterfaceDescriptor = &mUsbExtProtocol->USBDevInfo.InterfaceDescriptor;
    Status = UsbIo->UsbGetDeviceDescriptor(UsbIo, DeviceDescriptor);
    if (EFI_ERROR(Status))
    {
        DEBUG((DEBUG_INFO, "Fail to get device descriptor\n"));
        DEBUG((DEBUG_INFO, "DellUsbExtDxe: GetUsbDevInfo Exit  Status: %r\n", Status));
        return Status;
    }

    // Status = UsbIo->UsbGetInterfaceDescriptor(UsbIo, InterfaceDescriptor);
    // if (EFI_ERROR(Status))
    // {
    //     DEBUG((DEBUG_INFO, "Fail to get interface descriptor\n"));
    //     DEBUG((DEBUG_INFO, "DellUsbExtDxe: GetUsbDevInfo Exit  Status: %r\n", Status));
    //     return Status;
    // }

    Status = UsbIo->UsbGetStringDescriptor(UsbIo,
                                           mBmUsbLangId,
                                           DeviceDescriptor->StrManufacturer,
                                           &Manufacturer);
    Status = UsbIo->UsbGetStringDescriptor(UsbIo,
                                           mBmUsbLangId,
                                           DeviceDescriptor->StrProduct,
                                           &Product);
    Status = UsbIo->UsbGetStringDescriptor(UsbIo,
                                           mBmUsbLangId,
                                           DeviceDescriptor->StrSerialNumber,
                                           &SerialNumber);

    DEBUG((DEBUG_INFO, "UsbDeviceDesc: Vendor %x, Product %x Class %x Subclass %x\n", DeviceDescriptor->IdVendor, DeviceDescriptor->IdProduct, DeviceDescriptor->DeviceClass, DeviceDescriptor->DeviceSubClass));

    // DEBUG((DEBUG_INFO, "Interface Class %x Subclass %x, Protocol %x\n", InterfaceDescriptor->InterfaceClass, InterfaceDescriptor->InterfaceSubClass, InterfaceDescriptor->InterfaceProtocol));

    DEBUG((DEBUG_INFO, "Manufacture    %s\n", Manufacturer));
    DEBUG((DEBUG_INFO, "Product        %s\n", Product));
    DEBUG((DEBUG_INFO, "SerialNumber   %s\n", SerialNumber));
    DEBUG((DEBUG_INFO, "DevicePath     %s\n", DevicePathToStr(DevicePath)));

    // mEventParams->Param.Signature = USB_DEV_INFO_SIGNATURE;
    if (Manufacturer)
    {
        CopyMem(mUsbExtProtocol->USBDevInfo.Manufacturer, Manufacturer, MAX_STRING_LENGTH);
    }
    else
    {
        ZeroMem(mUsbExtProtocol->USBDevInfo.Manufacturer, MAX_STRING_LENGTH);
    }

    if (Product)
    {
        CopyMem(mUsbExtProtocol->USBDevInfo.Product, Product, MAX_STRING_LENGTH);
    }
    else
    {
        ZeroMem(mUsbExtProtocol->USBDevInfo.Product, MAX_STRING_LENGTH);
    }

    if (SerialNumber)
    {
        CopyMem(mUsbExtProtocol->USBDevInfo.SerialNumber, SerialNumber, MAX_STRING_LENGTH);
    }
    else
    {
        ZeroMem(mUsbExtProtocol->USBDevInfo.SerialNumber, MAX_STRING_LENGTH);
    }

    Status = GetPortInfo(DevicePath, &mUsbExtProtocol->USBDevInfo.PortInfo);

  
    if (Manufacturer)
    {
        FreePool(Manufacturer);
    }
    if (Product)
    {
        FreePool(Product);
    }
    if (SerialNumber)
    {
        FreePool(SerialNumber);
    }

    DEBUG((DEBUG_INFO, "DellUsbExtDxe: GetUsbDevInfo Exit  Status: %r\n", Status));

    return Status;
}
