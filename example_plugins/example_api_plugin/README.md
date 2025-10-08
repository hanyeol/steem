# Example API Plugin

This is a minimal working example demonstrating how to create a custom API plugin for Steem. It serves as a reference implementation for developers building their own plugins.

## Overview

The `example_api_plugin` demonstrates:
- Basic plugin structure and lifecycle
- JSON-RPC API registration
- Simple API method implementations
- Proper reflection of argument and return types

## API Methods

This plugin exposes two simple API methods:

### `hello_world`
Returns a static greeting message.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "example_api.hello_world",
  "params": {},
  "id": 1
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "result": {
    "message": "Hello World"
  },
  "id": 1
}
```

### `echo`
Echoes back the input string.

**Request:**
```json
{
  "jsonrpc": "2.0",
  "method": "example_api.echo",
  "params": {
    "call": "test message"
  },
  "id": 1
}
```

**Response:**
```json
{
  "jsonrpc": "2.0",
  "result": {
    "response": "test message"
  },
  "id": 1
}
```

## Code Structure

### Plugin Class Definition

```cpp
class example_api_plugin : public appbase::plugin< example_api_plugin >
{
   // Plugin requirements
   APPBASE_PLUGIN_REQUIRES( (plugins::json_rpc::json_rpc_plugin) );

   // Lifecycle methods
   virtual void plugin_initialize( const variables_map& options ) override;
   virtual void plugin_startup() override;
   virtual void plugin_shutdown() override;

   // API methods
   hello_world_return hello_world( const hello_world_args& args );
   echo_return echo( const echo_args& args );
};
```

### Key Components

1. **Dependencies**: Requires `json_rpc_plugin` (declared via `APPBASE_PLUGIN_REQUIRES`)

2. **Argument/Return Types**:
   - `hello_world_args` - Uses `json_rpc::void_type` (no arguments)
   - `hello_world_return` - Contains a `message` string
   - `echo_args` - Contains a `call` string
   - `echo_return` - Contains a `response` string

3. **API Registration**: Done in `plugin_initialize()` via `JSON_RPC_REGISTER_API`

4. **Reflection**: All custom types must be reflected using `FC_REFLECT` macro

## Building

This plugin is located in `example_plugins/` directory, separate from the main plugin tree. To include it in your build:

1. Add the plugin to your external plugins configuration
2. Link against required libraries: `appbase`, `steem_chain`, `fc`
3. Build normally with CMake

## Using This Plugin

### Enable in Configuration

Add to your `config.ini`:
```ini
plugin = example_api
```

### Testing via CLI

Using `curl`:
```bash
curl -s --data '{
  "jsonrpc":"2.0",
  "method":"example_api.hello_world",
  "params":{},
  "id":1
}' http://localhost:8090/rpc
```

```bash
curl -s --data '{
  "jsonrpc":"2.0",
  "method":"example_api.echo",
  "params":{"call":"Hello from CLI"},
  "id":1
}' http://localhost:8090/rpc
```

## Creating Your Own Plugin

Use this example as a template:

### 1. Define Your Plugin Structure

```cpp
class my_api_plugin : public appbase::plugin< my_api_plugin >
{
   APPBASE_PLUGIN_REQUIRES( (plugins::json_rpc::json_rpc_plugin) );

   static const std::string& name() {
      static std::string name = "my_api";
      return name;
   }

   virtual void plugin_initialize( const variables_map& options ) override;
   virtual void plugin_startup() override;
   virtual void plugin_shutdown() override;
};
```

### 2. Define API Types

```cpp
struct my_request_args
{
   string param1;
   int param2;
};

struct my_response
{
   string result;
};
```

### 3. Register Your API

```cpp
void my_api_plugin::plugin_initialize( const variables_map& options )
{
   JSON_RPC_REGISTER_API( name(), (method1)(method2) );
}
```

### 4. Reflect Your Types

```cpp
FC_REFLECT( my_api_plugin::my_request_args, (param1)(param2) )
FC_REFLECT( my_api_plugin::my_response, (result) )
```

## Best Practices

1. **Separate API and State**: Consider separating stateful plugins from API plugins. This allows APIs to be disabled while maintaining state tracking.

2. **Thread Safety**: API calls are dispatched asynchronously. Use `with_read_lock` or `with_write_lock` when accessing database:
   ```cpp
   return database().with_read_lock( [&]() {
      // Your database access code
   });
   ```

3. **Performance**: Design API calls to complete in <250ms (read locks expire after 1 second)

4. **Dependencies**: Declare all plugin dependencies using `APPBASE_PLUGIN_REQUIRES`

5. **Configuration**: Add config options via `set_program_options()` if needed

## Code Generation

For more complex plugins, use the plugin generator:
```bash
python3 programs/util/newplugin.py
```

This generates boilerplate code for new plugins with proper structure.

## Related Documentation

- [doc/plugin.md](../../doc/plugin.md) - Comprehensive plugin system documentation
- [libraries/plugins/](../../libraries/plugins/) - Production plugin implementations
- `programs/util/newplugin.py` - Plugin code generator

## Notes

- This is a minimal example - production plugins typically include database objects, signal handlers, and more complex business logic
- The plugin name (`STEEM_EXAMPLE_API_PLUGIN_NAME`) must be unique across all plugins
- Always test plugins in a testnet environment before production deployment
