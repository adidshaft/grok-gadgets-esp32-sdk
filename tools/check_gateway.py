"""Optional sibling integration: simulated actual firmware -> PTY USB -> gateway."""

import asyncio
import json
import os
from pathlib import Path
import pty
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
gateway_root = root.parent / "grok-gadgets-gateway"
sys.path.insert(0, str(gateway_root / "src"))
from grok_gadgets_gateway.domain import Gateway
from grok_gadgets_gateway.transport import Credentials, DeviceServer
from grok_gadgets_gateway.usb_bridge import bridge


async def until(condition, seconds=8):
    end = asyncio.get_running_loop().time() + seconds
    while not condition():
        if asyncio.get_running_loop().time() > end:
            raise AssertionError("Timed out waiting for integration assertion")
        await asyncio.sleep(0.02)


async def main():
    device_id = "c124-123456789abc"
    token = "local-test-token-not-a-live-secret"
    master, slave = pty.openpty()
    serial_port = os.ttyname(slave)
    os.set_blocking(master, False)
    loop = asyncio.get_running_loop()
    with tempfile.TemporaryDirectory() as tmp:
        credentials = Path(tmp) / "devices.json"

        def credential(revoked=False):
            credentials.write_text(
                json.dumps({"devices": {device_id: {"token": token, "revoked": revoked}}})
            )
            credentials.chmod(0o600)

        credential()
        gateway = Gateway()
        server = await DeviceServer(gateway, Credentials(credentials), port=0).start()
        port = server.port
        stop = asyncio.Event()
        bridge_task = asyncio.create_task(bridge(serial_port, token, port=port, stop=stop))
        process = await asyncio.create_subprocess_exec(
            root / "build/host_firmware",
            stdin=asyncio.subprocess.PIPE,
            stdout=asyncio.subprocess.PIPE,
        )

        async def outgoing():
            while data := await process.stdout.read(4096):
                os.write(master, data)

        def incoming():
            try:
                data = os.read(master, 4096)
            except BlockingIOError:
                return
            process.stdin.write(data)

        loop.add_reader(master, incoming)
        pump = asyncio.create_task(outgoing())
        try:
            await until(lambda: device_id in gateway.devices)
            assert gateway.devices[device_id]["simulated"] is True
            gateway.command(
                device_id, "rgb.set", {"r": 0, "g": 255, "b": 0, "on": True}, "integration-green"
            )
            await until(lambda: gateway.command_status("integration-green")["status"] == "executed")
            assert gateway.devices[device_id]["state"]["rgb"] == {
                "r": 0,
                "g": 255,
                "b": 0,
                "on": True,
            }
            old_session = gateway.devices[device_id]["session_id"]
            await server.close()
            await until(lambda: not gateway.devices[device_id]["connected"])
            server = await DeviceServer(gateway, Credentials(credentials), port=port).start()
            await until(
                lambda: gateway.devices[device_id]["connected"]
                and gateway.devices[device_id]["session_id"] != old_session
            )
            gateway.command(
                device_id, "rgb.set", {"r": 0, "g": 0, "b": 0, "on": False}, "integration-off"
            )
            await until(lambda: gateway.command_status("integration-off")["status"] == "executed")
            assert gateway.devices[device_id]["state"]["rgb"]["on"] is False
            credential(True)
            await until(lambda: not gateway.devices[device_id]["connected"])
            credential(False)
            await until(lambda: gateway.devices[device_id]["connected"])
            print(
                "integration: simulated firmware -> PTY -> bridge -> authenticated gateway; "
                "green/off, server restart, revocation/restoration passed"
            )
        finally:
            loop.remove_reader(master)
            process.terminate()
            await process.wait()
            pump.cancel()
            await asyncio.gather(pump, return_exceptions=True)
            stop.set()
            await asyncio.wait_for(bridge_task, timeout=2)
            await server.close()
            os.close(master)
            os.close(slave)


asyncio.run(main())
