# Build and Release Guide

This project is Windows-only. Use Windows 10/11 with Visual Studio 2022 Build Tools, CMake 3.20+, Git, and Dear ImGui v1.91.8.

## Local Release Build

Clone Dear ImGui if it is not already present:

```batch
git clone --depth 1 --branch v1.91.8 https://github.com/ocornut/imgui.git third_party\imgui
```

Configure and build the standalone release executable:

```batch
cmake --preset windows-release
cmake --build --preset windows-release
```

The output is:

```text
build\Release\CS2SkinChanger.exe
```

## DLL Build

The DLL preset exists for parity with the existing CMake option:

```batch
cmake --preset windows-dll
cmake --build --preset windows-dll
```

The output is written under:

```text
build-dll\Release\
```

## Release Checklist

1. Confirm the working tree is clean.
2. Bump versions in `CMakeLists.txt`, `src/resources/version.rc`, and `src/resources/app.manifest`.
3. Run a local Windows release build with `cmake --build --preset windows-release`.
4. Commit the version and release notes changes.
5. Tag the release:

   ```batch
   git tag vX.Y.Z
   git push origin main
   git push origin vX.Y.Z
   ```

6. The `Build Release` GitHub Actions workflow builds the executable and uploads `CS2SkinChanger.exe` to the GitHub Release.

## Notes

- `third_party/`, `build/`, `build-dll/`, and generated binaries are intentionally ignored.
- The GitHub Release workflow is tag-driven and only runs for tags matching `v*.*.*`.
- Use `setup.bat` for the guided Windows setup path.
