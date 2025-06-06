import usb.core
# import usb.util
from enum import IntFlag, Enum
import time
import struct
import array
from dataclasses import dataclass
from typing import List, Optional

XINPUT_VID = 0x045E
XINPUT_PID = 0x028E

class DeviceType(Enum):
    UNKNOWN = 0
    XINPUT = 1
    XID = 2
    XGIP = 3
    HID = 4

class UsbDescType(IntFlag):
    DEVICE = 0x01
    CONFIG = 0x02
    STRING = 0x03
    INTERFACE = 0x04
    ENDPOINT = 0x05
    HID = 0x21
    HID_REPORT = 0x22
    XID = 0x42
    XINPUT_AUTH = 0x41

    ITF_CLASS_HID = 0x03
    ITF_CLASS_XID = 0x58
    ITF_CLASS_XID_AUDIO = 0x78
    ITF_CLASS_VENDOR = 0xFF

    ITF_SUBCLASS_NONE = 0x00
    ITF_SUBCLASS_XID = 0x42
    ITF_SUBCLASS_XINPUT = 0x5D
    ITF_SUBCLASS_XINPUT_AUTH = 0xfd
    ITF_SUBCLASS_XGIP = 0x47

    ITF_PROTOCOL_NONE = 0x00
    ITF_PROTOCOL_XGIP = 0xd0
    ITF_PROTOCOL_XINPUT_HID = 0x01
    ITF_PROTOCOL_XINPUT_PLUGIN = 0x02
    ITF_PROTOCOL_XINPUT_AUDIO = 0x03
    ITF_PROTOCOL_XINPUT_AUTH = 0x13

class UsbReqType(IntFlag):
    DIR_HOST_TO_DEVICE = 0x00
    DIR_DEVICE_TO_HOST = 0x80

    TYPE_STANDARD = 0x00
    TYPE_CLASS = 0x20
    TYPE_VENDOR = 0x40

    RECIP_DEVICE = 0x00
    RECIP_INTERFACE = 0x01
    RECIP_ENDPOINT = 0x02
    RECIP_OTHER = 0x03

class UsbReq(IntFlag):
    GET_DESCRIPTOR = 0x06
    SET_ADDRESS = 0x05
    SET_CONFIGURATION = 0x09
    GET_STATUS = 0x00
    CLEAR_FEATURE = 0x01
    SET_FEATURE = 0x03

@dataclass
class UsbDescDevice:
    bLength: int
    bDescriptorType: int
    bcdUSB: int
    bDeviceClass: int
    bDeviceSubClass: int
    bDeviceProtocol: int
    bMaxPacketSize0: int
    idVendor: int
    idProduct: int
    bcdDevice: int
    iManufacturer: int
    iProduct: int
    iSerialNumber: int
    bNumConfigurations: int
    
    @classmethod
    def from_bytes(cls, data: bytes):
        if len(data) < 18:
            print(f"Warning: Device descriptor has {len(data)} bytes, expected 18")
            # Pad with zeros
            data = data + bytes(18 - len(data))
                
        fields = struct.unpack('<BBHBBBBHHHBBBB', data[:18])
        return cls(*fields)
    
    def __str__(self):
        return (f"Device Descriptor:\n"
                f"  bLength: {self.bLength}\n"
                f"  bDescriptorType: 0x{self.bDescriptorType:02x}\n"
                f"  bcdUSB: {self.bcdUSB >> 8}.{self.bcdUSB & 0xFF:02d}\n"
                f"  bDeviceClass: 0x{self.bDeviceClass:02x}\n"
                f"  bDeviceSubClass: 0x{self.bDeviceSubClass:02x}\n"
                f"  bDeviceProtocol: 0x{self.bDeviceProtocol:02x}\n"
                f"  bMaxPacketSize0: {self.bMaxPacketSize0}\n"
                f"  idVendor: 0x{self.idVendor:04x}\n"
                f"  idProduct: 0x{self.idProduct:04x}\n"
                f"  bcdDevice: {self.bcdDevice >> 8}.{self.bcdDevice & 0xFF:02d}\n"
                f"  iManufacturer: {self.iManufacturer}\n"
                f"  iProduct: {self.iProduct}\n"
                f"  iSerialNumber: {self.iSerialNumber}\n"
                f"  bNumConfigurations: {self.bNumConfigurations}\n")

