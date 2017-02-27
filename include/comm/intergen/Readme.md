# INTERGEN

This document describes the intergen tool for generating C++/Javascript/Lua APIs from C++ classes. The naming convention used here:

**host class** - any application class, can have one or more interfaces declared

**client interface class** - generated thin wrapper class for a particular interface declaration

An example of how the host class could be decorated for intergen to automatically create C++, JavaScript and Lua client interfaces to the same class:

```c++
    //host class
    class engine
    {
    public:
        //declare an interface, declared name is used as the interface class name
        // the name can optionally contain a namespace under which it should be created
        //the second parameter is a relative path to the generated interface header, with optional interface file name
        ifc_class(ot::engine_interface, ”ifc/”)
    
        //non-static interface methods
        ifc_fn bool create_entity( const char* name, ifc_out uint* eid );
        ifc_fn void delete_entity( uint eid );
    
        //in case you want the interface method to be named differently than the method
        // name in the host class, use ifc_fnx(name) to declare it
        ifc_fnx(something) int do_something();
    
        //an interface creator static method (see below)
        ifc_fn static iref<engine> get( const char* param );
    
        //other normal class content, optionally other interfaces
        // ...
    };
```

Running the intergen tool on the header file containing this class definition generates an interface class containing the decorated methods from the host class.

```c++
    namespace ot {
    
    //interface class
    class engine_interface
    {
    public:
        //a static method to retrieve the interface
        static iref<engine_interface> get( const char* param );
    
        bool create_entity( const char* name, uint* eid );
        void delete_entity( uint eid );
    
        int something();
    };
    } //namespace ot
```

The generated client interface class can be used like this:

```c++
    //usage: create interface object by calling its creator method that internally calls
    // host interface creator method
    iref<engine_interface> ent = engine_interface::get(“blabla”);
    
    //call a method of the interface
    uint eid;
    ent->create_entity(“hoohoo”, &eid);
```
    
Note that one host class can have multiple interfaces defined by `ifc_class` or `ifc_class_var` macros. All decorated methods that follow the interface class macro belong to the interface, up until the end of file or until the next interface class declaration.


### Interface methods

Normal methods callable from the interface can be simply declared by prefixing the method with `ifc_fn` or `ifc_fnx` keyword. Additionally, any out and in-out parameters have to be decorated by `ifc_out` or `ifc_inout` prefix keywords, respectively.

```c++
    //host class
    class engine
    {
    public:
        ifc_class(ot::engine_interface, ”ifc/”)
    
        //an interface creator static method
        ifc_fn static iref<engine> get( const char* param );
    
        //non-static interface methods, out/inout parameters prefixed
        ifc_fn bool create_entity( const char* name, ifc_out uint* eid );
        ifc_fn void delete_entity( uint eid );
    
        //enum type parameters must have explicit enum keyword
        ifc_fn bool something( enum ESomething p );
    };
```    

Interface destructor can be propagated as a special method of the host class, decorated by `ifc_fnx(~)`. This method is not exposed in the interface directly, it gets called only when the interface is released.

```c++
    //host class
    class engine
    {
    public:
        ifc_class(ot::engine_interface, ”ifc/”)

        //a method called when interface client is released
        ifc_fnx(~) void destroy();
```


### Obtaining interfaces to C++ objects

When a client wants to obtain interface to a host object, it needs to call a special static method of the interface named **creator**. Creators are static host class methods that allow clients to connect to a host object. There can be several creators defined on the host class, and what object instance they return depends solely on their implementation. One can have creator that returns a singleton object, and so all clients would retrieve interfaces to the same object. Or a creator can create a new instance of the host class, or fetch an existing one based on optional parameters declared for the creator method.

Creator methods are declared as static `ifc_fn` decorated methods with `iref<host>` return value. No other static interface methods are allowed.

```c++
    class engine
    {
    public:
        ifc_class(ot::engine_interface, ”ifc/”)

        //a static methods returning a reference to the host class are considered
        // to be the accessors used to obtain the host object when interface is requested.
        //a client would call these methods like engine_interface::get() to create
        // the interface, which will invoke these methods

        ifc_fn static iref<engine> get( const char* param ) {
            //just create a new engine instance for the client
            return new engine;
        }
    };
```

Note that if the host class is in a namespace, you have to use a fully qualified name when declaring the return type of interface creator methods.

In this example the client class will be defined in the “ifc/engine_interface.h” header file, generated by intergen. Apart from this file the generator also produces a cpp file with interface dispatcher. If the original header file where the host class was defined was “test.hpp”, then the generated dispatcher file will be named “test.intergen.cpp”.

To obtain the interface from client, include the interface header file and call:

```c++
    iref<ot::engine_interface> ifc = ot::engine_interface::get("foo");
```

For javascript clients there are extra parameters in each creator method added by the intergen, so that one can specify the context where the script runs. The script_handle argument can be set up with a script string, or a *.js script file name, or even a html file or url of page that should be open and that will have bound the reference to the interface to a symbol name specified as the last parameter of the javascript creator function.


### Arbitrary property access

To map property access in Javascript to get/set methods in C++ one can decorate operator() in C++ class with `ifc_fn` to get it function as property getter/setter for Javascript variable bound to an interface instance.

Two operators have to be provided: a setter with key and value-type arguments as a non-const method, and a getter returning the value-type, with one key-type parameter, as a const method.

```c++
    class engine
    {
    public:
        ifc_class(ot::engine_interface, ”ifc/”)
    
        ifc_fn double operator()(const char* key) const;
        ifc_fn void operator()(const char* key, double val);
    };
```

This will allow you to use arbitrary member access in the script, for example:

```c++
    ifc.key
    ifc[“a string key”]
```

