# GnuTLS -- ABI information dump

This GnuTLS sub-project tracks the ABI information of GnuTLS
libraries.  The .abi files are generated using the `abidw` command and
compared against the libraries with `abicheck`.  Both commands are
available in the [libabigail] project.

# How GnuTLS uses this information

The main GnuTLS repository embeds this project as a git submodule,
pointing to a specific revision.  The GnuTLS build script will call
`abicheck` to ensure the the currently built libraries do not
introduce any ABI change.  If the ABI change is intentional, the .abi
files in this project should be regenerated and updated, by running
`make files-update`.

# `git-abidiff-gnutls` script

This is a wrapper around `abicheck` which can be used as a [git diff
driver].  To enable it, follow the instruction written in the script.

[libabigail]: https://sourceware.org/libabigail/
[git diff driver]: https://git-scm.com/docs/gitattributes#_defining_an_external_diff_driver
