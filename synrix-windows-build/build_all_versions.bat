@echo off
REM Build all SYNRIX package versions
REM Simple: Just run this script to build everything

echo ========================================
echo   SYNRIX Package Builder
echo ========================================
echo.

echo Building all versions...
echo.

REM Build Free Tier 50k
echo [1/3] Building Free Tier 50k...
python build_package.py --limit 50000 --name free_tier_50k
if errorlevel 1 (
    echo ERROR: Free Tier 50k build failed!
    pause
    exit /b 1
)

echo.
echo [2/3] Building Free Tier 100k...
python build_package.py --limit 100000 --name free_tier_100k
if errorlevel 1 (
    echo ERROR: Free Tier 100k build failed!
    pause
    exit /b 1
)

echo.
echo [3/3] Building Unlimited...
python build_package.py --unlimited --name unlimited
if errorlevel 1 (
    echo ERROR: Unlimited build failed!
    pause
    exit /b 1
)

echo.
echo ========================================
echo   All Packages Built Successfully!
echo ========================================
echo.
echo Packages are in: packages/
echo.
pause