Note: the key argument can be anything with implicit conversion from const char* type, or coid::token or coid::charstr types.




### Bidirectional interfaces

With bidirectional interfaces, instances of host class can talk back to the connected clients via events. Events are class methods that are only declared inside the host class definition, and their definition is generated by intergen in the corresponding intergen.cpp file.

Bidirectional interfaces can be declared via `ifc_class_var` macro, which also injects a member variable that will hold a reference to the connected client:

```c++
    //host class
    class engine
    {
    public:
        ifc_class_var(ot::engine_interface, ”ifc/”, var)

        //declaration only, the definition is generated automatically by intergen
        ifc_event void update( float dt );

        //use ifc_eventx(name) to have the interface event name different from
        // the name of the method in the host class
        ifc_eventx(init) void initialize();
    };
```

Host instance can detect whether a client is connected by testing the value of variable declared with `ifc_class_var` clause (var in the above example). 

`ifc_event` or `ifc_eventx` can be then used to decorate a method declaration that can be called from host class and will be invoked in the derived client class. Note the body of the method will be generated by intergen and it therefore must not have definition in the host class itself.

`ifc_volatile` can be used to mark event method parameters to indicate that the memory is temporarily mapped and valid for the duration of the call into clients. Javascript and Lua clients can optimize the way the arguments are passed, without duplicating arrays or strings.

`ifc_evbody` can be used to specify the default implementation of an event, in case the client doesn't implement the event handling.
It can be specified after an ifc_event method declaration:

```c++
    ifc_event bool event() const ifc_evbody(return false);
```


### Interface inheritance

Interface can be declared virtual using `ifc_class_virtual` decoration:

```c++
    ifc_class_virtual(ot::object, "ifc/");
    
    //normal ifc_fn/ifc_event methods
```
A virtual interface doesn't have creators, and can be used to provide an abstract base for other derived interfaces. A derived interface must implement all base interface methods.
To declare a derived interface, use the following syntax:

```c++
    ifc_class_var(ot::derived : ot::base, "ifc/")
    
    //declare all base methods + any extra ones
```


### Asynchronous access, multithreading

WIP:
Client context running in different thread, process or remotely
Methods with no out-type arguments can be invoked asynchronously (blocking/non-blocking versions?)
Methods marked thread-safe can be invoked in non-blocking mode from contexts in separate threads


### Additional constructs recognized by intergen

The intergen parser recognizes the following constructs that help in controlling what should be included in the generated interface header files:

```c++
    //ifc{
      … code that should be shared with the interface class, will be copied into the
        generated header file …
    //}ifc

    /*ifc{
      … code that will be copied into the interface class header file, but is
        inaccessible in the host class …
    }ifc*/

    class engine
    {
        ...
```

To inject a code block specifically into the header class of a particular interface header, use the following construct:

```c++
    //ifc{ ot::engine_interface
      … code that should be shared with the interface class, will be copied into the
        generated header file of ot::engine_interface only …
    //}ifc
```

`ifc_fnx(!)` or `ifc_eventx(!)` can be used to suppress generating the method or the event in the scripting interface.






## Javascript interface

Apart from the C++ interface class, intergen also generates Javascript interface to the host class, which allows to access the host object from Javascript in the same way as from a C++ module.

To bind an interface to a Javascript object, call a creator in the generated JS wrapper object (in ifc/engine_interface.js.h):

```c++
    iref<js::ot::engine_interface> iface = js::ot::engine_interface::get(script_handle(url,true));
```

The script_handle class accepts urls or direct scripts. The url can be either a javascript file (*.js), or a html/htm file that should be opened and where the interface object resides.

Html files can be open with optional attributes encoded as query tokens (after ?, separated by &). The following attributes are recognized:

name            - window name
width           - initial window width
height          - initial window height
x, y            - initial window position
transparent     - 1/true, enable transparency
conceal         - 1/true, hide window from showing on screenshots

Since Javascript doesn’t support out or inout-type parameters, invocation of interface methods is a bit different from C++. In case there are multiple output-type parameters (including the return type if it’s not void), all out and inout values are returned encapsulated in a compound return value object as its members. The original return value is stored in $ret member in that case. If there is just one output value, the return value (or a single output parameter) is returned directly as the return value to Javascript.



### Creating interface objects from Javascript

Interface object can be created directly from the script by calling $query_interface injected Javascript function with the full interface name and creator function path as the first argument, followed by optional creator method arguments:

```Javascript
    //for the ot::engine_interface as defined above, with creator fn get(const char* param)

    var iobj = $query_interface(“ot::engine_interface.get”, “...”)
```

Sometimes it’s not desirable to expose all interface creator methods to Javascript, as they may be internal, requiring custom parameters that only make sense (or are available) on the C++ side. In that case, if the creator method begins with underscore character, it’s not visible to Javascript’s $query_interface call:

```c++
    class engine
    {
    public:
        ifc_class(ot::engine_interface, ”ifc/”)
    
        //a creator method not visible from Javascript
        ifc_fn static iref<engine> _get( const char* param );
    
        //a hidden creator can be renamed so that the underscore doesn’t appear in clients
        ifc_fnx(create) static iref<engine> _create();
    };
```


## Using Intergen with Visual Studio

- Right click on the project -> build customizations, add intergen props file (comm/intergen/intergen.props) and enable it
- Add a header file file into the project (can use hpp or hxx to differentiate from other headers), and click properties, and set the item type to intergen interface generator
- From that moment whenever the header file is modified, intergen runs and produces interface files, if it finds any interface declarations within the header
- Add the intergen generated *.intergen.cpp files into the project. If you add the *.js.intergen.cpp" file as well for Javascript interface support, you'll need to include v8.lib into link dependencies (or any other lib/dll that includes it)
