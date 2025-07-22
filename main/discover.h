#ifndef _DISCOVER_H
#define _DISCOVER_H

#include <string>
#include <memory>

/**
 * @brief Discovery class for mDNS-based OTA server discovery
 *
 * This class encapsulates mDNS functionality for discovering Xiaozhi OTA servers
 * on the local network. It provides proper initialization and cleanup through
 * constructor/destructor pattern.
 */
class Discovery {
public:
    /**
     * @brief Factory method to create and initialize a Discovery instance
     *
     * @return std::unique_ptr<Discovery> Initialized Discovery instance, or nullptr if initialization failed
     */
    static std::unique_ptr<Discovery> Init();

    /**
     * @brief Destructor - automatically deinitializes mDNS service
     */
    ~Discovery();

    /**
     * @brief Discover OTA server via mDNS
     *
     * This method searches for a Xiaozhi OTA server on the local network using mDNS.
     * It looks for services with the type "_xiaozhi._tcp" and returns the URL of the first
     * available server found.
     *
     * @param timeout_ms Timeout in milliseconds for the mDNS query (default: 3000ms)
     * @return std::string The discovered OTA server URL, or empty string if not found
     */
    std::string DiscoverOtaServer(int timeout_ms = 3000);

    /**
     * @brief Check if mDNS is properly initialized
     *
     * @return true if mDNS is initialized and ready to use, false otherwise
     */
    bool IsInitialized() const;

private:
    /**
     * @brief Private constructor - use Init() factory method instead
     */
    Discovery();

    /**
     * @brief Initialize mDNS service
     *
     * @return true if initialization was successful, false otherwise
     */
    bool Initialize();

    /**
     * @brief Deinitialize mDNS service
     */
    void Deinitialize();

    bool mdns_initialized_;
};

#endif // _DISCOVER_H