@dataclass
class UsbDescConfig:
    bLength: int
    bDescriptorType: int
    wTotalLength: int
    bNumInterfaces: int
    bConfigurationValue: int
    iConfiguration: int
    bmAttributes: int
    bMaxPower: int
    
    @classmethod
    def from_bytes(cls, data: bytes):
        if len(data) < 9:
            raise ValueError(f"Config descriptor requires 9 bytes, got {len(data)}")
            
        fields = struct.unpack('<BBHBBBBB', data[:9])
        return cls(*fields)
    
    def __str__(self):
        self_powered = bool(self.bmAttributes & 0x40)
        remote_wakeup = bool(self.bmAttributes & 0x20)
        
        return (f"Configuration Descriptor:\n"
                f"  bLength: {self.bLength}\n"
                f"  bDescriptorType: 0x{self.bDescriptorType:02x}\n"
                f"  wTotalLength: {self.wTotalLength}\n"
                f"  bNumInterfaces: {self.bNumInterfaces}\n"
                f"  bConfigurationValue: {self.bConfigurationValue}\n"
                f"  iConfiguration: {self.iConfiguration}\n"
                f"  bmAttributes: 0x{self.bmAttributes:02x} "
                f"(Self-powered: {self_powered}, Remote Wakeup: {remote_wakeup})\n"
                f"  bMaxPower: {self.bMaxPower * 2}mA")

@dataclass
class UsbDescInterface:
    bLength: int
    bDescriptorType: int
    bInterfaceNumber: int
    bAlternateSetting: int
    bNumEndpoints: int
    bInterfaceClass: int
    bInterfaceSubClass: int
    bInterfaceProtocol: int
    iInterface: int
    
    @classmethod
    def from_bytes(cls, data: bytes):
        if len(data) < 9:
            raise ValueError(f"Interface descriptor requires 9 bytes, got {len(data)}")
            
        fields = struct.unpack('<BBBBBBBBB', data[:9])
        return cls(*fields)
    
    def __str__(self):
        return (f"Interface Descriptor:\n"
                f"  bLength: {self.bLength}\n"
                f"  bDescriptorType: 0x{self.bDescriptorType:02x}\n"
                f"  bInterfaceNumber: {self.bInterfaceNumber}\n"
                f"  bAlternateSetting: {self.bAlternateSetting}\n"
                f"  bNumEndpoints: {self.bNumEndpoints}\n"
                f"  bInterfaceClass: 0x{self.bInterfaceClass:02x}\n"
                f"  bInterfaceSubClass: 0x{self.bInterfaceSubClass:02x}\n"
                f"  bInterfaceProtocol: 0x{self.bInterfaceProtocol:02x}\n"
                f"  iInterface: {self.iInterface}")

@dataclass
class UsbDescEndpoint:
    bLength: int
    bDescriptorType: int
    bEndpointAddress: int
    bmAttributes: int
    wMaxPacketSize: int
    bInterval: int
    
    @classmethod
    def from_bytes(cls, data: bytes):
        if len(data) < 7:
            raise ValueError(f"Endpoint descriptor requires 7 bytes, got {len(data)}")
            
        fields = struct.unpack('<BBBBHB', data[:7])
        return cls(*fields)
    
    def __str__(self):
        direction = "IN" if self.bEndpointAddress & 0x80 else "OUT"
        ep_num = self.bEndpointAddress & 0x0F
        ep_type = self.bmAttributes & 0x03
        ep_type_str = ["Control", "Isochronous", "Bulk", "Interrupt"][ep_type]
        
        return (f"Endpoint Descriptor:\n"
                f"  bLength: {self.bLength}\n"
                f"  bDescriptorType: 0x{self.bDescriptorType:02x}\n"
                f"  bEndpointAddress: 0x{self.bEndpointAddress:02x} (EP{ep_num} {direction})\n"
                f"  bmAttributes: 0x{self.bmAttributes:02x} (Type: {ep_type_str})\n"
                f"  wMaxPacketSize: {self.wMaxPacketSize}\n"
                f"  bInterval: {self.bInterval}ms")

@dataclass
class UsbDescString:
    bLength: int
    bDescriptorType: int
    wString: str
    name: str
    
    @classmethod
    def from_bytes(cls, data, name=None, is_int=False):
        if hasattr(data, 'tobytes'):
            data = data.tobytes()
        elif isinstance(data, array.array):
            data = bytes(data)
            
        if len(data) < 2:
            raise ValueError(f"String descriptor requires at least 2 bytes, got {len(data)}")
            
        bLength = data[0]
        bDescriptorType = data[1]

        if is_int:
            wString = ""
            if bLength >= 4:
                langs = []
                for i in range(2, bLength, 2):
                    if i + 1 < bLength:
                        langs.append(f"0x{data[i+1]:02x}{data[i]:02x}")
                wString = f"{', '.join(langs)}"
        else:
            # String data is UTF-16LE
            if bLength > 2:
                try:
                    wString = data[2:bLength].decode('utf-16-le')
                except UnicodeDecodeError:
                    wString = f"Cannot decode: {' '.join([f'{x:02x}' for x in data[2:bLength]])}"
            else:
                wString = ""
            
        result = cls(bLength, bDescriptorType, wString, name or "String")
        return result
    
    def __str__(self):
        return (f"String Descriptor {self.name}:\n"
                f"  bLength: {self.bLength}\n"
                f"  bDescriptorType: 0x{self.bDescriptorType:02x}\n"
                f"  wString: '{self.wString}'")
    
