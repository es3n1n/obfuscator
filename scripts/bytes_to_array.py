#!/usr/bin/env python3

buf = ''
while True:
    try:
        buf += input()
    except (KeyboardInterrupt, EOFError):
        break

data_str = ', '.join([f'0x{byte:02X}' for byte in bytes.fromhex(buf)])
print('\nconstexpr auto kData = std::to_array<std::uint8_t>({%data%});'.replace('%data%', data_str))
