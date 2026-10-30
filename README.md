# Phos

**Phos** is a CMS for Linux written entirely from scratch. With it's own
TCP event-loop based server and HTTP level.

## Architecture

Phos consists of three independent layers:
- **tcp** (`tcp/`) - a TCP server in ANSI C, built as the static library
  `libphostcp.a`. It knows nothing about HTTP:
- **http** (`http/`) - an ANSI C layer on top of tcp: HTTP/1.1 request parsing,
  building responses, routing.
- **CMS** - the content management system itself, on top of the HTTP layer.

## Requirements

* linux as the running operating system
* gcc & g++ or any other C & C++ compatible compilers
* make

## Building and running

```sh
make            # build everything
make run        # build and run the echo server
make clean      # remove build/
```

| Target | Output |
|--------|--------|
| `make lib` | `build/libphostcp.a` - phostcp static library |
| `make server` | `build/echo-server` - echo server |
| `make examples` | `build/examples/echo`, `build/examples/ping` - test clients |
<!-- | `make http` | `build/phos-http` - HTTP layer | -->

By default the server listens on `127.0.0.1:8080`. The address, port and other
parameters are set with macros at compile time, f.e.:

```sh
make clean && make DEFS=-DSTCP_SERVER_PORT=9000
```

You can check that the server works with the test clients:

```sh
./build/examples/echo
./build/examples/ping
```

or with `nc`:

```sh
printf 'hello' | nc 127.0.0.1 8080
```

## License

Phos is distributed under the MIT License, see [LICENSE](LICENSE).