@dataclass
class UsbDescClassHid:
    bLength: int
    bDescriptorType: int
    bcdHID: int
    bCountryCode: int
    bNumDescriptors: int
    bDescriptorTypeHIDReport: int
    wDescriptorLength: int
    
    @classmethod
    def from_bytes(cls, data: bytes):
        if len(data) < 9:
            raise ValueError(f"HID descriptor requires at least 9 bytes, got {len(data)}")
            
        fields = struct.unpack('<BBHBBHB', data[:9])
        return cls(*fields)
    
    def __str__(self):
        return (f"HID Class Descriptor:\n"
                f"  bLength: {self.bLength}\n"
                f"  bDescriptorType: 0x{self.bDescriptorType:02x}\n"
                f"  bcdHID: {self.bcdHID >> 8}.{self.bcdHID & 0xFF:02d}\n"
                f"  bCountryCode: {self.bCountryCode}\n"
                f"  bNumDescriptors: {self.bNumDescriptors}\n"
                f"  bDescriptorTypeHIDReport: 0x{self.bDescriptorTypeHIDReport:02x}\n"
                f"  wDescriptorLength: {self.wDescriptorLength}")
    
@dataclass
class UsbDescGeneric:
    bLength: int
    bDescriptorType: int
    raw_data: bytes  # Store all remaining data as raw bytes
    name: str = "Unknown Descriptor"
    
    @classmethod
    def from_bytes(cls, data: bytes, desc_name=None):
        if len(data) < 2:
            raise ValueError(f"Generic descriptor requires at least 2 bytes, got {len(data)}")
        
        bLength = data[0]
        bDescriptorType = data[1]
        
        # Store everything after the header as raw bytes
        raw_data = data[2:bLength] if len(data) >= bLength else data[2:]
        
        result = cls(bLength, bDescriptorType, raw_data)
        if desc_name is not None:
            result.name = desc_name

        return result
    
    def __str__(self):
        """Format the descriptor data for display"""
        # Basic header information
        result = [
            f"{self.name} Descriptor:",
            f"  bLength: {self.bLength}",
            f"  bDescriptorType: 0x{self.bDescriptorType:02x}"
        ]
        
        # Add raw data dump in a readable format
        result.append("  Unknown Data:")
        
        # Format in blocks of 8 bytes for readability
        for i in range(0, len(self.raw_data), 8):
            block = self.raw_data[i:i+8]
            hex_values = " ".join([f"0x{b:02x}" for b in block])
            ascii_values = "".join([chr(b) if 32 <= b <= 126 else "." for b in block])
            offset = i
            result.append(f"    {offset:04x}: {hex_values:<23} | {ascii_values}")
        
        return "\n".join(result)

