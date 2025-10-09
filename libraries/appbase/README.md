AppBase
=======

The AppBase library provides a basic framework for building applications from
a set of plugins. AppBase manages the plugin life-cycle and ensures that all
plugins are configured, initialized, started, and shutdown in the proper order.

## Key Features

- **Dynamically Specify Plugins to Load** - Runtime plugin selection
- **Automatically Load Dependent Plugins in Order** - Dependency resolution
- **Plugins can specify commandline arguments and configuration file options** - Flexible configuration
- **Program gracefully exits from SIGINT and SIGTERM** - Signal handling
- **Minimal Dependencies** - Only requires Boost 1.58+ and C++14

## Defining a Plugin

A simple example of a 2-plugin application can be found in the /examples directory. Each plugin has
a simple life cycle:

1. Initialize - parse configuration file options
2. Startup - start executing, using configuration file options
3. Shutdown - stop everything and free all resources

All plugins complete the Initialize step before any plugin enters the Startup step. Any dependent plugin specified
by `APPBASE_PLUGIN_REQUIRES` will be Initialized or Started prior to the plugin being Initialized or Started. 

Shutdown is called in the reverse order of Startup. 

```
class net_plugin : public appbase::plugin<net_plugin>
{
   public:
     net_plugin(){};
     ~net_plugin(){};

     APPBASE_PLUGIN_REQUIRES( (chain_plugin) );

     virtual void set_program_options( options_description& cli, options_description& cfg ) override
     {
        cfg.add_options()
              ("listen-endpoint", bpo::value<string>()->default_value( "127.0.0.1:9876" ), "The local IP address and port to listen for incoming connections.")
              ("remote-endpoint", bpo::value< vector<string> >()->composing(), "The IP address and port of a remote peer to sync with.")
              ("public-endpoint", bpo::value<string>()->default_value( "0.0.0.0:9876" ), "The public IP address and port that should be advertized to peers.")
              ;
     }

     void plugin_initialize( const variables_map& options ) { std::cout << "initialize net plugin\n"; }
     void plugin_startup()  { std::cout << "starting net plugin \n"; }
     void plugin_shutdown() { std::cout << "shutdown net plugin \n"; }

};

int main( int argc, char** argv ) {
   try {
      appbase::app().register_plugin<net_plugin>(); // implict registration of chain_plugin dependency
      if( !appbase::app().initialize( argc, argv ) )
         return -1;
      appbase::app().startup();
      appbase::app().exec();
   } catch ( const boost::exception& e ) {
      std::cerr << boost::diagnostic_information(e) << "\n";
   } catch ( const std::exception& e ) {
      std::cerr << e.what() << "\n";
   } catch ( ... ) {
      std::cerr << "unknown exception\n";
   }
   std::cout << "exited cleanly\n";
   return 0;
}
```

This example can be used like follows:

```
./examples/appbase_example --plugin net_plugin
initialize chain plugin
initialize net plugin
starting chain plugin
starting net plugin
^C
shutdown net plugin
shutdown chain plugin
exited cleanly
```

### Boost ASIO 

AppBase maintains a singleton `application` instance which can be accessed via `appbase::app()`.  This 
application owns a `boost::asio::io_service` which starts running when appbase::exec() is called. If 
a plugin needs to perform IO or other asynchronous operations then it should dispatch it via 
`app().get_io_service().post( lambda )`.  

Because the app calls `io_service::run()` from within `application::exec()` all asynchronous operations
posted to the io_service should be run in the same thread.  

## Graceful Exit

To trigger a graceful exit call `appbase::app().quit()` or send SIGTERM or SIGINT to the process.

## Plugin Access

Get references to other plugins:

```cpp
// Get plugin (throws if not found)
auto& plugin = appbase::app().get_plugin<my_plugin>();

// Find plugin (returns nullptr if not found)
auto* plugin = appbase::app().find_plugin<my_plugin>();
```

**Safe to use:**
- In `plugin_initialize()` for required dependencies
- In `plugin_startup()` for any plugin (including optional dependencies)
- In `plugin_shutdown()` for dependencies

## Configuration

### Command-Line Arguments

```bash
./my_app --plugin net_plugin --listen-endpoint 0.0.0.0:9876
```

### Configuration File

Create a `config.ini` file:

```ini
# Plugin to load
plugin = net_plugin

# Plugin options
listen-endpoint = 0.0.0.0:9876
remote-endpoint = seed.example.com:9876
```

### Environment Variables

Set via `set_program_options()`:

```cpp
cfg.add_options()
    ("data-dir", bpo::value<string>()->default_value("data"),
     "Data directory");
```

