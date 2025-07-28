#pragma once

#include <Protocol/UsbIo.h>
//

// #define DELL_USB_DEVICE_EVENT_ADD        1
// #define DELL_USB_DEVICE_EVENT_COMPLETE   2
#define MAX_STRING_LENGTH                64
#define USB_DEV_INFO_SIGNATURE SIGNATURE_32('u', 'd', 'i', 'f')

typedef enum {
  DELL_USB_DEVICE_EVENT_ADD = 1,
  DELL_USB_DEVICE_EVENT_COMPLETE = 2
} EVENT_TYPE;

#pragma pack(1)
typedef union
{
    struct
    {
        UINT8   Interface;
        UINT8   Port;
        UINT8   Fn;
        UINT8   Dev;
        UINT32  Bus;
    }   a;
    UINT64 i;
}   PORT_INFO;
#pragma pack()

typedef struct
{
    UINT32                          Signature;
    EFI_USB_DEVICE_DESCRIPTOR       DeviceDescriptor;
    EFI_USB_INTERFACE_DESCRIPTOR    InterfaceDescriptor;
    CHAR16                          Manufacturer[MAX_STRING_LENGTH];
    CHAR16                          Product[MAX_STRING_LENGTH];
    CHAR16                          SerialNumber[MAX_STRING_LENGTH];
    PORT_INFO                       PortInfo;
}   DELL_USB_DEV_INFO;

#define USB_DESC_TYPE_INTERFACE_ASSOC  0xB
typedef struct {
  UINT8           Length;
  UINT8           DescriptorType;
  UINT8           FirstInterface;
  UINT8           InterfaceCount;
  UINT8           FunctionClass;
  UINT8           FunctionSubClass;
  UINT8           FunctionProtocol;
  UINT8           Function;
} USB_INTERFACE_ASSOC_DESC;

typedef struct USB_EXT_PROTOCOL {
  DELL_USB_DEV_INFO USBDevInfo;
  EFI_EVENT UpdateEvent;
  EVENT_TYPE EventType;
} USB_EXT_PROTOCOL;

extern EFI_GUID gUsbExtProtocolGuid;
// Declare the GUID for this protocol
// 3a7f1e32-d5a5-498a-8c2c-192711761864
// { 0x3a7f1e32, 0xd5a5, 0x498a, { 0x8c, 0x2c, 0x19, 0x27, 0x11, 0x76, 0x18, 0x64 } }

























// #ifndef _USB_EXT_PROTOCOL_H_
// #define _USB_EXT_PROTOCOL_H_
// #pragma once

// #include <Protocol/UsbIo.h>
// #include <Uefi.h>


// // #define DELL_USB_DEVICE_EVENT_ADD        1
// // #define DELL_USB_DEVICE_EVENT_COMPLETE   2
// #define MAX_STRING_LENGTH                64
// #define USB_DEV_INFO_SIGNATURE SIGNATURE_32('u', 'd', 'i', 'f')
// // #pragma pack(1)


// typedef union
// {
//     struct
//     {
//         UINT8   Interface;
//         UINT8   Port;
//         UINT8   Fn;
//         UINT8   Dev;
//         UINT32  Bus;
//     }   a;
//     UINT64 i;
// }   PORT_INFO;
// // #pragma pack()

// typedef struct
// {
//     // UINT32                          Signature;
//     EFI_USB_DEVICE_DESCRIPTOR       DeviceDescriptor;
//     EFI_USB_INTERFACE_DESCRIPTOR    InterfaceDescriptor;
//     CHAR16                          Manufacturer[MAX_STRING_LENGTH];
//     CHAR16                          Product[MAX_STRING_LENGTH];
//     CHAR16                          SerialNumber[MAX_STRING_LENGTH];
//     // EVENT_TYPE                      EventType;
//     // EFI_EVENT                       UpdateEvent;
//     PORT_INFO                       PortInfo;
// }   USB_EXT_PROTOCOL;

// extern EFI_GUID gUsbExtProtocolGuid;
// // Declare the GUID for this protocol
// // 3a7f1e32-d5a5-498a-8c2c-192711761864
// // { 0x3a7f1e32, 0xd5a5, 0x498a, { 0x8c, 0x2c, 0x19, 0x27, 0x11, 0x76, 0x18, 0x64 } }



// typedef struct
// {
//     EFI_STATUS          Status;
//     UINTN               Cmd;
//     USB_EXT_PROTOCOL   Param;
// }   USB_INFO_EVENT_PARAMETER;

// #endif




// // #define USB_DESC_TYPE_INTERFACE_ASSOC  0xB
// // typedef struct {
// //   UINT8           Length;
// //   UINT8           DescriptorType;
// //   UINT8           FirstInterface;
// //   UINT8           InterfaceCount;
// //   UINT8           FunctionClass;
// //   UINT8           FunctionSubClass;
// //   UINT8           FunctionProtocol;
// //   UINT8           Function;
// // } USB_INTERFACE_ASSOC_DESC;