@dataclass
class UsbConfiguration:
    descriptors: List
    config_number: int
    gp_type: DeviceType = DeviceType.UNKNOWN
    
    def __init__(self, config_data):
        self.descriptors = []
        desc_cfg_len = config_data[2]
        if len(config_data) < desc_cfg_len:
            raise ValueError(f"Configuration descriptor requires at least {desc_cfg_len} bytes, got {len(config_data)}")
        
        pos = 0
        protocol = 0

        while pos < desc_cfg_len:
            desc_len = config_data[pos]
            desc_type = config_data[pos + 1]
            
            match desc_type:
                case UsbDescType.CONFIG:
                    desc_cfg = UsbDescConfig.from_bytes(config_data[pos:pos + desc_len])
                    self.config_number = desc_cfg.bConfigurationValue
                    self.descriptors.append(desc_cfg)

                case UsbDescType.INTERFACE:
                    desc_itf = UsbDescInterface.from_bytes(config_data[pos:pos + desc_len])
                    protocol = desc_itf.bInterfaceProtocol

                    if self.gp_type == DeviceType.UNKNOWN:
                        match desc_itf.bInterfaceClass:
                            case UsbDescType.ITF_CLASS_VENDOR:
                                match desc_itf.bInterfaceSubClass:
                                    case UsbDescType.ITF_SUBCLASS_XINPUT:
                                        self.gp_type = DeviceType.XINPUT
                                    case UsbDescType.ITF_SUBCLASS_XINPUT_AUTH:
                                        self.gp_type = DeviceType.XINPUT
                                    case UsbDescType.ITF_SUBCLASS_XGIP:
                                        self.gp_type = DeviceType.XGIP
                            case UsbDescType.ITF_CLASS_XID:
                                if desc_itf.bInterfaceSubClass == UsbDescType.ITF_SUBCLASS_XID:
                                    self.gp_type = DeviceType.XID
                            case UsbDescType.ITF_CLASS_XID_AUDIO:
                                self.gp_type = DeviceType.XID
                            case UsbDescType.ITF_CLASS_HID:
                                self.gp_type = DeviceType.HID
                            case _:
                                self.gp_type = DeviceType.UNKNOWN

                    self.descriptors.append(desc_itf)

                case UsbDescType.HID:
                    if self.gp_type == DeviceType.XINPUT:
                        class_name = "XInput Unknown Class"
                        match protocol:
                            case UsbDescType.ITF_PROTOCOL_XINPUT_HID:
                                class_name = "XInput HID Class"
                            case UsbDescType.ITF_PROTOCOL_XINPUT_AUDIO:
                                class_name = "XInput Audio Class"
                            case UsbDescType.ITF_PROTOCOL_XINPUT_PLUGIN:
                                class_name = "XInput Plugin Class"
                            
                        desc_hid = UsbDescGeneric.from_bytes(config_data[pos:pos + desc_len], desc_name=class_name)
                    else:
                        desc_hid = UsbDescClassHid.from_bytes(config_data[pos:pos + desc_len])
                    self.descriptors.append(desc_hid)

                case UsbDescType.XINPUT_AUTH:
                    name = None
                    if self.gp_type == DeviceType.XINPUT:
                        name = "XInput Auth"
                    desc_auth = UsbDescGeneric.from_bytes(config_data[pos:pos + desc_len], desc_name=name)
                    self.descriptors.append(desc_auth)

                case UsbDescType.ENDPOINT:
                    desc_ep = UsbDescEndpoint.from_bytes(config_data[pos:pos + desc_len])
                    self.descriptors.append(desc_ep)

                case _:
                    unk_desc = UsbDescGeneric.from_bytes(config_data[pos:pos + desc_len])
                    self.descriptors.append(unk_desc)

            pos += desc_len

    def __str__(self):
        result = [f"/ ---- Configuration {self.config_number} ---- /\n"]
        
        for desc in self.descriptors:
            result.append(str(desc))
            
        return "\n".join(result)
    
@dataclass
class UsbDescReport:
    bDescriptorType: int
    data: bytes
    interface_number: int
    name: str = "Report Descriptor"  # Add this field with a default value
    
    def __str__(self):
        lines = [f"{self.name} for Interface {self.interface_number}:"]
        lines.append(f"  bDescriptorType: 0x{self.bDescriptorType:02x}")
        lines.append(f"  Size: {len(self.data)} bytes")
        lines.append("  Raw Data:")
        
        # Format in blocks of 16 bytes with both hex and ASCII
        for i in range(0, len(self.data), 16):
            block = self.data[i:i+16]
            hex_part = " ".join([f"{b:02x}" for b in block])
            ascii_part = "".join([chr(b) if 32 <= b <= 126 else "." for b in block])
            lines.append(f"    {i:04x}: {hex_part:<47} | {ascii_part}")
            
        return "\n".join(lines)
    
@dataclass
class UsbDevice:
    device: UsbDescDevice
    configurations: List[UsbConfiguration] = None
    strings: List[UsbDescString] = None
    reports: List[UsbDescReport] = None
    
    def __init__(self, device):
        self.device = device
        self.configurations = []
        self.strings = []
        self.reports = []
        
    def __str__(self):
        result = ["\n/ ---- USB Device ---- /\n"]
        result.append(str(self.device))

        for config in self.configurations:
            result.append(str(config))

        if self.strings:
            result.append("\n/ ---- String Descriptors ---- /\n")
            for string in self.strings or []:
                result.append(str(string))
                result.append("")

        if self.reports:
            result.append("\n/ ---- Misc Descriptors ---- /\n")
            for report in self.reports or []:
                result.append(str(report))
                result.append("")
            
        return "\n".join(result)
    
