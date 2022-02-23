# simpleIRCd
simpleIRCd is a simple, fast, and cross-platform IRC daemon that implements most of [RFC 1459](https://datatracker.ietf.org/doc/html/rfc1459). It's extremely simple to build, having no dependencies, and runs on both Windows and Linux. Built as part of my A-level coursework many years ago, it certainly has issues (excessive commenting to follow the mark scheme being but one), but works reasonably well, most of the time.

## Installation
On either a Linux or Windows system with `make` and a C compiler, build the binary with:
```bash
git clone https://github.com/ewnh/simpleIRCd
cd simpleIRCd
make
```
And optionally on Linux, to install the daemon on your PATH:
```bash
make install
```

## Usage
Run the binary with no arguments:
```bash
simpleIRCd
```
Or, you might want to run the server as a daemon instead (output is still sent to stdout, so you might want to use a new terminal anyway):
```bash
simpleIRCd &
```
This starts the server listening on port 6667 for new IRC connections. From here, you should be able to point any IRC client at the address and connect. Commands are listed on the [RFC](https://datatracker.ietf.org/doc/html/rfc1459#section-4).

Unfortunately, most modern IRC clients don't seem to completely follow the RFC, so you may encounter difficulties. The server was tested with HexChat and should almost certainly work with simple clients (e.g. terminal applications), but with anything more complex YMMV.
