# Local libraries

Put custom Arduino libraries here if PlatformIO cannot install them from `lib_deps`.

This project still needs the weather authentication/decompression libraries used by the original code:

- `JwtUtil`, providing `JwtUtil.h` and `generateJWT(...)`
- `ArduinoZlib`, providing `ArduinoZlib.h`

Expected layout examples:

```text
lib/
  JwtUtil/
    src/
      JwtUtil.h
      JwtUtil.cpp
  ArduinoZlib/
    src/
      ArduinoZlib.h
      ArduinoZlib.cpp
```