@dataclass
class GipHeader:
    """Parser for GIP protocol headers"""
    command: int      # Command ID (0x01-0x60)
    client: int       # 4 bits - identifies which component 
    needs_ack: bool   # Requires acknowledgement
    system: bool      # System vs device-specific command
    chunk_start: bool # Marks start of chunk sequence
    chunked: bool     # Part of a chunk sequence
    sequence: int     # Message sequence number (1-255)
    length: int       # Payload length (variable encoding)
    
    @classmethod
    def from_bytes(cls, data):
        if len(data) < 3:
            return None
            
        command = data[0]
        flags = data[1]
        sequence = data[2]
        
        # Parse flags byte
        client = flags & 0x0F
        needs_ack = bool(flags & 0x10)
        system = bool(flags & 0x20)
        chunk_start = bool(flags & 0x40)
        chunked = bool(flags & 0x80)
        
        # Parse variable length (LEB128 encoding)
        length = 0
        pos = 3
        shift = 0
        
        while pos < len(data):
            byte = data[pos]
            length |= (byte & 0x7F) << shift
            pos += 1
            shift += 7
            if not (byte & 0x80):
                break
                
        return cls(command, client, needs_ack, system, chunk_start, chunked, sequence, length)
    
def xgip_init(dev, usb_dev_info):
    epaddr = 0x82
    for desc in usb_dev_info.configurations[0].descriptors:
        if isinstance(desc, UsbDescEndpoint):
            if not (desc.bEndpointAddress & 0x80):
                epaddr = desc.bEndpointAddress & 0x0F
                break
    pwr_on = array.array('B', [0x05, 0x20, 0x00, 1, 0x00, 0x00])
    enable1 = array.array('B', [0x05, 0x20, 0x00, 15, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0x53, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00])
    led_on = array.array('B', [0x0A, 0x20, 0x00, 3, 0x00, 0x01, 0x14])
    enable2 = array.array('B', [0x4d, 0x10, 0x00, 0x02, 0x07, 0x00])
    
    try:
        print("Initializing XGIP communication...")
        dev.write(epaddr, pwr_on, timeout=1000)
        time.sleep(0.1)
        dev.write(epaddr, enable1, timeout=1000)
        time.sleep(0.1)
        dev.write(epaddr, led_on, timeout=1000)
        time.sleep(0.1)
        dev.write(epaddr, enable2, timeout=1000)
        time.sleep(0.1)
        
        print("XGIP initialized successfully.")
    except Exception as e:
        print(f"Error initializing XGIP: {e}")
        return False

def get_xgip_messages(dev, endpoint_in=0x82):
    """Read XGIP messages from the device's input endpoint"""
    messages = []
    
    # Find the correct endpoint
    if isinstance(endpoint_in, int):
        ep_addr = endpoint_in
    else:
        # Find first IN interrupt endpoint
        for config in dev.configurations:
            for intf in config:
                for ep in intf:
                    if ep.bEndpointAddress & 0x80 and ep.bmAttributes & 0x03 == 3:  # IN and Interrupt
                        ep_addr = ep.bEndpointAddress
                        break
    
    try:
        # Set configuration and claim interface
        try:
            dev.set_configuration()
        except:
            pass  # May already be configured
        
        # Try to read data from the endpoint
        print(f"Reading XGIP messages from endpoint 0x{ep_addr:02x}...")
        
        for _ in range(5):  # Try a few reads
            try:
                data = dev.read(ep_addr, 64, timeout=1000)
                
                if data and len(data) > 3:
                    print(f"Got {len(data)} bytes from endpoint")
                    
                    # Try to parse as GIP header
                    header = GipHeader.from_bytes(data)
                    if header:
                        print(f"Parsed GIP header: command=0x{header.command:02x}, length={header.length}")
                        
                        # Extract payload
                        payload_start = 3  # Basic header
                        # Add variable length bytes
                        temp = header.length
                        while temp >= 0x80:
                            payload_start += 1
                            temp >>= 7
                            
                        payload = data[payload_start:payload_start + header.length]
                        
                        # Add to messages
                        messages.append({
                            'header': header,
                            'payload': payload,
                            'raw': bytes(data)
                        })
            except Exception as e:
                print(f"Error reading endpoint: {e}")
                time.sleep(0.1)
                
    except Exception as e:
        print(f"Error setting up device: {e}")
    
    return messages
    
