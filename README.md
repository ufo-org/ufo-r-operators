# UFO R Operators

This package contains a collection of operator overloads for use with UFO vectors: [R
API](https://github.com/ufo-org/ufo-r) of the [UFO
native vectors](https://github.com/ufo-org/ufo-r-vectors), and [UFO sandboxed R vectors](https://github.com/ufo-org/ufo-r-sandbox).

The package provides operators and helper functions for UFO
vectors: 

 * unary operators: `-`, `+`, `!`
 * binary arithmetic operators: `*`, `+`, `-`, `%%`, `/`, `%/%`
 * comparison operators: `<`, `<=`, `>`, `>=`, `>`, `>=`, `==`, `!=`, `|`, `&`
 * subsetting operators: `[`, `[<-`
 * subscript derivation `ufo_subscript`
 * in-place mutation: `ufo_mutate`

**Warning:** UFOs are under active development. Some bugs are to be expected,
and some features are not yet fully implemented. 

## System requirements

- R 3.6.0 or higher
- Linux 4.3 or higher (we currently do not support other operating systems)

## Prerequisites

Check if your operating system restricts who can call `userfaultfd`:

```bash
cat /proc/sys/vm/unprivileged_userfaultfd
```

*0* means only privileged users can call `userfaultfd` and UFOs will only work
for privileged users. To allow unprivileged users to call `userfaultfd`:

```bash
sysctl -w vm.unprivileged_userfaultfd=1
```

## Building

Before building, retrievew the code of a submodule:

```bash
git submodule update --init --recursive
```

To update the submodule, pull it.

```bash
cd include/ufo_r && git pull origin main && cd ../..
```

Install the package with R. This compiles and properly install the package.

```bash
R CMD INSTALL --preclean .
```

You can also build the project with debug symbols by setting (exporting) the
`UFO_DEBUG` environmental variable to `1`.

```bash
UFO_DEBUG=1 R CMD INSTALL --preclean .
```

## Testing

Check the package and display all errors:

```bash
_R_CHECK_TESTS_NLINES_=0 R CMD check .
```

## Troubleshooting

### Operation not permitted

```
syscall/userfaultfd: Operation not permitted
error initializing User-Fault file descriptor: Invalid argument
Error: package or namespace load failed for ‘ufos’:
 .onLoad failed in loadNamespace() for 'ufos', details:
  call: .initialize()
  error: Error initializing the UFO framework (-1)
```

The user has insufficient privileges to execute a userfaultfd system call. 

One likely culprit is that a global `sysctl` knob `vm.unprivileged_userfaultfd` to
control whether userfaultfd is allowed by unprivileged users was added to kernel
settings. If `/proc/sys/vm/unprivileged_userfaultfd` is `0`, do:

```bash
sysctl -w vm.unprivileged_userfaultfd=1
```