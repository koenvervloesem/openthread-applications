"""Toggle an LED using CoAP and DNS-SD.

Copyright (c) 2024 Koen Vervloesem

SPDX-License-Identifier: MIT
"""
from __future__ import annotations

import asyncio
import logging

from aiocoap import PUT, Context, Message
from zeroconf import IPVersion, ServiceStateChange, Zeroconf
from zeroconf.asyncio import (
    AsyncServiceBrowser,
    AsyncServiceInfo,
    AsyncZeroconf,
)


def async_on_service_state_change(
    zeroconf: Zeroconf,
    service_type: str,
    name: str,
    state_change: ServiceStateChange,
) -> None:
    """Send CoAP request when a service is added."""
    print(f"Service {name} of type {service_type} state changed: {state_change}")
    if state_change is not ServiceStateChange.Added:
        return
    asyncio.ensure_future(async_send_coap_request(zeroconf, service_type, name))


async def async_send_coap_request(
    zeroconf: Zeroconf,
    service_type: str,
    name: str,
) -> None:
    """Send PUT message to CoAP server."""
    info = AsyncServiceInfo(service_type, name)
    await info.async_request(zeroconf, 3000)
    if info:
        context = await Context.create_client_context()
        request = Message(code=PUT, payload=b"2", uri=f"coap://{info.server}/led")
        response = await context.request(request).response
        print(f"Response code: {response.code}")
        print(f"Response payload: {response.payload.decode('utf-8')}")
        asyncio.get_running_loop().stop()
    else:
        print("  No info")


class AsyncRunner:
    """Helper class to use mDNS service discovery."""

    def __init__(self):
        """Initialize mDNS service discovery."""
        self.aiobrowser: AsyncServiceBrowser | None = None
        self.aiozc: AsyncZeroconf | None = None

    async def async_run(self) -> None:
        """Browse DNS-SD services."""
        self.aiozc = AsyncZeroconf(ip_version=IPVersion.V6Only)

        services = ["_example._udp.local."]
        print(f"Browsing {','.join(services)} service(s), press Ctrl-C to exit...\n")
        self.aiobrowser = AsyncServiceBrowser(
            self.aiozc.zeroconf,
            services,
            handlers=[async_on_service_state_change],
        )
        while True:
            await asyncio.sleep(1)

    async def async_close(self) -> None:
        """Close mDNS service discovery."""
        await self.aiobrowser.async_cancel()
        await self.aiozc.async_close()


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)

    loop = asyncio.get_event_loop()
    runner = AsyncRunner()
    try:
        loop.run_until_complete(runner.async_run())
    except (KeyboardInterrupt, RuntimeError):
        loop.run_until_complete(runner.async_close())
