# mDNS OTA Server Discovery

This feature allows the Xiaozhi ESP32 device to automatically discover OTA servers on the local network using mDNS (Multicast DNS) before falling back to the configured OTA URL.

## Overview

When enabled, the device will:
1. First attempt to discover an OTA server via mDNS using the service type `_xiaozhi._tcp`
2. If a server is found, use its URL for OTA operations
3. If no server is found or mDNS fails, fall back to the configured OTA URL

This is particularly useful for local development and deployment scenarios where:
- The Xiaozhi server is running locally (e.g., at home)
- The server may not have a fixed IP address or port
- You want automatic discovery without manual configuration

## API Usage

### Discovery Class (Recommended)

The Discovery class provides a modern, RAII-based approach to mDNS discovery with automatic resource management:

```cpp
#include "discover.h"

// Create and initialize Discovery instance
auto discovery = Discovery::Init();
if (!discovery) {
    ESP_LOGE(TAG, "Failed to initialize Discovery");
    return;
}

// Discover OTA servers
std::string ota_url = discovery->DiscoverOtaServer(3000); // 3 second timeout
if (!ota_url.empty()) {
    ESP_LOGI(TAG, "Found OTA server: %s", ota_url.c_str());
}

// Discovery automatically cleans up when it goes out of scope
```

### Legacy Functions (Backward Compatibility)

For backward compatibility, the original function-based API is still available:

```cpp
#include "discover.h"

// Manual initialization
if (!InitializeMdns()) {
    ESP_LOGE(TAG, "Failed to initialize mDNS");
    return;
}

// Discover OTA servers
std::string ota_url = DiscoverOtaServer(3000);

// Manual cleanup
DeinitializeMdns();
```

**Note:** The Discovery class approach is recommended for new code as it provides automatic resource management and prevents resource leaks.

### Benefits of the Discovery Class

1. **Automatic Resource Management**: The Discovery class uses RAII (Resource Acquisition Is Initialization) to automatically initialize mDNS in the constructor and clean it up in the destructor.

2. **Exception Safety**: If an exception occurs, the destructor is automatically called, ensuring proper cleanup.

3. **No Resource Leaks**: Unlike the legacy functions, you cannot forget to call cleanup - it happens automatically.

4. **Thread Safety**: Each Discovery instance manages its own state, reducing the risk of race conditions.

5. **Factory Pattern**: The `Init()` factory method ensures that only properly initialized instances are created.

6. **Clear Ownership**: Using `std::unique_ptr` makes ownership and lifetime management explicit.

### Integration Example

```cpp
class MyOtaManager {
private:
    std::unique_ptr<Discovery> discovery_;

public:
    MyOtaManager() {
        discovery_ = Discovery::Init();
        if (!discovery_) {
            ESP_LOGW(TAG, "Discovery not available");
        }
    }

    std::string FindOtaServer() {
        if (!discovery_ || !discovery_->IsInitialized()) {
            return "";
        }
        return discovery_->DiscoverOtaServer(3000);
    }

    // Destructor automatically cleans up Discovery
    ~MyOtaManager() = default;
};
```

## Configuration

### Enable mDNS OTA Discovery

The feature is disabled by default. To enable it:

1. **Via menuconfig:**
   ```bash
   idf.py menuconfig
   ```
   Navigate to: `Xiaozhi Assistant` → `Enable mDNS OTA Server Discovery`

2. **Via sdkconfig:**
   Add the following line to your `sdkconfig` file:
   ```
   CONFIG_USE_MDNS_OTA_DISCOVERY=y
   ```

### Server Requirements

For the mDNS discovery to work, your OTA server must:

1. **Advertise the mDNS service** with type `_xiaozhi._tcp`
2. **Include the following information:**
   - Service name: `_xiaozhi._tcp`
   - Port: The port your OTA server is listening on
   - Optional TXT record: `path=/your/ota/path` (if OTA endpoint is not at root)

### Example Server Advertisement

Here's an example of how to advertise your OTA server using Python's `zeroconf` library:

```python
from zeroconf import ServiceInfo, Zeroconf
import socket

def advertise_ota_server(port=8080, path="/ota/"):
    zeroconf = Zeroconf()
    
    # Get local IP address
    hostname = socket.gethostname()
    local_ip = socket.gethostbyname(hostname)
    
    # Create service info
    service_info = ServiceInfo(
        "_xiaozhi._tcp.local.",
        "xiaozhi-ota._xiaozhi._tcp.local.",
        addresses=[socket.inet_aton(local_ip)],
        port=port,
        properties={
            "path": path.encode('utf-8')
        },
        server=f"{hostname}.local."
    )
    
    # Register the service
    zeroconf.register_service(service_info)
    print(f"OTA server advertised at {local_ip}:{port}{path}")
    
    return zeroconf, service_info

# Usage
zeroconf, service_info = advertise_ota_server(port=8080, path="/ota/")

# Keep the service running
try:
    input("Press Enter to stop advertising...\n")
finally:
    zeroconf.unregister_service(service_info)
    zeroconf.close()
```

## How It Works

1. **Initialization:** When the device starts and connects to WiFi, mDNS is initialized (if enabled)

2. **Discovery Process:** When checking for OTA updates, the device:
   - Sends an mDNS query for `_xiaozhi._tcp` services
   - Waits for responses (default timeout: 3 seconds)
   - Parses the first valid IPv4 response

3. **URL Construction:** The discovered URL is constructed as:
   ```
   http://<discovered_ip>:<discovered_port>[<path_from_txt_record>]
   ```

4. **Fallback:** If discovery fails or no servers are found, the device uses the configured OTA URL

## Troubleshooting

### mDNS Discovery Not Working

1. **Check if feature is enabled:**
   ```bash
   grep CONFIG_USE_MDNS_OTA_DISCOVERY sdkconfig
   ```

2. **Check network connectivity:**
   - Ensure device and server are on the same network
   - Verify multicast traffic is allowed on your network

3. **Check server advertisement:**
   - Use tools like `avahi-browse` (Linux) or `dns-sd` (macOS) to verify service advertisement:
     ```bash
     # Linux
     avahi-browse -rt _xiaozhi._tcp
     
     # macOS
     dns-sd -B _xiaozhi._tcp
     ```

4. **Check logs:**
   Look for mDNS-related log messages:
   ```
   I (12345) Discover: Discovering OTA server via mDNS (_xiaozhi._tcp)
   I (12678) Discover: Found service: xiaozhi-ota
   I (12679) Discover: Discovered OTA server: http://192.168.1.100:8080/ota/
   ```

### Common Issues

- **Firewall blocking multicast:** Ensure UDP port 5353 is open
- **Network isolation:** Some networks isolate devices from each other
- **Service not properly advertised:** Verify the server is correctly advertising the service
- **Timeout too short:** The default 3-second timeout might be too short for some networks

## Development and Testing

For development, you can test the mDNS discovery using command-line tools:

```bash
# Test mDNS query
dig @224.0.0.251 -p 5353 _xiaozhi._tcp.local PTR

# Browse for services (Linux)
avahi-browse -rt _xiaozhi._tcp

# Browse for services (macOS)
dns-sd -B _xiaozhi._tcp
```

## Security Considerations

- mDNS operates on the local network only
- No authentication is performed on discovered services
- Ensure your local network is trusted
- Consider using HTTPS for OTA servers when possible (though ESP32 HTTP client configuration may be needed)