def parse_xgip_message(message):
    """Parse a XGIP message based on its command ID"""
    header = message['header']
    payload = message['payload']
    result = {"command_name": "Unknown"}
    
    match header.command:
        case 0x01:  # Acknowledgement
            if len(payload) >= 9:
                result = {
                    "command_name": "Acknowledgement",
                    "unk1": payload[0],
                    "inner_command": payload[1],
                    "inner_flags": payload[2],
                    "bytes_received": payload[3] | (payload[4] << 8),
                    "unk2": payload[5] | (payload[6] << 8),
                    "remaining_buffer": payload[7] | (payload[8] << 8)
                }
                
        case 0x02:  # Device Arrival
            if len(payload) >= 24:
                result = {
                    "command_name": "Device Arrival",
                    "serial": int.from_bytes(payload[0:8], byteorder='little'),
                    "vendor_id": payload[8] | (payload[9] << 8),
                    "product_id": payload[10] | (payload[11] << 8),
                    "firmware_version": {
                        "major": payload[12] | (payload[13] << 8),
                        "minor": payload[14] | (payload[15] << 8),
                        "build": payload[16] | (payload[17] << 8),
                        "revision": payload[18] | (payload[19] << 8)
                    },
                    "hardware_version": {
                        "major": payload[20] | (payload[21] << 8),
                        "minor": payload[22] | (payload[23] << 8),
                        "build": payload[24] | (payload[25] << 8),
                        "revision": payload[26] | (payload[27] << 8)
                    }
                }
                
        case 0x03:  # Device Status
            if len(payload) >= 4:
                result = {
                    "command_name": "Device Status",
                    "battery_level": payload[0] & 0x03,
                    "battery_type": (payload[0] >> 2) & 0x03,
                    "unknown": payload[1:4]
                }
                
        case 0x04:  # Device Descriptor
            result = {"command_name": "Device Descriptor"}
            # This would need custom chunk handling
            
        # Add more command parsing as needed
            
    return result
    
def get_xgip_descriptors(dev, usb_dev_info):
    """Get XGIP messages from an Xbox One/Series controller via endpoints"""
    results = []
    
    # Initialize reports list if needed
    if not hasattr(usb_dev_info, 'reports') or usb_dev_info.reports is None:
        usb_dev_info.reports = []
    
    print("Trying XGIP endpoint communication...")
    
    # Find the OUT endpoint for sending commands
    ep_out = 0x02  # Default EP2 OUT
    for desc in usb_dev_info.configurations[0].descriptors:
        if isinstance(desc, UsbDescEndpoint) and not (desc.bEndpointAddress & 0x80):
            ep_out = desc.bEndpointAddress & 0x0F
            break
    
    # Send the Device Descriptor request (command 0x04)
    try:
        print(f"Requesting XGIP device descriptor...")
        # Command 0x04, system flag (0x20), sequence 1, no data
        desc_req = array.array('B', [0x04, 0x20, 0x01, 0x00])
        dev.write(ep_out, desc_req, timeout=1000)
        
        # Try to read the descriptor response
        ep_in = 0x80 | ep_out  # Corresponding IN endpoint
        
        # Collect chunks of the descriptor
        chunks = []
        chunk_complete = False
        retries = 10
        
        while not chunk_complete and retries > 0:
            try:
                data = dev.read(ep_in, 64, timeout=1000)
                if data and len(data) > 4:
                    header = GipHeader.from_bytes(data)
                    if header and header.command == 0x04:
                        print(f"Got XGIP descriptor chunk: {len(data)} bytes")
                        chunks.append(bytes(data))
                        
                        # Check if this is the last chunk
                        if not header.chunked:
                            chunk_complete = True
                    elif header and header.command == 0x01:  # Acknowledgement
                        # Send next chunk request if needed
                        continue
            except Exception as e:
                print(f"Error reading descriptor: {e}")
                retries -= 1
                time.sleep(0.1)
        
        # Process the chunks if we got any
        if chunks:
            # Combine chunks and extract descriptor
            combined_data = b''.join([c[4:] for c in chunks])  # Skip header in each chunk
            
            if len(combined_data) > 20:  # Minimum size for a valid descriptor
                results.append(('XGIP_DESCRIPTOR', combined_data))
                
                # Try to parse the HID descriptor from the XGIP descriptor
                try:
                    # Parse header
                    header_len = combined_data[0] | (combined_data[1] << 8)
                    data_len = combined_data[header_len-2] | (combined_data[header_len-1] << 8)
                    
                    # Extract offsets
                    offsets_start = header_len
                    hid_offset_pos = offsets_start + 14  # Position of HID descriptor offset
                    
                    if len(combined_data) > hid_offset_pos + 1:
                        hid_offset = combined_data[hid_offset_pos] | (combined_data[hid_offset_pos+1] << 8)
                        
                        if hid_offset > 0:
                            # HID descriptor starts with count byte
                            hid_pos = offsets_start + hid_offset
                            if len(combined_data) > hid_pos:
                                hid_count = combined_data[hid_pos]
                                if hid_count > 0:
                                    # Extract the HID descriptor
                                    hid_data = combined_data[hid_pos+1:hid_pos+1+hid_count]
                                    results.append(('XGIP_HID_DESCRIPTOR', hid_data))
                                    print(f"Found HID descriptor in XGIP data: {len(hid_data)} bytes")
                except Exception as e:
                    print(f"Error parsing XGIP descriptor: {e}")
    
    except Exception as e:
        print(f"Error sending XGIP descriptor request: {e}")
    
    # Still try to get the MS descriptors through control transfers
    os_desc = get_descriptor(
        dev, 
        UsbDescType.STRING,
        desc_index=0xEE,
        wLength=0x12
    )
    
    if os_desc and len(os_desc) > 2:
        results.append(('MS_OS_DESC', os_desc))
    
    # Try to read actual XGIP messages from endpoints
    messages = get_xgip_messages(dev)
    
    if messages:
        for i, msg in enumerate(messages):
            results.append((f'XGIP_MSG_{i}', msg['raw']))
            
            # Parse the message
            parsed = parse_xgip_message(msg)
            print(f"  Parsed XGIP message: {parsed['command_name']}")
    
    # Add results to usb_device.reports list
    if results:
        for name, data in results:
            usb_dev_info.reports.append(UsbDescReport(
                bDescriptorType=0xF0,
                data=data,
                interface_number=0,
                name=name  # Add proper name
            ))
        return True
    
    return False

