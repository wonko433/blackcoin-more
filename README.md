Blackcoin More
=====================================

https://blackcoinmore.org

What is Blackcoin?
----------------

Blackcoin is a decentralised digital currency with near-instant transaction speeds and negligible transaction fees built upon Proof of Stake 3.0 (PoSV3, BPoS) as introduced by the Blackcoin development team.

What is Blackcoin More?
----------------

Blackcoin More is the name of open source software which enables use of the Blackcoin protocol. 
By building upon Bitcoin Core 0.20.2, Blackcoin More is an alternate approach to the original client that provides wider compatibility with third party services and a more advanced base.

For more information, as well as an immediately useable, binary version of
the Blackcoin More software, see https://blackcoinmore.org.

License
-------

Blackcoin More is released under the terms of the MIT license. See [COPYING](COPYING) for more
information or see https://opensource.org/licenses/MIT.

Development Process
-------------------

The `master` branch is regularly built and tested, but is not guaranteed to be
completely stable. [Tags](https://gitlab.com/blackcoin/blackcoin-more/tags) are created
regularly to indicate new official, stable release versions of Blackcoin More.

Change log can be found in [CHANGELOG.md](CHANGELOG.md).

The contribution workflow is described in [CONTRIBUTING.md](CONTRIBUTING.md)
and useful hints for developers can be found in [doc/developer-notes.md](doc/developer-notes.md).

The best place to get started is to join the Development channel on Gitter: https://gitter.im/Blackcoin_Hub/Development

Testing
-------

Testing and code review is the bottleneck for development; we get more pull
requests than we can review and test on short notice. Please be patient and help out by testing
other people's pull requests, and remember this is a security-critical project where any mistake might cost people
lots of money.

### Automated Testing

Developers are strongly encouraged to write [unit tests](src/test/README.md) for new code, and to
submit new unit tests for old code. Unit tests can be compiled and run
(assuming they weren't disabled in configure) with: `make check`. Further details on running
and extending unit tests can be found in [/src/test/README.md](/src/test/README.md).

There are also [regression and integration tests](/test), written
in Python, that are run automatically on the build server.
These tests can be run (if the [test dependencies](/test) are installed) with: `test/functional/test_runner.py`

The Travis CI system makes sure that every pull request is built for Windows, Linux, and macOS, and that unit/sanity tests are run automatically.

### Manual Quality Assurance (QA) Testing

Changes should be tested by somebody other than the developer who wrote the
code. This is especially important for large or high-risk changes. It is useful
to add a test plan to the pull request description if testing the changes is
not straightforward.

Translations
------------

Changes to translations as well as new translations can be submitted to
[Bitcoin Core's Transifex page](https://www.transifex.com/bitcoin/bitcoin/).

Translations are periodically pulled from Transifex and merged into the git repository. See the
[translation process](doc/translation_process.md) for details on how this works.

**Important**: We do not accept translation changes as GitHub pull requests because the next
pull from Transifex would automatically overwrite them again.

Translators should also subscribe to the [mailing list](https://groups.google.com/forum/#!forum/bitcoin-translators).
