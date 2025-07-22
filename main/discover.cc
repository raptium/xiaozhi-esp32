#include "discover.h"

#include <esp_log.h>
#include <string>
#include <mdns.h>
#include <esp_netif.h>
#include <sstream>
#include <cstring>
#include <memory>

#define TAG "Discover"

// Discovery class implementation
Discovery::Discovery() : mdns_initialized_(false) {
}

std::unique_ptr<Discovery> Discovery::Init() {
    auto discovery = std::unique_ptr<Discovery>(new Discovery());
    if (discovery->Initialize()) {
        return discovery;
    }
    return nullptr;
}

Discovery::~Discovery() {
    Deinitialize();
}

bool Discovery::Initialize() {
    if (mdns_initialized_) {
        return true;
    }

    esp_err_t err = mdns_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize mDNS: %s", esp_err_to_name(err));
        return false;
    }

    mdns_initialized_ = true;
    ESP_LOGI(TAG, "mDNS initialized successfully");
    return true;
}

void Discovery::Deinitialize() {
    if (mdns_initialized_) {
        mdns_free();
        mdns_initialized_ = false;
        ESP_LOGI(TAG, "mDNS deinitialized");
    }
}

bool Discovery::IsInitialized() const {
    return mdns_initialized_;
}

std::string Discovery::DiscoverOtaServer(int timeout_ms) {
    if (!mdns_initialized_) {
        ESP_LOGW(TAG, "mDNS not initialized");
        return "";
    }

    ESP_LOGI(TAG, "Discovering OTA server via mDNS (_xiaozhi._tcp)");

    mdns_result_t *results = NULL;
    esp_err_t err = mdns_query_ptr("_xiaozhi", "_tcp", timeout_ms, 20, &results);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mDNS query failed: %s", esp_err_to_name(err));
        return "";
    }

    if (!results) {
        ESP_LOGW(TAG, "No Xiaozhi OTA servers found via mDNS");
        return "";
    }

    std::string ota_url;
    mdns_result_t *r = results;

    while (r) {
        ESP_LOGI(TAG, "Found service: %s", r->instance_name ? r->instance_name : "Unknown");

        if (r->hostname && r->port > 0) {
            // Get the first IPv4 address
            mdns_ip_addr_t *addr = r->addr;
            while (addr) {
                if (addr->addr.type == ESP_IPADDR_TYPE_V4) {
                    char ip_str[16];
                    esp_ip4addr_ntoa(&addr->addr.u_addr.ip4, ip_str, sizeof(ip_str));
                    std::ostringstream url_stream;
                    url_stream << "http://" << ip_str << ":" << r->port << "/xiaozhi/ota/";

                    ota_url = url_stream.str();
                    ESP_LOGI(TAG, "Discovered OTA server: %s", ota_url.c_str());
                    break;
                }
                addr = addr->next;
            }

            if (!ota_url.empty()) {
                break;
            }
        }
        r = r->next;
    }

    mdns_query_results_free(results);

    if (ota_url.empty()) {
        ESP_LOGW(TAG, "No valid OTA server URL found");
    }

    return ota_url;
}



