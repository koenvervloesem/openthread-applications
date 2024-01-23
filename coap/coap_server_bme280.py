"""CoAP server that receives BME280 sensor measurements.

Copyright (c) 2024 Koen Vervloesem

SPDX-License-Identifier: MIT
"""
import asyncio
import json
import logging

import aiocoap
from aiocoap import resource

logging.basicConfig(level=logging.INFO)
logging.getLogger("coap-server-bme280").setLevel(logging.DEBUG)


class Sensor(resource.Resource):
    """CoAP resource for a sensor."""

    async def render_put(self, request):
        """Handle CoAP PUT request and process sensor measurement."""
        payload = json.loads(request.payload)
        print(payload["id"])
        print(f"- Temperature : {payload['temp']} °C")
        print(f"- Pressure    : {payload['press']} hPa")
        print(f"- Humidity    : {payload['hum']} %")
        return aiocoap.Message(no_response=True)


async def main():
    """Create a CoAP resource and start the server."""
    root = resource.Site()
    root.add_resource(["sensor"], Sensor())

    await aiocoap.Context.create_server_context(root)
    await asyncio.get_running_loop().create_future()


if __name__ == "__main__":
    asyncio.run(main())
