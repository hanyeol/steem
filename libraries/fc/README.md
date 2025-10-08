fc - Fast-Compiling C++ Library
================================

FC stands for fast-compiling c++ library and provides a set of utility libraries useful
for the development of asynchronous libraries. Some of the highlights include:

- Cooperative Multi-Tasking Library with support for Futures, mutexes, signals
- Wrapper on Boost ASIO for handling async operations cooperatively with easy to code synchronous style
- Reflection for C++ allowing automatic serialization in JSON & Binary of C++ Structs
- Automatic generation of client/server stubs for reflected interfaces with support for JSON-RPC
- Cryptographic Primitives for a variety of hashes and encryption algorithms
- Logging Infrastructure
- Wraps many Boost APIs, such as boost::filesystem, boost::thread, and boost::exception to accelerate compiles
- Support for unofficial Boost.Process library

## Key Modules

### Cryptography (`crypto/`)
- **Elliptic Curve**: secp256k1 for digital signatures
- **Hashing**: SHA-256, SHA-512, RIPEMD-160, SHA-1
- **Base58/Base64**: Encoding/decoding utilities
- **AES**: Encryption and decryption
- **Key Management**: Public/private key pair generation and handling

### Networking (`network/`)
- **HTTP Client/Server**: RESTful communications
- **WebSocket**: Real-time bidirectional communication
- **TCP/UDP**: Low-level socket operations
- **URL Parsing**: URI utilities
- **SSL/TLS**: Secure connections via OpenSSL

### Serialization (`io/`)
- **Binary Packing**: Efficient serialization for network transmission
- **JSON**: Parsing and generation
- **Variant**: Type-safe discriminated unions
- **Reflection**: Compile-time type introspection using macros

### Asynchronous I/O (`thread/`)
- **Tasks**: Asynchronous task execution
- **Futures/Promises**: Future/promise pattern for async results
- **Thread Pool**: Managed worker threads
- **Mutexes**: Thread synchronization primitives

### Containers
- **Optional**: Similar to std::optional
- **Static Variant**: Compile-time variant type
- **Fixed String**: Stack-allocated strings
- **Safe Math**: Overflow-checked integer operations

### Time Utilities
- **time_point**: Microsecond precision timestamps
- **Duration**: Time intervals
- **Rate Limiting**: Throttling utilities

### Logging
- **Configurable Levels**: Debug, info, warn, error, fatal
- **Multiple Appenders**: Console, file, custom outputs
- **Thread-Safe**: Concurrent logging support
- **Contextual Logging**: File, line, function information

## Reflection System

FC provides a powerful reflection system for automatic serialization:

```cpp
struct person {
   std::string name;
   int age;
   std::vector<std::string> hobbies;
};

FC_REFLECT( person, (name)(age)(hobbies) )
```

This automatically enables:
- Binary serialization/deserialization
- JSON conversion (to_variant/from_variant)
- Type introspection at runtime
- Automatic API generation

## Variant Types

Type-safe discriminated unions used extensively in Steem:

```cpp
typedef fc::static_variant<int, std::string, bool> my_variant;

my_variant v = "hello";
std::string s = v.get<std::string>();

v.visit([](auto& value) {
   // Type-safe visitor pattern
});
```

## Exception Handling

FC provides convenient macros for exception management:

```cpp
FC_ASSERT( condition, "Error message ${detail}", ("detail", value) );
FC_THROW_EXCEPTION( exception_type, "Error occurred" );

try {
   // Code that may throw
} FC_CAPTURE_AND_RETHROW( (context_var1)(context_var2) )
```

## Async I/O Example

```cpp
// HTTP client
fc::http::client client;
auto response = client.get("https://api.example.com/data");

// WebSocket server
fc::http::websocket_server server;
server.on_message([](auto msg) {
   // Handle message
});
server.listen(8090);
```

## Usage in Steem

FC is used throughout the Steem codebase:

- **Protocol**: Serialization, cryptographic signatures, types
- **Chain**: Time utilities, logging, variant operations
- **Network**: P2P communication, HTTP/WebSocket APIs
- **Wallet**: HTTP clients, JSON handling, key management
- **Plugins**: All of the above

### Common Patterns

**Logging:**
```cpp
ilog("Processing block ${num}", ("num", block_num));
wlog("Warning: ${msg}", ("msg", warning_message));
elog("Error occurred: ${e}", ("e", exception.what()));
```

**JSON Conversion:**
```cpp
auto json = fc::json::to_string(my_object);
auto obj = fc::json::from_string(json).as<my_type>();
```

**Async Operations:**
```cpp
fc::async([&]() {
   // Async task
   return result;
}).then([](auto result) {
   // Handle result
});
```

## Building

FC is built as part of Steem:

```bash
cd build
make fc
```

Standalone build:

```bash
cd libraries/fc
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Running Tests

```bash
cd libraries/fc/build
make fc_test
./tests/fc_test
```

## Dependencies

### Required
- **Boost**: System, Thread, Filesystem, Chrono, Test, Context, Coroutine
- **OpenSSL**: For cryptography (libssl, libcrypto)
- **C++14**: Compiler support (GCC 4.8+, Clang 3.3+)

### Vendored Libraries
FC includes vendored dependencies in `vendor/`:
- **websocketpp** - WebSocket library
- **secp256k1-zkp** - Elliptic curve cryptography
- **equihash** - Proof-of-work algorithm (optional)
- **diff-match-patch-cpp-stl** - Text diffing utilities

## Platform Support

- **Linux**: Full support (Ubuntu, Debian, Fedora, etc.)
- **macOS**: Full support (10.11+)
- **Windows**: Partial support (requires additional configuration)

## Development Notes

- Extensive use of templates may increase compile times despite "fast-compiling" name
- Header-only components where possible
- Exception-based error handling throughout
- Thread-safe where documented (check specific module docs)
- Macros used extensively for convenience (FC_ASSERT, FC_REFLECT, etc.)

## Additional Resources

- [Steem Protocol Library](../protocol/) - Uses FC reflection and serialization
- [Steem Chain Library](../chain/) - Uses FC logging and time utilities
- [Boost.ASIO Documentation](https://www.boost.org/doc/libs/release/doc/html/boost_asio.html)
- [WebSocketPP Documentation](https://github.com/zaphoyd/websocketpp)
