@echo off
setlocal enabledelayedexpansion

REM Check if Vulkan SDK is installed
if not defined VULKAN_SDK (
    echo Vulkan SDK not found. Installing...

    REM Check if installer exists
    if exist VulkanSDK-1.4.309.0-Installer.exe (
        echo Found local Vulkan SDK installer. Running it...
    ) else (
        echo VulkanSDK installer not found locally. Downloading...

        curl -L -o VulkanSDK-1.4.309.0-Installer.exe https://sdk.lunarg.com/sdk/download/1.4.309.0/windows/VulkanSDK-1.4.309.0-Installer.exe

        if exist VulkanSDK-1.4.309.0-Installer.exe (
            echo Download successful.
        ) else (
            echo ERROR: Failed to download Vulkan SDK installer.
            pause
            exit /b
        )
    )

    REM Run the installer
    start /wait VulkanSDK-1.4.309.0-Installer.exe

    echo Vulkan SDK installation complete. You may need to restart this script if VULKAN_SDK is still not set.
    pause
    exit /b
) else (
    echo Vulkan SDK found at %VULKAN_SDK%.
)

REM Run premake
pushd ..
vendor\bin\premake\Windows\premake5.exe --file=Build-Walnut-GambitEngine.lua vs2022
popd

pause
