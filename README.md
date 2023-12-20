# Obfuscator
A native code obfuscator written in c++23.

## Available transforms:
* Bogus Control Flow
* Constant Crypt
* Decompilation breaker (ida, ghidra)
* Substitution

___

## Usage
```commandline
12:11:30.151 | [  info  ] Available options:
12:11:30.151 | |        [  info  ] -h, --help                   -- This message
12:11:30.151 | |        [  info  ] -pdb         [path]          -- Set custom .pdb file location
12:11:30.151 | |        [  info  ] -map         [path]          -- Set custom .map file location
12:11:30.151 | |        [  info  ] -f           [name]          -- Start new function configuration
12:11:30.151 | |        [  info  ] -t           [name]          -- Start new transform configuration
12:11:30.151 | |        [  info  ] -g           [name]          -- Start new transform global configuration
12:11:30.151 | |        [  info  ] -v           [name] [value]  -- Push value
12:11:30.151 | [  info  ]  
12:11:30.151 | [  info  ] Examples:
12:11:30.152 | |        [  info  ] obfuscator hehe.exe -f main -t TransformName -v SomeName 1337
12:11:30.152 | |        [  info  ] obfuscator hehe.exe -f main -t TransformName -v SomeName 1337 -g TransformName -v SomeGlobalName 1337
12:11:30.152 | |        [  info  ] obfuscator hehe.exe -f main -t TransformName -v SomeName 1337 -v SomeName0 1337 -g TransformName -v SomeGlobalName 1337
12:11:30.152 | |        [  info  ] obfuscator hehe.exe -map mymap.map -pdb mypdb.pdb -f main -t TransformName -v SomeName 1337 -v SomeName0 1337 -g TransformName -v SomeGlobalName 1337
```

## Writeup
- [https://blog.es3n1n.eu/posts/obfuscator-pt-1](https://blog.es3n1n.eu/posts/obfuscator-pt-1)

___

## Building
This project is designed to be used on both Linux and Windows, and on any architecture.
Although you may need to update your compiler/libc++ to the latest version, as we're using some fancy C++23 features.

```commandline
cmake -B build -DOBFUSCATOR_BUILD_TESTS=0
cmake --build build --config Release
```

*If you are using Visual Studio on Windows, you can generate the sln project using the first command and build the project using the generated .sln file.*

___

## Special thanks
- [@j4ckson4800](https://github.com/j4ckson4800), [@bs1337](https://github.com/bs1337) - Code review, proof-reading

## License
GPL-3.0
