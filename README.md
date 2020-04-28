# ald

This is a WIP. The general goal is to enable multiple modes of linking: a quicker one for development and a more robust one which includes standard LTO included in e.g. lld / ld64, but also features like multi-binary output.

# How to build

- [Clone LLVM](https://llvm.org/docs/GettingStarted.html)
- Navigate to `$LLVM_REPO_ROOT/llvm/tools`
- Clone this repo: `git clone https://github.com/danzimm/ald.git`
- Build LLVM just like normal (see instructions in first bullet)
- Voila ald has been built in `$LLVM_BUILD/out/bin/ald`
