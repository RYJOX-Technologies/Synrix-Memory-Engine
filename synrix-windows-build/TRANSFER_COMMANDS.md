# Transfer Commands for Windows

## Step 1: Create Transfer Package (Already Done)

The package is at: `/tmp/synrix-windows-build.tar.gz`

## Step 2: Transfer to Windows

### Option A: PowerShell with OpenSSH (Recommended)

If you have OpenSSH installed (Windows 10/11 usually has it):

```powershell
# Navigate to Downloads
cd $env:USERPROFILE\Downloads

# Transfer the file
scp astro@192.168.1.185:/tmp/synrix-windows-build.tar.gz .
```

### Option B: WSL (If you have WSL2)

```bash
# In WSL terminal
cd ~/Downloads
scp astro@192.168.1.185:/tmp/synrix-windows-build.tar.gz .
```

### Option C: Using WinSCP (GUI Tool)

1. Download WinSCP: https://winscp.net/
2. Connect to: `astro@192.168.1.185`
3. Navigate to `/tmp/`
4. Download `synrix-windows-build.tar.gz` to Downloads

### Option D: Using FileZilla (GUI Tool)

1. Download FileZilla: https://filezilla-project.org/
2. Connect via SFTP to: `astro@192.168.1.185`
3. Navigate to `/tmp/`
4. Download `synrix-windows-build.tar.gz` to Downloads

## Step 3: Extract on Windows

### Using 7-Zip (Recommended)

1. Right-click `synrix-windows-build.tar.gz`
2. Select "7-Zip" → "Extract Here"
3. This creates `synrix-windows-build.tar`
4. Right-click the `.tar` file
5. Select "7-Zip" → "Extract Here"
6. You'll get `synrix-windows-build/` folder

### Using PowerShell (Windows 10/11)

```powershell
cd $env:USERPROFILE\Downloads

# Extract .tar.gz (requires tar command, available in Windows 10/11)
tar -xzf synrix-windows-build.tar.gz
```

### Using WSL

```bash
cd ~/Downloads
tar -xzf synrix-windows-build.tar.gz
```

## Step 4: Build

```powershell
cd $env:USERPROFILE\Downloads\synrix-windows-build
.\build.ps1
```

## Troubleshooting

### "scp: command not found" in PowerShell

Install OpenSSH:
```powershell
# Run as Administrator
Add-WindowsCapability -Online -Name OpenSSH.Client~~~~0.0.1.0
```

Or use WinSCP/FileZilla GUI tools instead.

### "Permission denied"

Make sure you have SSH key set up, or use password:
```powershell
scp astro@192.168.1.185:/tmp/synrix-windows-build.tar.gz .
# Enter password when prompted
```

### File size check

To verify the file transferred correctly:
```powershell
Get-Item synrix-windows-build.tar.gz | Select-Object Name, Length
```

Expected size: ~500KB - 2MB (depending on source files)
