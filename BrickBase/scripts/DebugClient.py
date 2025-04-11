# scan_and_show.py
import asyncio
from bleak import BleakScanner, BleakClient


async def select_device():
    print("🔍 Scanning for BLE devices...")
    devices = await BleakScanner.discover()
    if not devices:
        print("❌ No BLE devices found.")
        return None

    for i, d in enumerate(devices):
        print(f"[{i}] {d.name or '(unknown)'} - {d.address}")

    while True:
        idx = input("🔢 Select device index: ").strip()
        if idx.isdigit() and int(idx) < len(devices):
            return devices[int(idx)].address
        print("⚠️ Invalid selection.")


async def show_services(address):
    async with BleakClient(address) as client:
        if not await client.is_connected():
            print("❌ Failed to connect.")
            return

        print(f"\n✅ Connected to {address}\n")

        services = await client.get_services()
        all_chars = []

        for service in services:
            print(f"🔷 Service {service.uuid}")
            for char in service.characteristics:
                all_chars.append(char)
                print(f"  └── 🔸 Char {char.uuid}")
                print(f"      ├─ Properties: {', '.join(char.properties)}")
                print(f"      ├─ Handle: {char.handle}")
                if "read" in char.properties:
                    try:
                        value = await client.read_gatt_char(char.uuid)
                        print(f"      └─ Value: {value}")
                    except Exception:
                        print(f"      └─ Value: <read error>")
                else:
                    print(f"      └─ Value: <not readable>")

        while True:
            idx = input("\n🔢 Enter char index to read/write (or q to quit): ").strip()
            if idx.lower() == 'q':
                break

            try:
                index = int(idx)
                char = all_chars[index]
            except (ValueError, IndexError):
                print("⚠️ Invalid index.")
                continue

            action = input("👉 Action [r]ead / [w]rite / [b]oth: ").strip().lower()

            if action in ('r', 'b') and "read" in char.properties:
                try:
                    value = await client.read_gatt_char(char.uuid)
                    print(f"📥 Value: {value} (hex: {value.hex()})")
                except Exception as e:
                    print(f"❌ Read error: {e}")
            elif action == 'r':
                print("⚠️ Not readable.")

            if action in ('w', 'b') and "write" in char.properties:
                hex_input = input("✏️ Enter hex bytes to write (e.g. 01ff): ").strip()
                try:
                    await client.write_gatt_char(char.uuid, bytes.fromhex(hex_input))
                    print("✅ Write successful.")
                except Exception as e:
                    print(f"❌ Write error: {e}")
            elif action == 'w':
                print("⚠️ Not writable.")


async def main():
    address = await select_device()
    if address:
        await show_services(address)


if __name__ == "__main__":
    asyncio.run(main())
