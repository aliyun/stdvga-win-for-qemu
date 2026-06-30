> **⚠️ This project has moved.**
>
> This driver has been contributed to and is now maintained upstream as part of the
> [kvm-guest-drivers-windows](https://github.com/virtio-win/kvm-guest-drivers-windows)
> project under the [`stdvga/`](https://github.com/virtio-win/kvm-guest-drivers-windows/tree/master/stdvga) directory.
>
> Please file all future issues and pull requests at the new repository.
>
> This repository is preserved as a historical archive and will no longer receive updates.

---

# QEMU Standard VGA Display Driver for Windows

A Windows Kernel Mode Display-Only Driver (KMDOD) for the QEMU Standard VGA device (PCI VEN_1234 / DEV_1111).

Windows UEFI guests using the default Microsoft Basic Display Adapter are stuck at a fixed resolution because there is no VGA BIOS to provide mode setting. This driver programs the Bochs VBE registers directly, enabling runtime resolution switching on both BIOS and UEFI boot modes.

## Features

- Runtime resolution switching via Windows Display Settings or `Set-DisplayResolution`
- 15 supported resolutions: 800x600, 1024x768, 1152x864, 1280x720, 1280x768, 1280x800, 1280x960, 1280x1024, 1400x1050, 1440x900, 1600x1200, 1680x1050, 1920x1080 (default), 1920x1200, 2560x1600
- Single Universal x64 package covering Windows 10 (1709+) / 11 and Windows Server 2019 / 2022 / 2025

## Building

Prerequisites: Visual Studio 2022, [Windows Driver Kit (WDK)](https://learn.microsoft.com/en-us/windows-hardware/drivers/download-the-wdk), Windows SDK.

```cmd
build.bat
```

Output is placed in `Install\Win10\amd64\` (`stdvga.sys` / `stdvga.inf` / `stdvga.cat`).

> The solution also exposes a `Win11 Release` configuration (`build.bat "Win11 Release"`), which produces a byte-identical Universal Driver in `Install\Win11\amd64\`. It is reserved for future Windows 11–only DDI work and is **not** required for normal builds or releases — a single package built from `Win10 Release` is the supported artifact.

## Installation

Download the latest release from [Releases](../../releases), extract, and install:

```cmd
pnputil /add-driver stdvga.inf /install
```

Uninstall:

```cmd
pnputil /delete-driver oemXX.inf /uninstall /force
```

## Usage

Interactive (VNC / RDP):

```powershell
Set-DisplayResolution -Width 1920 -Height 1080 -Force
```

> `Set-DisplayResolution` is only available on Windows Server. For client SKUs or unattended scenarios (SYSTEM session), use the registry method below:


```powershell
New-Item -Path 'HKLM:\System\CurrentControlSet\Services\StdVga\Parameters' -Force | Out-Null
Set-ItemProperty -Path 'HKLM:\System\CurrentControlSet\Services\StdVga\Parameters' -Name TargetWidth  -Value 1280 -Type DWord
Set-ItemProperty -Path 'HKLM:\System\CurrentControlSet\Services\StdVga\Parameters' -Name TargetHeight -Value 720  -Type DWord

# Clear dxgkrnl mode cache
Remove-Item 'HKLM:\SYSTEM\CurrentControlSet\Control\GraphicsDrivers\Configuration' -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item 'HKLM:\SYSTEM\CurrentControlSet\Control\GraphicsDrivers\Connectivity' -Recurse -Force -ErrorAction SilentlyContinue

# Restart the display device (no reboot needed)
$dev = (Get-PnpDevice -PresentOnly | Where-Object {
    $_.Class -eq 'Display' -and $_.HardwareID -match 'VEN_1234&DEV_1111'
} | Select-Object -First 1).InstanceId
pnputil /restart-device "$dev"
```

## License

[BSD 3-Clause](LICENSE) — Copyright (c) 2026 Alibaba Cloud Computing Ltd.
