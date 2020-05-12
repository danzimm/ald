# ald

Generally this is a WIP. No promises are made about the sanity of result files (for now).

## What & Why

ald is a new static linker for mach-o files. I've wanted to rewrite ld64 for a while, if anything just for fun. Generally I want to speed up the time it takes to statically link an app when debugging. It's been my experience that when a developer is able to iterate quickly in debug / development mode they are able to produce better results (if not just quicker results). It's also always more delightful to work with tools that don't block you from proceeding.

## Goals

Below are some head-in-the-clouds goals:

- Development mode that's super quick
- Support debug information generation as required/supported by lldb (TBD what debug info that will be... `eh_frame`? `compact_unwind`? do we need symbol tables? what's in a DWARF file?)
- Release mode that'll support LTO (full & thin), but not necessarily be super fast
- Support multi-binary results
- Support DCE

## Restrictions

Below are some restrictions (they could also be rephrased as non-goals) of the project:

- Only support `x86_64` & `arm64`
- Only support compressed linkedit mach-o files
- Only support two-level namespace lookups

## Contributions

Want to try and help out? That'd be wicked cool! Right now the project is stupid immature, so it might seem a bit vague where to start. If you have a good idea of what to do then please feel free to make a PR! I do reserve the right to decline a PR, but generally if it seems within the restrictions above and doesn't cause a maintenance burden then the PR probably will be accepted. Feel free to reach out to me directly and we can discuss what sort of roadmap I have in my head (eventually if this project takes off I'll formalize a roadmap and publish it on Github)- you can find me on Twitter: @danzimm.

# How to build

- [Clone LLVM](https://llvm.org/docs/GettingStarted.html)
- Navigate to `$LLVM_REPO_ROOT/llvm/tools`
- Clone this repo: `git clone https://github.com/danzimm/ald.git`
- Build LLVM just like normal (see instructions in first bullet)
- Voila ald has been built in `$LLVM_BUILD/out/bin/ald`
