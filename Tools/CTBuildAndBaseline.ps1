# 에디터 빌드 후 성공했을 때만 꼬리잡기 기준선과 비교합니다. (에디터를 끄고 실행)
# 사용법: powershell -File Tools\CTBuildAndBaseline.ps1 -Name step3a
param([string]$Name = "check", [string]$CompareTo = "before")
$root = Split-Path -Parent $PSScriptRoot
if (Get-Process UnrealEditor* -ErrorAction SilentlyContinue) { Write-Error "에디터가 켜져 있습니다."; exit 1 }
$build = & "C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat" CatchTheTailEditor Win64 Development -Project="$root\CatchTheTail.uproject" -WaitMutex 2>&1
$errors = $build | Select-String " error "
$result = $build | Select-String "Result:"
$errors | Select-Object -First 8
$result
if (-not ($result -match "Succeeded")) { Write-Output "BUILD FAILED - 기준선 비교 건너뜀"; exit 4 }
& powershell -ExecutionPolicy Bypass -File "$root\Tools\CTBaseline.ps1" -Name $Name -CompareTo $CompareTo | Select-Object -First 3
$code = $LASTEXITCODE
Remove-Item "$root\docs\baseline\$Name.*" -ErrorAction SilentlyContinue
exit $code