## Advanced Features

### Optional Dependencies

Plugins can check for optional dependencies in `plugin_startup()`:

```cpp
void plugin_startup() {
    auto* optional = app().find_plugin<optional_plugin>();
    if (optional) {
        // Use optional functionality
    }
}
```

### Signals and Events

Plugins can emit and handle events using Boost.Signals2:

```cpp
// In plugin header
boost::signals2::signal<void(int)> my_signal;

// Connect handler
my_signal.connect([](int value) {
    // Handle event
});

// Emit signal
my_signal(42);
```

## Usage in Steem

AppBase is used throughout Steem:

- **steemd**: Main node application with 20+ plugins
- **cli_wallet**: Wallet application
- **Plugin System**: All Steem plugins inherit from `appbase::plugin`

### Example from Steem

```cpp
// In steemd main.cpp
appbase::app().register_plugin<chain_plugin>();
appbase::app().register_plugin<p2p_plugin>();
appbase::app().register_plugin<webserver_plugin>();

if (!appbase::app().initialize(argc, argv))
    return -1;

appbase::app().startup();
appbase::app().exec();
```

## Building

### Standalone Build

```bash
cd libraries/appbase
mkdir build && cd build
cmake ..
make
```

### As Part of Steem

AppBase is built automatically with Steem:

```bash
cd build
make appbase
```

### Running Examples

```bash
cd libraries/appbase/examples
mkdir build && cd build
cmake ..
make
./appbase_example --plugin net_plugin
```

## API Reference

### Application Class

```cpp
class application {
public:
    // Singleton access
    static application& app();

    // Plugin registration
    template<typename Plugin>
    void register_plugin();

    // Lifecycle
    bool initialize(int argc, char** argv);
    void startup();
    void exec();
    void quit();

    // Plugin access
    template<typename Plugin>
    Plugin& get_plugin();

    template<typename Plugin>
    Plugin* find_plugin();

    // IO service
    boost::asio::io_service& get_io_service();
};
```

### Plugin Base Class

```cpp
template<typename Impl>
class plugin {
public:
    // Plugin name
    static const std::string& name();

    // Configuration
    virtual void set_program_options(
        options_description& cli,
        options_description& cfg
    ) {}

    // Lifecycle
    virtual void plugin_initialize(const variables_map& options) = 0;
    virtual void plugin_startup() = 0;
    virtual void plugin_shutdown() = 0;
};
```

## Dependencies

### Required
1. **C++14 or newer** - clang or g++
2. **Boost 1.60 or newer** - Compiled with C++14 support
   - Boost.ProgramOptions
   - Boost.Filesystem
   - Boost.ASIO
   - Boost.Signals2

### Compiling Boost with C++14

```bash
./b2 cxxflags="-std=c++0x -stdlib=libc++" linkflags="-stdlib=libc++"
```

## Error Handling

### Plugin Not Found

```cpp
try {
    auto& plugin = app().get_plugin<missing_plugin>();
} catch (const std::exception& e) {
    // Plugin not registered or initialized
}
```

### Initialization Failure

```cpp
if (!appbase::app().initialize(argc, argv)) {
    std::cerr << "Failed to initialize\n";
    return -1;
}
```

## Best Practices

1. **Declare Dependencies**: Always use `APPBASE_PLUGIN_REQUIRES` for required plugins
2. **Safe Plugin Access**: Use `find_plugin()` for optional dependencies
3. **Configuration**: Provide sensible defaults in `set_program_options()`
4. **Cleanup**: Release resources in `plugin_shutdown()`
5. **Async Operations**: Use `app().get_io_service().post()` for async work
6. **Error Handling**: Throw exceptions in initialize, handle in startup

## Troubleshooting

### Circular Dependencies

```
Error: Circular dependency detected
```

**Solution**: Redesign plugin dependencies to eliminate cycles. Use optional dependencies or signals for cross-plugin communication.

### Plugin Not Found

```
Error: Plugin 'my_plugin' not found
```

**Solution**:
- Check plugin is registered before `initialize()`
- Verify plugin name matches exactly
- Ensure plugin is enabled in config

### Initialization Order

Plugins initialize in dependency order:
1. Plugins with no dependencies
2. Plugins whose dependencies are initialized
3. Continue until all plugins initialized

## Additional Resources

- [Steem Plugin Documentation](../../docs/plugin.md)
- [Steem Plugin Examples](../../libraries/plugins/)
- [Boost.ProgramOptions](https://www.boost.org/doc/libs/release/doc/html/program_options.html)
- [Boost.ASIO](https://www.boost.org/doc/libs/release/doc/html/boost_asio.html)


