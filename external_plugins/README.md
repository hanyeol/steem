# External Plugins Directory

This directory is designed for adding custom plugins that are external to the main Steem plugin tree. It allows developers to extend Steem functionality without modifying the core codebase.

## Purpose

The `external_plugins` directory serves as a integration point for:
- Custom third-party plugins
- Organization-specific extensions
- Experimental plugins not yet ready for the main tree
- Plugins that should be compiled with the node but kept separate from core functionality

## How It Works

The build system automatically discovers and compiles any plugin placed in this directory:

1. Each subdirectory with a `CMakeLists.txt` is treated as a plugin
2. Plugins are compiled **before** the main libraries (required for proper integration)
3. External plugins populate the `STEEM_EXTERNAL_PLUGINS` variable used by the app layer

## Adding an External Plugin

### Method 1: Symbolic Link (Recommended for Development)

Link an example plugin or your custom plugin into this directory:

```bash
cd external_plugins
ln -s ../example_plugins/example_api_plugin example_api_plugin
```

### Method 2: Direct Placement

Copy or create your plugin directly in this directory:

```bash
external_plugins/
├── my_custom_plugin/
│   ├── CMakeLists.txt
│   ├── my_custom_plugin.cpp
│   └── include/
│       └── steem/plugins/my_custom_plugin/
│           └── my_custom_plugin.hpp
└── README.md
```

## Plugin Requirements

Each plugin directory must contain:

1. **CMakeLists.txt** - Build configuration
2. **Source files** - Plugin implementation (.cpp)
3. **Headers** (recommended) - Plugin interface (.hpp)

### Minimal CMakeLists.txt Example

```cmake
file(GLOB HEADERS "include/steem/plugins/my_plugin/*.hpp")

add_library( my_plugin
             ${HEADERS}
             my_plugin.cpp
           )

target_link_libraries( my_plugin appbase steem_chain fc )
target_include_directories( my_plugin
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
```

## Enabling Your Plugin

After building, enable the plugin in your `config.ini`:

```ini
plugin = my_custom_plugin
```

Or via command line:

```bash
./steemd --plugin=my_custom_plugin
```

## Example: Adding the Example API Plugin

To test external plugin functionality with the provided example:

```bash
# Create symbolic link
cd external_plugins
ln -s ../example_plugins/example_api_plugin example_api_plugin

# Rebuild
cd ../build
cmake ..
make

# Configure
echo "plugin = example_api" >> witness_node_data_dir/config.ini

# Run
./programs/steemd/steemd
```

Test the API:

```bash
curl -s --data '{
  "jsonrpc":"2.0",
  "method":"example_api.hello_world",
  "params":{},
  "id":1
}' http://localhost:8090/rpc
```

## Build Order

External plugins are compiled **first** in the build process:

```cmake
# From root CMakeLists.txt:260
# external_plugins needs to be compiled first because libraries/app
# depends on STEEM_EXTERNAL_PLUGINS being fully populated
add_subdirectory( external_plugins )
add_subdirectory( libraries )
add_subdirectory( programs )
```

This ensures that plugin manifests are available when building the main application.

## Best Practices

1. **Follow Plugin Conventions**: Structure your plugin similar to those in `libraries/plugins/`
2. **Namespace Properly**: Use `steem::plugins::your_plugin_name` namespace
3. **Document Your Plugin**: Include a README.md in your plugin directory
4. **Version Control**: Use `.gitignore` for build artifacts but keep source in version control
5. **Dependencies**: Declare all plugin dependencies via `APPBASE_PLUGIN_REQUIRES`
6. **Testing**: Test in a testnet environment before production

## Plugin Development Resources

- [example_plugins/example_api_plugin/](../example_plugins/example_api_plugin/) - Minimal working example
- [docs/plugin.md](../docs/plugin.md) - Plugin system documentation
- [docs/api-notes.md](../docs/api-notes.md) - API configuration and security
- [libraries/plugins/](../libraries/plugins/) - Production plugin examples
- `programs/util/newplugin.py` - Plugin code generator

## Security Considerations

If your plugin exposes an API:

1. **Access Control**: Consider whether the API should be public or require authentication
2. **Rate Limiting**: Implement appropriate limits for expensive operations
3. **Input Validation**: Always validate and sanitize user inputs
4. **Thread Safety**: Use `with_read_lock`/`with_write_lock` for database access

Example API security configuration:

```ini
# Public access
public-api = database_api login_api my_custom_api

# Or require authentication
api-user = {"username":"user", "password_hash_b64":"...", "allowed_apis":["my_custom_api"]}
```

See [doc/api-notes.md](../doc/api-notes.md) for detailed API security setup.

## Troubleshooting

### Plugin Not Loading

- Check that CMakeLists.txt exists in plugin directory
- Verify plugin is enabled in config.ini
- Check logs for initialization errors
- Ensure all dependencies are declared and available

### Build Errors

- Verify all required libraries are linked in CMakeLists.txt
- Check that header paths are correctly specified
- Ensure namespace declarations match directory structure

### Runtime Errors

- Check plugin initialization order and dependencies
- Verify API registration if plugin provides APIs
- Review log output for detailed error messages

## Current Contents

Currently, this directory is empty by default. Plugins are added on demand by developers.

To see what's currently in this directory:

```bash
ls -la external_plugins/
```

## Contributing

When developing external plugins that may be useful to the community:

1. Test thoroughly in testnet
2. Document API methods and configuration
3. Consider contributing to `example_plugins/` if generally useful
4. Share with the community via appropriate channels
