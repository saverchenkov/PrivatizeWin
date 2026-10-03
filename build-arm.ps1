# PrivatizeWin ARM64 Build Helper
param(
    [string]$Config = "Release",
    [string]$OutDir = "bin",
    [string]$BuildDir = "build_arm"
)

$ErrorActionPreference = "Stop"

if (!(Test-Path $OutDir)) {
    New-Item -ItemType Directory -Path $OutDir -Force | Out-Null
}

# Locate vcvarsamd64_arm64.bat
$vcvarsCandidates = @(
    "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvarsamd64_arm64.bat",
    "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsamd64_arm64.bat",
    "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsamd64_arm64.bat",
    "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsamd64_arm64.bat",
    "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsamd64_arm64.bat"
)

$vcvarsArm = $null
foreach ($path in $vcvarsCandidates) {
    if (Test-Path $path) {
        $vcvarsArm = $path
        break
    }
}

if (!$vcvarsArm) {
    $found = Get-ChildItem "C:\Program Files*\Microsoft Visual Studio" -Recurse -Filter "vcvarsamd64_arm64.bat" -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($found) { $vcvarsArm = $found.FullName }
}

if ($vcvarsArm) {
    Write-Host "[PrivatizeWin] Using ARM64 toolchain: $vcvarsArm"
    $env:VSCMD_SKIP_SENDTELEMETRY = "1"
    $cmd = "call `"$vcvarsArm`" && cmake -B $BuildDir -G Ninja -DCMAKE_BUILD_TYPE=$Config -DARCH_SUFFIX=_arm -DCMAKE_SYSTEM_NAME=Windows -DCMAKE_SYSTEM_PROCESSOR=ARM64 -DBUILD_TESTING=OFF && cmake --build $BuildDir --config $Config"
    cmd.exe /c $cmd
    if ($LASTEXITCODE -eq 0) {
        Copy-Item "$BuildDir/PrivatizeWin_arm.exe" "$OutDir/PrivatizeWin_arm.exe" -Force
        Copy-Item "$BuildDir/PrivatizeWin_arm64.exe" "$OutDir/PrivatizeWin_arm64.exe" -Force
        Write-Host "[PrivatizeWin] ARM build complete: $OutDir\PrivatizeWin_arm.exe and $OutDir\PrivatizeWin_arm64.exe"
    } else {
        exit $LASTEXITCODE
    }
} else {
    Write-Host "[PrivatizeWin] Notice: MSVC ARM64 cross-compiler (vcvarsamd64_arm64.bat) not found on this machine." -ForegroundColor Yellow
    Write-Host "[PrivatizeWin] To build ARM locally, install 'MSVC v143 - VS 2022 C++ ARM64/ARM64EC build tools' in Visual Studio Installer." -ForegroundColor Yellow
    Write-Host "[PrivatizeWin] GitHub Actions CI cross-compiles and publishes both _x65 and _arm binaries automatically on push." -ForegroundColor Cyan
}
