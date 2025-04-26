@echo off

REM Check if Vulkan SDK is installed
if not defined VULKAN_SDK (
    echo Vulkan SDK not found. Installing...
    if exist VulkanSDK-1.4.309.0-Installer.exe (
        VulkanSDK-1.4.309.0-Installer.exe
    ) else (
        echo ERROR: VulkanSDK installer not found! Please place VulkanSDK.exe next to this script.
        pause
        exit /b
    )
) else (
    echo Vulkan SDK found at %VULKAN_SDK%
)

pushd ..
vendor\bin\premake\Windows\premake5.exe --file=Build-Walnut-GambitEngine.lua vs2022
popd
pause