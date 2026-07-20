Package: lz4:x64-windows@1.10.0

**Host Environment**

- Host: x64-windows
- Compiler: MSVC 19.44.35228.0
- CMake Version: 4.3.3
-    vcpkg-tool version: 2026-05-27-d5b6777d666efc1a7f491babfcdab37794c1ae3e
    vcpkg-scripts version: unknown

**To Reproduce**

`vcpkg install `

**Failure logs**

```
Downloading https://github.com/lz4/lz4/archive/v1.10.0.tar.gz -> lz4-lz4-v1.10.0.tar.gz
Attempt 1 of 3, retrying download.
Attempt 2 of 3, retrying download.
error: Download timed out.
error: Download timed out.
error: Reached maximum number of attempts, won't retry download from https://github.com/lz4/lz4/archive/v1.10.0.tar.gz.
CMake Error at scripts/cmake/vcpkg_download_distfile.cmake:136 (message):
  Download failed, halting portfile.
Call Stack (most recent call first):
  scripts/cmake/vcpkg_from_github.cmake:120 (vcpkg_download_distfile)
  ports/lz4/portfile.cmake:1 (vcpkg_from_github)
  scripts/ports.cmake:206 (include)



```

**Additional context**

<details><summary>vcpkg.json</summary>

```
{
  "name": "my-video-conference",
  "version": "0.1.0",
  "dependencies": [
    "qtbase",
    "zlib"
  ]
}

```
</details>
