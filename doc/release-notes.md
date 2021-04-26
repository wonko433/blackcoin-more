0.19.2 Release Notes
===============================

Bitcoin Core version 0.19.2 is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-0.19.2/>

This minor release includes various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/bitcoin/bitcoin/issues>

To receive security and update notifications, please subscribe to:

  <https://bitcoincore.org/en/list/announcements/join/>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over `/Applications/Bitcoin-Qt` (on Mac)
or `bitcoind`/`bitcoin-qt` (on Linux).

Upgrading directly from a version of Bitcoin Core that has reached its EOL is
possible, but it might take some time if the datadir needs to be migrated. Old
wallet versions of Bitcoin Core are generally supported.

Compatibility
==============


### Build system
- #15188 Update zmq to 4.3.1 (rex4539)
- #15983 build with -fstack-reuse=none (MarcoFalke)

### Documentation
- #13941 Add PSBT documentation (sipa)
- #14319 Fix PSBT howto and example parameters (priscoan)
- #14944 Update NetBSD build instructions for 8.0 (fanquake)
- #14966 fix testmempoolaccept CLI syntax (1Il1)
- #15012 Fix minor error in doc/psbt.md (bitcoinhodler)
- #15213 Remove errant paste from walletcreatefundedpsbt for nLocktime replaceable (instagibbs)

### GUI
- #14123 Add GUIUtil::bringToFront (promag)
- #14133 Favor macOS show / hide action in dock menu (promag)
- #14383 Clean system tray icon menu for '-disablewallet' mode (hebasto)
- #14597 Cleanup MacDockIconHandler class (hebasto)
- #15085 Fix incorrect application name when passing -regtest (benthecarman)

### RPC and other APIs
- #14941 Make unloadwallet wait for complete wallet unload (promag)
- #14890 Avoid creating non-standard raw transactions (MarcoFalke)

### Test
- #15985 Add test for GCC bug 90348 (sipa)

### Wallet
- #11911 Free BerkeleyEnvironment instances when not in use (ryanofsky)
- #12493 Reopen CDBEnv after encryption instead of shutting down (achow101)
- #14320 Fix duplicate fileid detection (ken2812221)
- #14350 Add WalletLocation class (promag)
- #14552 Detecting duplicate wallet by comparing the db filename (ken2812221)
- #15297 Releases dangling files on BerkeleyEnvironment::Close (promag)
- #16257 Abort when attempting to fund a transaction above -maxtxfee (sjors)
- #16322 Fix -maxtxfee check by moving it to CWallet::CreateTransaction (promag)
=======
as frequently tested on them.

From 0.17.0 onwards, macOS <10.10 is no longer supported. 0.17.0 is
built using Qt 5.9.x, which doesn't support versions of macOS older than
10.10. Additionally, Bitcoin Core does not yet change appearance when
macOS "dark mode" is activated.

Known issues
============

Wallet GUI
----------

For advanced users who have both (1) enabled coin control features, and
(2) are using multiple wallets loaded at the same time: The coin control
input selection dialog can erroneously retain wrong-wallet state when
switching wallets using the dropdown menu. For now, it is recommended
not to use coin control features with multiple wallets loaded.

Notable changes
===============


0.18.x change log
=================

=======
Bitcoin Core is supported and extensively tested on operating systems using
the Linux kernel, macOS 10.10+, and Windows 7 and newer. It is not recommended
to use Bitcoin Core on unsupported systems.

Bitcoin Core should also work on most other Unix-like systems but is not
as frequently tested on them.

From Bitcoin Core 0.17.0 onwards, macOS versions earlier than 10.10 are no
longer supported, as Bitcoin Core is now built using Qt 5.9.x which requires
macOS 10.10+. Additionally, Bitcoin Core does not yet change appearance when
macOS "dark mode" is activated.

In addition to previously supported CPU platforms, this release's pre-compiled
distribution provides binaries for the RISC-V platform.

0.19.2 change log
=================

### Policy
- #19620 Add txids with non-standard inputs to reject filter (sdaftuar)

### Mining
- #17946 Fix GBT: Restore "!segwit" and "csv" to "rules" key (luke-jr)

### RPC and other APIs
- #19836 Properly deserialize txs with witness before signing (MarcoFalke)

### GUI
- #18123 Fix race in WalletModel::pollBalanceChanged (ryanofsky)
- #18160 Avoid Wallet::GetBalance in WalletModel::pollBalanceChanged (promag)
- #19097 Add missing QPainterPath include (achow101)

### Build system
- #18004 don't embed a build-id when building libdmg-hfsplus (fanquake)
- #18425 releases: Update with new Windows code signing certificate (achow101)
- #18676 Check libevent minimum version in configure script (hebasto)
- #19536 qt, build: Fix QFileDialog for static builds (hebasto)
- #20142 build: set minimum required Boost to 1.48.0 (fanquake)

### Tests and QA
- #18001 Updated appveyor job to checkout a specific vcpkg commit ID (sipsorcery)
- #19444 Remove cached directories and associated script blocks from appveyor config (sipsorcery)
- #18640 appveyor: Remove clcache (MarcoFalke)
- #20095 ci: Bump vcpkg commit id to get new msys mirror list (sipsorcery)

### Miscellaneous
- #19612 lint: fix shellcheck URL in CI install (fanquake)
- #18284 scheduler: Workaround negative nsecs bug in boost's `wait_until` (luke-jr)
- #19194 util: Don't reference errno when pthread fails (miztake)

### Documentation
- #19777 Correct description for getblockstats's txs field (shesek)

### Refactoring
- #20141 Avoid the use of abs64 in timedata (sipa)

Credits
=======

Thanks to everyone who directly contributed to this release:

- Aaron Clauson
- Andrew Chow
- fanquake
- Gregory Sanders
- Hennadii Stepanov
- Jonas Schnelli
- João Barbosa
- Luke Dashjr
- MarcoFalke
- MIZUTA Takeshi
- Nadav Ivgi
- Pieter Wuille
- Russell Yanofsky
- Suhas Daftuar
- Wladimir J. van der Laan

As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/bitcoin/bitcoin/).