# Helper function to print descriptor data
def print_descriptor(desc_data, desc_name):
    if desc_data is not None:
        print(f"\n{desc_name} Descriptor:")
        print(f"  Length: {len(desc_data)} bytes")
        print(f"  Raw data: {' '.join([f'0x{x:02x}' for x in desc_data])}")

def get_descriptor(dev, desc_type, 
                   request_type=UsbReqType.DIR_DEVICE_TO_HOST | UsbReqType.TYPE_STANDARD | UsbReqType.RECIP_DEVICE, 
                   desc_index=0, wIndex=0, wLength=64):
    try:
        return  dev.ctrl_transfer(
                    bmRequestType=request_type,
                    bRequest=UsbReq.GET_DESCRIPTOR,
                    wValue=(desc_type << 8) | desc_index,
                    wIndex=wIndex,
                    data_or_wLength=wLength
                )
    except Exception as e:
        print(f"  Error getting descriptor type 0x{desc_type:02x}, index {desc_index}: {e}")
        return None
    
def xinput_set_led(dev, epaddr, led):
    cmd = array.array('B', [ 0x01, 0x03, 0x00 ])
    if led < 1 or led > 4:
        cmd[2] = 0
    else:
        cmd[2] = led + 5
    dev.write(epaddr, cmd, timeout=1000)

def get_report_descriptor(dev, interface_number, wLength=1024):
    """Get a HID report descriptor from an interface"""
    try:
        data = dev.ctrl_transfer(
            bmRequestType=UsbReqType.DIR_DEVICE_TO_HOST | UsbReqType.TYPE_CLASS | UsbReqType.RECIP_INTERFACE,
            bRequest=UsbReq.GET_DESCRIPTOR,
            wValue=(UsbDescType.HID_REPORT << 8),  # HID REPORT descriptor
            wIndex=interface_number,
            data_or_wLength=wLength
        )
        if data and len(data) > 0:
            return data
        
        return None
    except Exception as e:
        return None
        
