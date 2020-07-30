macOS Build Instructions and Notes
====================================
The commands in this guide should be executed in a Terminal application.
The built-in one is located in `/Applications/Utilities/Terminal.app`.

Preparation
-----------
Install the macOS command line tools:

`xcode-select --install`

When the popup appears, click `Install`.

Then install [Homebrew](https://brew.sh).

Dependencies
----------------------

    brew install automake berkeley-db6 libtool boost miniupnpc openssl pkg-config protobuf python qt libevent qrencode

See [dependencies.md](dependencies.md) for a complete overview.

If you want to build the disk image with `make deploy` (.dmg / optional), you need RSVG

    brew install librsvg

Berkeley DB
-----------
It is recommended to use Berkeley DB 6.2.

**Note**: You only need Berkeley DB if the wallet is enabled (see the section *Disable-Wallet mode* below).

Build Blackcoin More
------------------------

1. Clone the Blackcoin More source code and cd into `blackcoin-more`

        git clone https://gitlab.com/blackcoin/blackcoin-more/
        cd blackcoin-more

2.  Build Blackcoin More:

    Configure and build the headless Blackcoin More binaries as well as the GUI (if Qt is found).

    You can disable the GUI build by passing `--without-gui` to configure.

        ./autogen.sh
        ./configure
        make

3.  It is recommended to build and run the unit tests:

        make check

4.  You can also create a .dmg that contains the .app bundle (optional):

        make deploy

Running
-------

Blackcoin More is now available at `./src/blackmored`

Before running, it's recommended that you create an RPC configuration file.

    echo -e "rpcuser=blackcoinrpc\nrpcpassword=$(xxd -l 16 -p /dev/urandom)" > "/Users/${USER}/Library/Application Support/Blackmore/blackmore.conf"

    chmod 600 "/Users/${USER}/Library/Application Support/Blackmore/blackmore.conf"

The first time you run blackmored, it will start downloading the blockchain. This process could take several hours.

You can monitor the download process by looking at the debug.log file:

    tail -f $HOME/Library/Application\ Support/Blackmore/debug.log

Other commands:
-------

    ./src/blackmored -daemon # Starts the blackcoin daemon.
    ./src/blackmore-cli --help # Outputs a list of command-line options.
    ./src/blackmore-cli help # Outputs a list of RPC commands when the daemon is running.

Notes
-----

* Tested on OS X 10.10 Yosemite through macOS 10.13 High Sierra on 64-bit Intel processors only.

* Building with downloaded Qt binaries is not officially supported. See the notes in [#7714](https://github.com/bitcoin/bitcoin/issues/7714)
