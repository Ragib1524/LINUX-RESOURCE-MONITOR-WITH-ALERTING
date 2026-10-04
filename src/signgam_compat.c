/*
 * src/signgam_compat.c
 *
 * Compatibility shim for the debug (AddressSanitizer) build only.
 *
 * libasan intercepts lgamma/lgammaf/lgammal and references the GNU-unique
 * symbol 'signgam' (normally defined in libm). On some toolchains — notably
 * non-root local compiler prefixes where the linker's libm symbol resolution
 * differs — this symbol fails to resolve at link time.
 *
 * We provide a single definition so `make debug` links cleanly. This object
 * is linked ONLY by the 'debug' target (see Makefile) and is never part of
 * the release build. The variable is unused by our code; it exists solely to
 * satisfy the static sanitizer library's relocation.
 */

int signgam = 0;
