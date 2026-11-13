# 꼬리잡기 기준선(회귀 확인) 실행 스크립트
# 사용법: powershell -File Tools\CTBaseline.ps1 -Name before
#   - 에디터를 끄고 실행하세요.
#   - 짧은 설정(AI 3명, 경기 45초, 라운드 18초)으로 경기 맵을 직접 실행해 서버 로그를 모읍니다.
#   - 결과: docs\baseline\<Name>.log (전체 로그), docs\baseline\<Name>.events.txt ([CT] 흐름 로그만, 시각/좌표 제거)
#   - 단계마다 -Name 을 바꿔 실행한 뒤 events.txt 를 before 와 비교하세요 (Compare-Object 또는 diff).
param(
	[string]$Name = "before",
	[string]$Map = "/Game/Levels/L_Building",
	[int]$AiCount = 3,
	[int]$GameSeconds = 45,
	[int]$RoundSeconds = 18,
	[int]$TimeoutSeconds = 150,
	[string]$CompareTo = "",   # 비교할 이전 기준선 이름 (예: before). 비우면 비교 안 함
	[switch]$Packaged           # 패키징된 실행 파일(Packaged\Windows\HeyChicken.exe)로 확인
)

$root = Split-Path -Parent $PSScriptRoot
$out = Join-Path $root "docs\baseline"
New-Item -ItemType Directory -Force $out | Out-Null
$log = Join-Path $out "$Name.log"
if (Test-Path $log) { Remove-Item $log -Force }

if (Get-Process UnrealEditor* -ErrorAction SilentlyContinue) { Write-Error "에디터가 켜져 있습니다. 먼저 끄세요."; exit 1 }

$exe = "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe"
$cmds = "set CTGameMode DebugAIPlayerCount $AiCount,set CTGameMode GameDurationSeconds $GameSeconds,set CTGameMode RoundDurationSeconds $RoundSeconds"
$projectArg = "`"$root\CatchTheTail.uproject`" "
if ($Packaged) {
	$exe = Join-Path $root "Packaged\Windows\HeyChicken.exe"
	$projectArg = ""
	if (-not (Test-Path $exe)) { Write-Error "패키징 파일이 없습니다: $exe"; exit 1 }
}
$arguments = "$projectArg$Map -game -windowed -ResX=800 -ResY=450 -nosound -nosplash -nop4 -log -ExecCmds=`"$cmds`" -ABSLOG=`"$log`""
$p = Start-Process $exe -ArgumentList $arguments -PassThru

# 게임 종료 로그가 나오거나 시간이 다 되면 끝냄
$deadline = (Get-Date).AddSeconds($TimeoutSeconds)
$finished = $false
while ((Get-Date) -lt $deadline -and -not $p.HasExited) {
	Start-Sleep -Seconds 3
	if ((Test-Path $log) -and (Select-String -Path $log -Pattern "\[CT\] 게임 종료" -Quiet -Encoding UTF8)) { $finished = $true; Start-Sleep -Seconds 4; break }
}
if (-not $p.HasExited) { Stop-Process $p -Force }

# 흐름 로그만 추려서 시각, 좌표, 번호 같은 매번 달라지는 값을 지움
$lines = Select-String -Path $log -Pattern "\[CT\]" -Encoding UTF8 | ForEach-Object { $_.Line }
$flow = $lines | Where-Object { $_ -match "게임 시작|라운드 \d+ 시작|새 라운드 시작|AI 플레이어|게임 종료|잡음|꼬리 보충|메인 메뉴로 이동|입장 거절|이탈" } |
	ForEach-Object { ($_ -replace '^\[[^\]]*\]\[[^\]]*\]', '' ) -replace '\(-?\d+(\.\d+)?, -?\d+(\.\d+)?, -?\d+(\.\d+)?\)', '(x,y,z)' }
$eventsFile = Join-Path $out "$Name.events.txt"
$flow | Set-Content -Encoding UTF8 $eventsFile
Write-Output "게임 종료 로그 확인: $finished"
Write-Output "저장: $eventsFile ($($flow.Count)줄)"
# 매번 달라지는 AI 잡기, 꼬리 보충, 아이템 개수 줄을 뺀 뼈대만 비교 (뼈대 = 시작, 라운드, 종료의 순서와 횟수)
$skeleton = $flow | Where-Object { $_ -notmatch "잡음|꼬리 보충|새 라운드 시작" } | ForEach-Object { ($_ -replace "승자 \d+명", "승자 N명") -replace "동점=(예|아니오)", "동점=?" }
$skeletonFile = Join-Path $out "$Name.skeleton.txt"
$skeleton | Set-Content -Encoding UTF8 $skeletonFile
if ($CompareTo -ne "") {
	$other = Join-Path $out "$CompareTo.skeleton.txt"
	if (-not (Test-Path $other)) { Write-Output "비교 대상이 없습니다: $other"; exit 2 }
	$diff = Compare-Object (Get-Content $other -Encoding UTF8) (Get-Content $skeletonFile -Encoding UTF8)
	if ($diff) { Write-Output "비교 결과: 다름 (FAIL)"; $diff | ForEach-Object { "$($_.SideIndicator) $($_.InputObject)" } ; exit 3 }
	else { Write-Output "비교 결과: 같음 (PASS)" }
}
$flow
