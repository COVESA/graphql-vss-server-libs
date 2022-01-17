# GraphQL VSS Server Libraries

Main libraries of the VSS GraphQL C++ Server.

## Introduction

This repository contains libraries to implement a GraphQL Server along with [Microsoft's GraphQL service libraries](https://github.com/microsoft/cppgraphqlgen).

## Table of Contents

* [GraphQL VSS Server Libraries](#graphql-vss-server-libraries)
   * [Introduction](#introduction)
      * [The Protocol Library](#the-protocol-library)
         * [Connection handling](#connection-handling)
         * [Authorization process](#authorization-process)
         * [Request handling](#request-handling)
      * [The Support Library](#the-support-library)
         * [Singletons](#singletons)
         * [CommonAPI Singletons](#commonapi-singletons)
         * [Debug Messages](#debug-messages)
         * [DLT logs](#dlt-logs)
         * [Range Validation](#range-validation)
         * [Permissions](#permissions)
         * [Spinlock](#spinlock)
         * [Demangle](#demangle)
         * [Type Traits Extras](#type-traits-extras)
   * [Dependencies](#dependencies)
   * [Build and install](#build-and-install)
      * [CommonAPI Version](#commonapi-version)
      * [Debug builds](#debug-builds)
      * [Build command](#build-command)
   * [Usage](#usage)

### The Protocol Library

This library supports the creation and management of requests. It handles the HTTP and WebSocket connections, validations, and starts the data resolution process. The flow from starting the server until resolving a query, is:

1. Create HTTP and WebSockets endpoints;
2. Start listening for requests;
3. On HTTP or WebSocket message:
4. Extract the authorization and the query from the message;
5. Validate the authorization;
6. Parse the query to an [ast](https://en.wikipedia.org/wiki/Abstract_syntax_tree);
7. Start the resolution of the query.
8. After the resolution finishes, build a JSON with the response;
9. Send the result back to the client;
10. Clean operation data (if the query or if a subscription ends).

#### Connection handling

This library implements two communication protocols: HTTP and WebSocket. Both regular HTTP requests and WebSocket connections listen on the same port.

* HTTP message: Can be used to do queries and mutations. An HTTP message comes with an *authorization* (a *JWT token* in the header) and the *payload* (in the body). The connection handler uses a JSON parser to separate them.

* Websocket message: In addition to queries and mutations, it is used for subscriptions, since it is full-duplex and enables streams of messages. The payload contains the request. The WebSocket messages come with the authorization and the request in the payload. A helper function extracts the request and the authorization from the payload.

If one tries to do a subscription query over HTTP, the return will be the result of a simple query without executing a successful subscription.

#### Authorization process

A [*JWT token*](https://jwt.io/) contains the permissions of the requesting client. The library checks the authenticity of the token using the [RSA 256 algorithm](https://en.wikipedia.org/wiki/RSA_(cryptosystem)) (public-/private key). The library checks the token against the public key that the server holds. The private key must be held by an authentication service of your trust, that signs the token. For development and testing purposes, you can use the ssh-keygen to generate public and private keys:

```bash
echo "" | ssh-keygen -t rsa -b 4096 -m PEM -f jwtRS256.key -N ""
```

After decoding and validating the token, the *authorization* object stores the permissions in the *client permissions* object. The *client permissions* object is held by the *request state* object.

The *request state* object is a parameter of all resolver functions. It contains the function that validates the request according to the permission supplied by the client. The parameter of the *validate function* is the name or number of the permission to resolve that node of the graph. The *validate* function will call another function that looks into the set of permissions that the client has for the permission required by that node.

The permissions may be integers `uint16` or strings. If you use strings as permission, the JWT token can become too big, therefore we recommend using integers to specify permissions and map to the names of the strings.

This library also provides a *dummy authorizer* for testing the GraphQL server without any authorization. To use it, one must pass the parameter `-D DISABLE_PERMISSION=ON` to CMake.

#### Request handling

The *connection handler*, with the payload containing the parsed GraphQL request, creates a *connection operation* object and stores it in a map until the end of the connection (i.e. end of the subscription or, reply to a query or mutation).

A method on *connection operation* object, called by the *connection handler* object that created it, starts the process of resolving the request.

The function that starts the resolution of the request is set to run on a new thread. The function first parses the query to an ast for the **graphqlservice** (supplied by *cppgraphqlgen*, see the dependencies). Then, the ast is used by a function of the graphqlservice that transverses the graph according to the query, calling the resolver functions. This resolver function is a method of the executable schema object, of the *request class*. The *request class* is provided by the *graphqlservice* of the *cppgraphqlgen*.

The subscription requests are handled by a slightly different object of a *connection operation* class. It also needs to implement a *notify* function to deliver the response when it gets one. Instead of just dispatching the response immediately after a response is ready, like in a query, the connection operation implements a function that limits the *delivery interval* between responses. This time between deliveries is defined by the user and is a parameter of the subscription query. The objective of this *delivery interval* is to avoid overloading the network when an immediate response is not needed. If the user needs prompt responses, the delivery interval should be zero.

### The Support Library

This library supplies several classes that supports the resolver functions of the GraphQL Server. Among others, it supplies support for logging, permission validation, [custom scalars](https://www.apollographql.com/docs/apollo-server/schema/custom-scalars/), singletons and classes that handle CommonAPI calls.

#### Singletons

The library supplies a singleton implementation to help with the creation and management of single instance objects. Such objects may hold things like caches or connections. To obtain a singleton, the programmer should use the `getSingleton` function, passing the name of the singleton as a template parameter.

#### CommonAPI Singletons

Is a use case of singletons (consuming the `singleton.hpp`) supplied here. The process of requesting data with CommonAPI and SOME/IP is:
- create a SOME/IP proxy;
- request an attribute with the proxy;
- get the data from the attribute.

[More details about CommonAPI here](https://at.projects.genivi.org/wiki/pages/viewpage.action?pageId=5472316).

Since a proxy can deliver multiple attributes, a singleton object is created to hold a proxy object. Then, if different resolver functions need attributes fetched with the same proxy, only one instance of the proxy is created.

The attribute is an object with a getter function that gives the desired value. There are cases where the value given by the attribute, is another object with multiple data. Each data could be requested by a different resolver function of the graph.

To avoid over fetching an attribute, the attribute is also held by a singleton object. Then, when two or more points in a query request values from the same attribute, they will share it.

These singletons should be declared with the help of macros defined at `commonapi-singletons.hpp`. To obtain a singleton of a CommonAPI call, the programmer must call the `getSingleton` method of the *request state* object. The name of the proxy followed by the name of the attribute must be passed as template parameter, concatenated by two underscores.

The developer can use the provided macros to name a proxy.

Ex. If the proxy is called `some_proxy` and the attribute is called `SomeAttribute`, the singleton would be obtained with:

```cpp
auto mySingleton = state->getSingleton<some_proxy__SomeAttribute>
```

#### Debug Messages

The `debug.hpp` header provides macros for creating debug messages that will appear on the terminal. A set of macros is defined to set letter color and background color in the messages that will be displayed. As the name infers, this is just compiled in debug builds.

#### DLT logs

This library supports dlt logging. It already declares some contexts as you can see in `log.hpp`, but you can declare more. For more information of how to use DLT, refer to the [DLT documentation](https://github.com/COVESA/dlt-daemon).

**Note:** The `DLT_REGISTER_APP`, `DLT_REGISTER_CONTEXT`, `DLT_UNREGISTER_CONTEXT` and `DLT_UNREGISTER_APP` must declare the app and the contexts in your main function.

#### Range Validation

This library supports range validation directives. If an attribute in the schema is a number that can be mutated, then the developer is able to specify a *range directive* in the schema. For example, if an attribute is a float limited between 0 and 100, then one should use `@range(min: 0.0, max: 100.0)`. If a mutation is sent, which is outside of the specified range, an error is returned to the client.

#### Permissions

This supports the *request state* class. It provides the function, which validates the given permissions of the client.

#### Spinlock

Provides a spinlock to prevent data races. As explained, the *connection object* creates a thread for each query that it receives. While a thread is using a memory location shared with other threads, it holds the spinlock. Other threads cannot use that memory until the spinlock is available again.

The singletons implementations use the spinlock, since they are shared between the threads. The developer can use the spinlock implementation in other parts of the code if he or she wishes.

#### Demangle

Provides the *demangle function* that creates a string with the name of the object based on the name of the class and the templates used to create it. It is very useful for debugging.

#### Type Traits Extras

Provides extra type traits to get the types of the arguments and of the return of a function. It is consumed by the commonapi-singletons implementation.
## Dependencies

For development purposes, we recommend installing the libraries compiled from source outside the system's root, for example, under any directory of your `$HOME`. Define a variable to hold your path,

```bash
# Define the path to where you want to install things. Ex:
MY_PREFIX=$HOME/usr
```

and always pass the following parameters to CMake:

```bash
-DCMAKE_PREFIX_PATH=$MY_PREFIX -DCMAKE_INSTALL_PREFIX=$MY_PREFIX -DPKG_CONFIG_USE_CMAKE_PREFIX_PATH=TRUE
```

This is, of course, optional, but will allow you to use different versions of the dependencies and avoids messing up your system.

### The dependencies and range of compatible versions are given below.

* clang => 9.0
* CMake => 3.13
* dlt libraries 2.18;
* boost [1.66, 1.74];
* [WebSocket++](https://github.com/zaphoyd/websocketpp/) 0.8.2;
* [cppgraphqlgen](https://github.com/microsoft/cppgraphqlgen) 3.5.0;
* [JWT++](https://github.com/Thalhammer/jwt-cpp) => 0.5.0;
* PEGTL 3.2 (clone cppgraphqlgen and do `git submodule update --init --recursive` to build along with cppgraphqlgen);
* CommonAPI C++ Core Runtime[3.1, 3.2] (other versions => 2.7 may work but were not tested).
* CommonAPI C++ SOME/IP Runtime [3.1, 3.2] ( other versions => 2.7 may work but were not tested).

## Build and install

### CommonAPI Version

You can specify the CommonAPI version with which you want to build with the flag `-DCOMMONAPI_RELEASE` . If not specified, it defaults to version 3.2.

### Debug builds

You can build a debug version with `-DCMAKE_BUILD_TYPE=Debug`. The `CMakeLists.txt` also provides options for address and thread sanitizers.

### Build command

If you installed any dependency compiled from source outside your system's directories as we recommended above, you must specify your installation path again. For example, if you installed your dependencies on your `~/usr` and you wish also to install the **GraphQL VSS Server Libraries** in this location, specify the same path in the `CMAKE_INSTALL_PREFIX` and `CMAKE_PREFIX_PATH` again:

```bash
MY_PREFIX=$HOME/usr

cd graphql-vss-server-libs mkdir build && cd build

cmake -G Ninja -DCMAKE_PREFIX_PATH="$MY_PREFIX" -DCMAKE_INSTALL_PREFIX="$MY_PREFIX" \
  -DCMAKE_CXX_COMPILER=$(which clang++) \
  -DCMAKE_STATIC_LINKER_FLAGS="" \
  -DCMAKE_SHARED_LINKER_FLAGS="-fuse-ld=lld -L/usr/local/lib -L$MY_PREFIX/lib" \
  -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=lld -L/usr/local/lib" \
  -DPKG_CONFIG_USE_CMAKE_PREFIX_PATH=ON -S .. -B .
```

Later, when compiling the server with CMake, do not forget to pass the same value of `$MY_PREFIX` on the `CMAKE_PREFIX_PATH` parameter.

## Usage

We supply an example implementation of these libraries at COVESA's repository [GraphQL VSS Data Server](https://github.com/COVESA/graphql-vss-data-server). To run this example, you must also have the [Test SOME/IP Service](https://github.com/COVESA/test-someip-service), from where the server will get mocked data.

As the example shows, the developer creating a C++ GraphQL Server with these libraries must provide:

* **The resolver functions and the schema types in C++**, which can be easily generated with the *schemagen* provided by [cppgraphqlgen](https://github.com/microsoft/cppgraphqlgen) and GraphQL Schema to C++ Code Generator (yet to be supplied
TODO: put the link to it here when ready on upstream). The GraphQL VSS Data Server provides these files already generated;
* **The implementation library**, with the declaration of the proxies and attributes (using the macros provided at [commonapi-singletons.hpp](graphql_vss_server_libs/support/commonapi-singletons.hpp)), and also supplying any code that could not be generated, for example, functions that post-process the data obtained from SOME/IP;
* **The main function** that creates and sets the server instance (instance of `GraphQLServer` class) itself and register the DLT loggers. See the [cpp-server/src/server.cpp](https://github.com/COVESA/graphql-vss-data-server/blob/master/cpp-server/src/server.cpp) file.

More examples and explanations are provided at the README.md of *GraphQL VSS Data Server*.