def get_device_info(dev):
    """Parse all descriptors from a USB device"""
    device_data = get_descriptor(dev, UsbDescType.DEVICE)
    if device_data:
        usb_dev_info = UsbDevice(UsbDescDevice.from_bytes(device_data))
    else:
        print("Could not get device descriptor")
        return None
    
    for config_index in range(0, usb_dev_info.device.bNumConfigurations):
        config_data = get_descriptor(dev, UsbDescType.CONFIG, desc_index=config_index, wLength=512)

        if not config_data:
            print(f"Could not get configuration descriptor for index {config_index}")
            continue

        usb_configuration = UsbConfiguration(config_data)
        usb_dev_info.configurations.append(usb_configuration)

    lang_id = 0x0409 
    str_lang_data = get_descriptor(dev, UsbDescType.STRING, wIndex=0, wLength=0xFF)
    if str_lang_data and len(str_lang_data) >= 4:
        lang_id = struct.unpack('<H', str_lang_data[2:4])[0]
        usb_dev_info.strings = [UsbDescString.from_bytes(str_lang_data, name="0: Language ID", is_int=True)]
    else:
        print("Warning: Could not get language ID string descriptor, using default 0x0409")

    # if usb_device.configurations[0].is_xinput:
    #     print("XInput controller detected")
    #     epaddr = 2
    #     for desc in usb_device.configurations[0].descriptors:
    #         if isinstance(desc, UsbDescEndpoint):
    #             if not desc.bEndpointAddress & 0x80:
    #                 epaddr = desc.bEndpointAddress & 0x0F
    #                 break
    #     xinput_set_led(dev, epaddr, 1)

    if usb_dev_info.device.iManufacturer:
        idx = usb_dev_info.device.iManufacturer
        str_manufacturer =  get_descriptor(
                                dev, 
                                UsbDescType.STRING, 
                                wIndex=lang_id, 
                                desc_index=idx, 
                                wLength=0xFF
                            )
        if str_manufacturer:
            usb_dev_info.strings.append(
                UsbDescString.from_bytes(
                    str_manufacturer, 
                    name=f"{idx}: Manufacturer")
                )

    if usb_dev_info.device.iProduct:
        idx = usb_dev_info.device.iProduct
        str_product =   get_descriptor(
                            dev, 
                            UsbDescType.STRING, 
                            wIndex=lang_id, 
                            desc_index=idx, 
                            wLength=0xFF
                        )
        if str_product:
            usb_dev_info.strings.append(
                UsbDescString.from_bytes(
                    str_product, 
                    name=f"{idx}: Product")
                )

    if usb_dev_info.device.iSerialNumber:
        idx = usb_dev_info.device.iSerialNumber
        str_serial =    get_descriptor(
                            dev, 
                            UsbDescType.STRING, 
                            wIndex=lang_id, 
                            desc_index=idx, 
                            wLength=0xFF
                        )
        if str_serial:
            usb_dev_info.strings.append(
                UsbDescString.from_bytes(
                    str_serial, 
                    name=f"{idx}: Serial Number")
                )

    for desc in usb_dev_info.configurations[0].descriptors:
        if desc.bDescriptorType != UsbDescType.INTERFACE:
            continue
        if desc.iInterface == 0:
            continue
        str_interface = get_descriptor(
                            dev, 
                            desc_type=UsbDescType.STRING, 
                            wIndex=lang_id, 
                            desc_index=desc.iInterface, 
                            wLength=0xFF
                        )
        if str_interface:
            usb_dev_info.strings.append(
                UsbDescString.from_bytes(
                    str_interface, 
                    name=f"{desc.iInterface}: Interface {desc.bInterfaceNumber}")
                )
        else:
            print(f"Could not get interface string descriptor for index {idx}")

    for config in usb_dev_info.configurations:
        if config.gp_type == DeviceType.HID:
            descriptors = config.descriptors

            for i in range(len(descriptors)):
                if not isinstance(descriptors[i], UsbDescInterface):
                    continue
                elif i + 1 >= len(descriptors):
                    break
                elif descriptors[i].bInterfaceClass != UsbDescType.ITF_CLASS_HID:
                    continue

                itf = descriptors[i]
                itf_num = itf.bInterfaceNumber

                for j in range(i + 1, len(descriptors)):
                    if isinstance(descriptors[j], UsbDescInterface):
                        break
                    elif not isinstance(descriptors[j], UsbDescClassHid):
                        continue
                    elif descriptors[j].bDescriptorType != UsbDescType.HID_REPORT:
                        continue

                    report_len = descriptors[j].wDescriptorLength
                    report_desc = get_report_descriptor(dev, itf_num, report_len)

                    if report_desc:
                        usb_dev_info.reports.append(UsbDescReport(
                            bDescriptorType=descriptors[j].bDescriptorType,
                            data=report_desc,
                            interface_number=itf_num
                        ))
        elif config.gp_type == DeviceType.XGIP:
            xgip_init(dev, usb_dev_info)
            if get_xgip_descriptors(dev, usb_dev_info):
                print("XGIP descriptors found and added to the device.")
            else:
                print("No XGIP descriptors found.")

    return usb_dev_info

print("Looking for Xbox controller...")
dev = usb.core.find(idVendor=XINPUT_VID)
if dev is None:
    # Try with just vendor ID in case it's a different model
    dev = usb.core.find(idVendor=XINPUT_VID)
    if dev is None:
        print("No Microsoft controller found!")
        exit(1)

print(f"Found controller: {dev.idVendor:04x}:{dev.idProduct:04x}")

usb_device = get_device_info(dev)
if usb_device:
    print(usb_device)