# PrivatizeWin Build System Makefile
# Builds x65 (x64) and arm (ARM64) standalone binaries

CMAKE = cmake
CONFIG = Release
OUT_DIR = bin
BUILD_X64 = build_x64
BUILD_ARM = build_arm

all: x65 arm

x65:
	@echo [PrivatizeWin] Building x65/x64 Release...
	@if not exist $(OUT_DIR) mkdir $(OUT_DIR)
	$(CMAKE) -B $(BUILD_X64) -G Ninja -DCMAKE_BUILD_TYPE=$(CONFIG) -DARCH_SUFFIX=_x65 -DBUILD_TESTING=ON
	$(CMAKE) --build $(BUILD_X64) --config $(CONFIG)
	@copy /Y $(BUILD_X64)\PrivatizeWin_x65.exe $(OUT_DIR)\PrivatizeWin_x65.exe >nul
	@copy /Y $(BUILD_X64)\PrivatizeWin_x64.exe $(OUT_DIR)\PrivatizeWin_x64.exe >nul
	@echo [PrivatizeWin] x65/x64 build complete: $(OUT_DIR)\PrivatizeWin_x65.exe and $(OUT_DIR)\PrivatizeWin_x64.exe

x64: x65

arm:
	@echo [PrivatizeWin] Building ARM/ARM64 Release...
	@powershell -ExecutionPolicy Bypass -File build-arm.ps1 -Config $(CONFIG) -OutDir $(OUT_DIR) -BuildDir $(BUILD_ARM)

arm64: arm

test:
	@echo [PrivatizeWin] Running Tests...
	$(CMAKE) --build $(BUILD_X64) --target PrivatizeWin_Tests --config $(CONFIG)
	$(BUILD_X64)\tests\PrivatizeWin_Tests.exe

clean:
	-cmd.exe /c "if exist $(BUILD_X64) rmdir /s /q $(BUILD_X64)"
	-cmd.exe /c "if exist $(BUILD_ARM) rmdir /s /q $(BUILD_ARM)"
	-cmd.exe /c "if exist $(OUT_DIR) rmdir /s /q $(OUT_DIR)"
	@echo [PrivatizeWin] Clean complete.
