# Repository Guidelines

## Project Structure & Module Organization
`src/` hosts the ESP-IDF/Arduino component entry (`M5Unified.cpp/.hpp`) plus hardware-specific helpers under `src/utility/{imu,led,power,rtc}`. `examples/Basic/` and `examples/Advanced/` are Arduino sketches grouped by feature, while `examples/PlatformIO_SDL` is a native SDL harness whose `platformio.ini` pulls this repo in via `lib_extra_dirs`. Build metadata lives in `CMakeLists.txt`, `component.mk`, `library.json`, and `library.properties`, with supporting schematics in `docs/img`.

## Build, Test, and Development Commands
- `platformio run -d examples/PlatformIO_SDL -e native` verifies the SDL host harness compiles against the root library; use the `native_…` envs to cover each board flag.
- `platformio run -d examples/PlatformIO_SDL -e esp32_arduino` (or `esp32c3_arduino`, `esp32s3_arduino`) checks Arduino-framework builds for each chip line defined in `platformio.ini`.
- `platformio run -d examples/PlatformIO_SDL -e esp32_idf` exercises the ESP-IDF flow through PlatformIO so its `CMakeLists.txt`/`component.mk` paths stay valid.
- Use `arduino-cli compile --fqbn <board> examples/Basic/<ExampleName>` when updating a sketch to keep IDE consumers happy.
- Once installed as an IDF component, run `idf.py build` from your ESP-IDF project to confirm the component links.

## Coding Style & Naming Conventions
Keep ANSI C++ style consistent: braces on their own lines, four-space indentation, and brief `//` comments for complex mappings. Classes/structs follow `CamelCase`, helper enums use `board_t::board_*`, and macros remain uppercase (`CONFIG_*`, `NON_BREAK`). Match the existing casing when adding new `utility/` helpers or board constants, and prefer member functions in `PascalCase` when mirroring the `M5Unified` API.

## Testing Guidelines
No automated suite exists—testing means building sketches and hardware examples. Treat each example folder as a test target (e.g., `HowToUse`, `Button`, `Speaker`), describe your manual steps in the PR, and if you add host assertions rerun `platformio test -d examples/PlatformIO_SDL -e native` after the build.

## Commit & Pull Request Guidelines
Adopt the existing `type(scope): summary` pattern (`fix(tab5): …`, `feat`, `chore`). Include the affected board or module in scope, mention related issues, and list the commands or hardware you used to verify the change in the PR description. When power/IMU/audio code changes, tag maintainers so they can confirm on the relevant device matrix.

## Configuration & Dependency Tips
Keep `library.json`/`library.properties` synced with the `M5GFX` version and bump `version` when releasing. Reference `examples/PlatformIO_SDL/platformio.ini` when adding new PlatformIO envs—just extend `esp32_base` and adjust `framework`/`board` overrides.
