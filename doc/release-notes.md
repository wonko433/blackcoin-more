
Bitcoin Core version 0.19.2 is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-0.19.2/>
=======
0.20.1 Release Notes
====================

Bitcoin Core version 0.20.1 is now available from:

  <https://bitcoincore.org/bin/bitcoin-core-0.20.1/>

This minor release includes various bug fixes and performance
improvements, as well as updated translations.

Please report bugs using the issue tracker at GitHub:

  <https://github.com/bitcoin/bitcoin/issues>

To receive security and update notifications, please subscribe to:

  <https://bitcoincore.org/en/list/announcements/join/>

How to Upgrade
==============

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes in some cases), then run the
installer (on Windows) or just copy over `/Applications/Bitcoin-Qt` (on Mac)
or `bitcoind`/`bitcoin-qt` (on Linux).

Upgrading directly from a version of Bitcoin Core that has reached its EOL is
possible, but it might take some time if the data directory needs to be migrated. Old
wallet versions of Bitcoin Core are generally supported.

Compatibility
==============

as frequently tested on them.
=======
Bitcoin Core is supported and extensively tested on operating systems
using the Linux kernel, macOS 10.12+, and Windows 7 and newer.  Bitcoin
Core should also work on most other Unix-like systems but is not as
frequently tested on them.  It is not recommended to use Bitcoin Core on
unsupported systems.

From Bitcoin Core 0.20.0 onwards, macOS versions earlier than 10.12 are no
longer supported. Additionally, Bitcoin Core does not yet change appearance
when macOS "dark mode" is activated.

Known Bugs
==========

The process for generating the source code release ("tarball") has changed in an
effort to make it more complete, however, there are a few regressions in
this release:


Wallet GUI
----------

For advanced users who have both (1) enabled coin control features, and
(2) are using multiple wallets loaded at the same time: The coin control
input selection dialog can erroneously retain wrong-wallet state when
switching wallets using the dropdown menu. For now, it is recommended
not to use coin control features with multiple wallets loaded.
=======
- The generated `configure` script is currently missing, and you will need to
  install autotools and run `./autogen.sh` before you can run
  `./configure`. This is the same as when checking out from git.

- Instead of running `make` simply, you should instead run
  `BITCOIN_GENBUILD_NO_GIT=1 make`.

Notable changes
===============


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
=======
Changes regarding misbehaving peers
-----------------------------------

Peers that misbehave (e.g. send us invalid blocks) are now referred to as
discouraged nodes in log output, as they're not (and weren't) strictly banned:
incoming connections are still allowed from them, but they're preferred for
eviction.

Furthermore, a few additional changes are introduced to how discouraged
addresses are treated:

- Discouraging an address does not time out automatically after 24 hours
  (or the `-bantime` setting). Depending on traffic from other peers,
  discouragement may time out at an indeterminate time.

- Discouragement is not persisted over restarts.

- There is no method to list discouraged addresses. They are not returned by
  the `listbanned` RPC. That RPC also no longer reports the `ban_reason`
  field, as `"manually added"` is the only remaining option.

- Discouragement cannot be removed with the `setban remove` RPC command.
  If you need to remove a discouragement, you can remove all discouragements by
  stop-starting your node.

Notification changes
--------------------

`-walletnotify` notifications are now sent for wallet transactions that are
removed from the mempool because they conflict with a new block. These
notifications were sent previously before the v0.19 release, but had been
broken since that release (bug
[#18325](https://github.com/bitcoin/bitcoin/issues/18325)).

PSBT changes
------------

PSBTs will contain both the non-witness utxo and the witness utxo for segwit
inputs in order to restore compatibility with wallet software that are now
requiring the full previous transaction for segwit inputs. The witness utxo
is still provided to maintain compatibility with software which relied on its
existence to determine whether an input was segwit.

0.20.1 change log
=================

### Mining
- #19019 Fix GBT: Restore "!segwit" and "csv" to "rules" key (luke-jr)

### P2P protocol and network code
- #19219 Replace automatic bans with discouragement filter (sipa)

### Wallet
- #19300 Handle concurrent wallet loading (promag)
- #18982 Minimal fix to restore conflicted transaction notifications (ryanofsky)

### RPC and other APIs
- #19524 Increment input value sum only once per UTXO in decodepsbt (fanquake)
- #19517 psbt: Increment input value sum only once per UTXO in decodepsbt (achow101)
- #19215 psbt: Include and allow both non_witness_utxo and witness_utxo for segwit inputs (achow101)

### GUI
- #19097 Add missing QPainterPath include (achow101)
- #19059 update Qt base translations for macOS release (fanquake)

### Build system
- #19152 improve build OS configure output (skmcontrib)
- #19536 qt, build: Fix QFileDialog for static builds (hebasto)

### Tests and QA
- #19444 Remove cached directories and associated script blocks from appveyor config (sipsorcery)
- #18640 appveyor: Remove clcache (MarcoFalke)

### Miscellaneous
- #19194 util: Don't reference errno when pthread fails (miztake)
- #18700 Fix locking on WSL using flock instead of fcntl (meshcollider)

Credits
=======

Thanks to everyone who directly contributed to this release:

- Aaron Clauson
- Andrew Chow
- fanquake
- Hennadii Stepanov
- João Barbosa
- Luke Dashjr
- MarcoFalke
- MIZUTA Takeshi
- Pieter Wuille
- Russell Yanofsky
- sachinkm77
- Samuel Dobson
- Wladimir J. van der Laan

As well as to everyone that helped with translations on
[Transifex](https://www.transifex.com/bitcoin/bitcoin/).
