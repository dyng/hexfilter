# Hexfilter

Read/Write non-printable characters in a printable way.

## Synopsis

    hexfilter COMMAND [ARG]...

`hexfilter` was originally developed for cracking. It's not rare when you need to input raw hex values in cracking something, usually it is done by `echo` or `printf`. Use `echo` or `printf` to generate the hex string and redirect it to a temp file, then `cat` the file as stdin. However, this way does not work if you need to input interactively. Yes, `hexfilter` is exactly for this.

`hexfilter` takes over subcommand's *stdin*, *stdout* and *stderr*, parses non-printal characters in *stdout*/*stderr* to '\xXX' format and prints it to terminal. Also it accepts the same '\xXX' format for inputing hex characters, converts and passes it to subcommand.

## Compile

`hexfilter` depends on C POSIX library only as you can compile it on UNIX-like system by

    gcc -o hexfilter hexfilter.c

Or if you prefer `clang`, nothing will change

    clang -o hexfilter hexfilter.c

If a Makefile turns out to be necessary, I'll write it, but now I want to keep everything in a simplest way.

## Example

#### Read

    $ echo -ne 'ab\xff\xfe' >/tmp/input
    $ cat /tmp/input
    ab??%
    $ ./hexfilter cat /tmp/input
    ab\xff\xfe%

#### Write

    $ ./hexfilter tee /tmp/output
    ab\xff\xfe
    ab\xff\xfe
    ^C
    $ hexdump /tmp/output
    0000000 61 62 ff fe 0a
    0000005

## TODO

1. End stdin when `CTRL-D` is pressed. (The conventional way to end input but not supported yet)
2. Handle redirection and pipe for subcommand.

I think it is an useful tool but I don't have much time to improve it. Your contribution is really welcome!
