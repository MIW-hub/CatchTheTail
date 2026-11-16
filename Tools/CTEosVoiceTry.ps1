# EOS 음성 직접 확인용 실행기: 방장과 참가자를 한 컴퓨터에서 두 창으로 띄워 놓고 끝냄 (창은 직접 닫으세요)
# 사용법: powershell -File Tools\CTEosVoiceTry.ps1   (에디터를 끄고 실행)
#   헤드셋을 쓰세요. 한 창에서 V를 누르고 말하면, 다른 창(다른 사람)에서 그 목소리가 나는지 확인합니다.
#   두 창 모두 같은 마이크와 스피커를 쓰므로 스피커로 들으면 마이크가 소리를 다시 잡아 울릴 수 있습니다.
$root = Split-Path -Parent $PSScriptRoot
$out = Join-Path $root "docs\baseline"
New-Item -ItemType Directory -Force $out | Out-Null
$hl = Join-Path $out "voicetry_host.log"; $cl = Join-Path $out "voicetry_client.log"
Remove-Item $hl, $cl -Force -ErrorAction SilentlyContinue
if (Get-Process UnrealEditor* -ErrorAction SilentlyContinue) { Write-Error "에디터가 켜져 있습니다. 먼저 끄세요."; exit 1 }

$exe = "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe"
$base = "`"$root\CatchTheTail.uproject`" /Game/Levels/L_Mainmenu -game -windowed -ResX=900 -ResY=500 -nosplash -nop4 -log -NOSTEAM"
$null = Start-Process $exe -ArgumentList "$base -WinX=0 -WinY=0 -ini:Engine:[SystemSettings]:ct.AutoHostAfterLogin=1 -ABSLOG=`"$hl`""
$code = $null
$deadline = (Get-Date).AddSeconds(90)
while ((Get-Date) -lt $deadline -and -not $code) {
	Start-Sleep -Seconds 3
	if (Test-Path $hl) {
		$m = Select-String -Path $hl -Pattern "\[CT\] 방 코드: (\S+)" -Encoding UTF8 | Select-Object -First 1
		if ($m) { $code = $m.Matches[0].Groups[1].Value }
	}
}
if (-not $code) { "방장이 방 코드를 만들지 못함"; exit 3 }
Start-Sleep -Seconds 8
$null = Start-Process $exe -ArgumentList "$base -WinX=920 -WinY=0 -ini:Engine:[SystemSettings]:ct.AutoJoinCode=$code -ini:Engine:[SystemSettings]:ct.EosFreshDeviceId=1 -ABSLOG=`"$cl`""
"방 코드 $code 로 두 창을 띄웠습니다. 왼쪽이 방장, 오른쪽이 참가자입니다."
"1) 두 창이 로비에 모이면, 한 창을 클릭해 선택하고 V 키를 누른 채 말해 보세요."
"2) 다른 창에서 그 목소리가 들리는지 확인하세요. (헤드셋 권장)"
"3) 끝나면 두 창을 직접 닫으세요."
