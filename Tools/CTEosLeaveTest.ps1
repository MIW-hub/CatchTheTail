# 방장이 방을 나간 뒤에는 그 방 코드로 들어갈 수 없는지(남은 방이 없는지) 확인하는 시험
# 사용법: powershell -File Tools\CTEosLeaveTest.ps1   (에디터를 끄고 실행)
$root = Split-Path -Parent $PSScriptRoot
$out = Join-Path $root "docs\baseline"
New-Item -ItemType Directory -Force $out | Out-Null
$hl = Join-Path $out "eosleave_host.log"; $cl = Join-Path $out "eosleave_client.log"
Remove-Item $hl, $cl -Force -ErrorAction SilentlyContinue
if (Get-Process UnrealEditor* -ErrorAction SilentlyContinue) { Write-Error "에디터가 켜져 있습니다. 먼저 끄세요."; exit 1 }

$exe = "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe"
$base = "`"$root\CatchTheTail.uproject`" /Game/Levels/L_Mainmenu -game -windowed -ResX=640 -ResY=360 -nosound -nosplash -nop4 -log -NOSTEAM"
$h = Start-Process $exe -ArgumentList "$base -WinX=0 -WinY=0 -ini:Engine:[SystemSettings]:ct.AutoHostAfterLogin=1 -ini:Engine:[SystemSettings]:ct.AutoLeaveSeconds=8 -ABSLOG=`"$hl`"" -PassThru
$code = $null
$deadline = (Get-Date).AddSeconds(90)
while ((Get-Date) -lt $deadline -and -not $code) {
	Start-Sleep -Seconds 3
	if (Test-Path $hl) {
		$m = Select-String -Path $hl -Pattern "\[CT\] 방 코드: (\S+)" -Encoding UTF8 | Select-Object -First 1
		if ($m) { $code = $m.Matches[0].Groups[1].Value }
	}
}
if (-not $code) { Get-Process UnrealEditor* -ErrorAction SilentlyContinue | Stop-Process -Force; "방장이 방 코드를 만들지 못함"; exit 3 }
"방 코드: $code"
# 방장이 나갈 때까지 기다린 뒤, 나간 지 12초 뒤에 다른 사람이 같은 코드로 찾아봄
$deadline = (Get-Date).AddSeconds(60)
while ((Get-Date) -lt $deadline) { Start-Sleep -Seconds 2; if ((Select-String -Path $hl -Pattern "방장이 방을 나감" -Quiet -Encoding UTF8)) { break } }
Start-Sleep -Seconds 12
$c = Start-Process $exe -ArgumentList "$base -WinX=660 -WinY=0 -ini:Engine:[SystemSettings]:ct.AutoJoinCode=$code -ini:Engine:[SystemSettings]:ct.EosFreshDeviceId=1 -ABSLOG=`"$cl`"" -PassThru
Start-Sleep -Seconds 40
Get-Process UnrealEditor*, CrashReportClient* -ErrorAction SilentlyContinue | Stop-Process -Force

$hostLog = Select-String -Path $hl -Pattern "\[CT-Test\]|\[CT\] (방 열기|방 코드|메인 메뉴로)|방 나가기" -Encoding UTF8 | ForEach-Object { ($_.Line -split "LogTemp: ")[-1] }
$clientLog = Select-String -Path $cl -Pattern "\[CT\] (온라인|방 검색 결과|방 입장)" -Encoding UTF8 | ForEach-Object { ($_.Line -split "LogTemp: ")[-1] }
"-- 방장"; $hostLog; "-- 찾는 사람"; $clientLog
$fail = @()
if (-not ($hostLog -match "방장이 방을 나감")) { $fail += "방장이 방을 나가지 않음" }
if (-not ($clientLog -match "방 검색 결과")) { $fail += "찾는 사람의 검색이 끝나지 않음" }
if ($clientLog -match "방 입장:") { $fail += "방장이 나갔는데 그 방에 들어가짐 (남은 방)" }
if ($fail.Count -eq 0) { "EOS 방 정리 시험: PASS (나간 방은 찾아지지 않음)" } else { "EOS 방 정리 시험: FAIL"; $fail | ForEach-Object { "  - $_" }; exit 3 }
