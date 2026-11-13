# 보물찾기 한 판(AI 3명 + 사람 1명)을 돌려 흐름을 검사합니다. 에디터를 끄고, 빌드 성공 후 실행하세요.
# 사용법: powershell -File Tools\CTTreasureCheck.ps1            (에디터 빌드로 확인)
#         powershell -File Tools\CTTreasureCheck.ps1 -Packaged   (패키징된 HeyChicken.exe로 확인)
param([string]$Map = "/Game/Levels/L_Building", [switch]$Packaged, [int]$AiCount = 3, [int]$RoundSeconds = 20)

$root = Split-Path -Parent $PSScriptRoot
$name = "treasure_check"
$args2 = @("-ExecutionPolicy", "Bypass", "-File", "$root\Tools\CTBaseline.ps1", "-Name", $name, "-Map", $Map, "-Mode", 1, "-AiCount", $AiCount, "-GameSeconds", ($RoundSeconds * 3), "-RoundSeconds", $RoundSeconds, "-TimeoutSeconds", 220)
if ($Packaged) { $args2 += "-Packaged" }
& powershell @args2 | Out-Null

$events = Get-Content "$root\docs\baseline\$name.events.txt" -Encoding UTF8
$log = "$root\docs\baseline\$name.log"
$fail = @()
function Count($pattern) { return @($events | Where-Object { $_ -match $pattern }).Count }

if ((Count "게임 모드 규칙 생성: 보물찾기") -ne 1) { $fail += "보물찾기 규칙이 만들어지지 않음" }
if ((Count "보물찾기 시작: 팀0 \d+명, 팀1 \d+명") -ne 1) { $fail += "팀 배정 로그가 1번이 아님" }
if ((Count "에너지 100개 배치") -ne 3) { $fail += "라운드마다 에너지 100개 배치가 3번이 아님" }
if ((Count "무작위 스폰") -ne 3) { $fail += "라운드마다 무작위 스폰이 3번이 아님" }
if ((Count "보물찾기 라운드 \d 정산") -ne 3) { $fail += "라운드 정산이 3번이 아님" }
if ((Count "보물찾기 종료") -ne 1) { $fail += "보물찾기 종료 결과가 1번이 아님" }
if ((Count "던지기 오브젝트 20개 배치") -ne 3) { $fail += "라운드마다 던지기 오브젝트 20개 배치가 3번이 아님" }
if ((Count "게임 종료") -ne 1) { $fail += "게임 종료가 1번이 아님" }
$picked = Count "오브젝트 들기"; $thrown = Count "오브젝트 던짐"; $hit = Count "보물찾기 맞음"
if ($AiCount -gt 0 -and $picked -le 0) { $fail += "AI가 오브젝트를 한 번도 들지 않음" }
# 에너지를 누가 먹기는 했는지 (AI가 있으면 합계가 0이 아니어야 함)
$sum = 0
foreach ($e in ($events | Where-Object { $_ -match "정산: 팀0 (\d+) / 팀1 (\d+)" })) { if ($e -match "팀0 (\d+) / 팀1 (\d+)") { $sum += [int]$Matches[1] + [int]$Matches[2] } }
if ($AiCount -gt 0 -and $sum -le 0) { $fail += "에너지를 먹은 플레이어가 없음 (AI 수집 동작 확인 필요)" }
if (Select-String -Path $log -Pattern "Fatal error|Assertion failed" -Quiet -Encoding UTF8) { $fail += "치명적 오류 로그" }

$events | ForEach-Object { $_ }
if ($fail.Count -gt 0) { Write-Output "보물찾기 검사: FAIL"; $fail | ForEach-Object { "  - $_" }; exit 3 }
Write-Output "보물찾기 검사: PASS (먹은 에너지 합계 $sum, 오브젝트 들기 $picked / 던짐 $thrown / 맞음 $hit)"
