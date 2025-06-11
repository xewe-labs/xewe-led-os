#ifndef MEMORY_H
#define MEMORY_H

#include <Arduino.h>
#include <Preferences.h> // Use the Preferences library for NVS
#include "../../Debug.h" // Assuming this path is correct for your project

class Memory {
public:
    /**
     * @brief Constructor for the Memory class.
     * @param ns The namespace for this NVS instance. Defaults to "system_storage".
     * It's highly recommended to use unique namespaces for different logical groups of data.
     */
    explicit Memory(const char* ns = "system_storage");

    /**
     * @brief Destructor. Attempts to commit any pending changes before destruction.
     * It's still recommended to call commit() explicitly for critical data.
     */
    ~Memory();

    /**
     * @brief Initializes the Preferences library for this Memory instance.
     * Must be called before any read/write operations.
     * @return True if initialization was successful, false otherwise.
     */
    bool begin();

    /**
     * @brief Closes the Preferences handle, releasing resources and ensuring data is written to flash.
     * Automatically called by commit() and destructor.
     */
    void end();

    /**
     * @brief Commits all pending changes to NVS flash memory.
     * This is crucial for ensuring data persists across reboots.
     * @return True if commit was successful, false otherwise.
     */
    bool commit();

    /**
     * @brief Writes a String value to NVS.
     * @param key The key to store the value under.
     * @param value The String value to write.
     */
    void write_str(const String& key, const String& value);

    /**
     * @brief Reads a String value from NVS.
     * @param key The key to read the value from.
     * @param defaultValue The value to return if the key is not found.
     * @return The read String value, or defaultValue if not found.
     */
    String read_str(const String& key, const String& defaultValue = "");

    /**
     * @brief Writes a boolean value to a specific bit within a byte in NVS.
     * This performs a read-modify-write operation on the byte.
     * @param key The key of the byte to modify.
     * @param bit The bit position (0-7) to set or clear.
     * @param value The boolean value to write (true for 1, false for 0).
     */
    void write_bit(const String& key, uint8_t bit, bool value);

    /**
     * @brief Reads a boolean value from a specific bit within a byte in NVS.
     * @param key The key of the byte to read from.
     * @param bit The bit position (0-7) to read.
     * @return The boolean value of the specified bit.
     */
    bool read_bit(const String& key, uint8_t bit);

    /**
     * @brief Writes an 8-bit unsigned integer to NVS.
     * @param key The key to store the value under.
     * @param value The uint8_t value to write.
     */
    void write_uint8(const String& key, uint8_t value);

    /**
     * @brief Reads an 8-bit unsigned integer from NVS.
     * @param key The key to read the value from.
     * @param defaultValue The value to return if the key is not found.
     * @return The read uint8_t value, or defaultValue if not found.
     */
    uint8_t read_uint8(const String& key, uint8_t defaultValue = 0);

    /**
     * @brief Writes a 16-bit unsigned integer to NVS.
     * @param key The key to store the value under.
     * @param value The uint16_t value to write.
     */
    void write_uint16(const String& key, uint16_t value);

    /**
     * @brief Reads a 16-bit unsigned integer from NVS.
     * @param key The key to read the value from.
     * @param defaultValue The value to return if the key is not found.
     * @return The read uint16_t value, or defaultValue if not found.
     */
    uint16_t read_uint16(const String& key, uint16_t defaultValue = 0);

    /**
     * @brief Clears all keys within this Memory instance's namespace in NVS.
     * A commit() is performed automatically after clearing.
     */
    void reset();

    /**
     * @brief Checks if the Preferences instance is currently initialized and ready for use.
     * @return True if initialized, false otherwise.
     */
    bool is_initialised() const { return initialised_; }

private:
    Preferences preferences_;  // Preferences object to handle NVS
    const char* namespace_;    // The namespace used for this storage instance
    bool initialised_ = false; // To track if begin() has been successfully called
};

#endif // MEMORY_H