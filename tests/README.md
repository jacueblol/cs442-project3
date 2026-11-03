# Provided Tests

The files in this directory are named as follows:

```
input/<INPUT TEST NAME>
output/<INPUT TEST NAME>/<THREADS>[_debug].<OUTPUT TYPE>
```

- `<THREADS>` is the number of threads to be used for compression
- `<INPUT TEST NAME>` is the name of the input test file that the output is based on.
- `<OUTPUT TYPE>` is one of:
  - `expected`: the expected output of your program
  - `actual`: the actual output of your program

For example, if you run your program with these parameters,

```
./pzip ./tests/input/tiny ./tests/output/tiny/2.actual 2
```

The output file `./tests/output/tiny/2.actual` should look like the contents of the following file:

```
./tests/output/tiny/2.expected
```
