# 3개 맵 x 2개 모드 x (플레이 컷, 공중) = 12편을 1분씩 1920x1080 으로 촬영해 Saved\Broadcast 에 모읍니다. 에디터를 끄고 실행.
$root = Split-Path -Parent $PSScriptRoot
$dest = "$root\Saved\Broadcast"
New-Item -ItemType Directory $dest -Force | Out-Null
$maps = @(@("L_Box", "cube", 1.0), @("L_Country", "village", 2.2), @("L_Building", "building", 1.5))
$modes = @(@(0, "tailchase"), @(1, "treasure"))
$styles = @(@(1, "play", 1.0), @(2, "aerial", 0.0))
foreach ($m in $maps) {
  foreach ($mo in $modes) {
    foreach ($st in $styles) {
      $name = "{0}_{1}_{2}" -f $m[1], $mo[1], $st[1]
      $scale = 1.0
      if ($st[0] -eq 2) { $scale = $m[2] }
      & "$root\Tools\CTRenderBroadcast.ps1" -Map $m[0] -Mode $mo[0] -Style $st[0] -Seconds 60 -Scale $scale -Name $name | Out-Null
      $f = Get-ChildItem "$root\Saved\CineRender" -File | Where-Object { $_.Extension -in ".avi", ".mp4" } | Select-Object -First 1
      if ($f) { Move-Item $f.FullName "$dest\$name$($f.Extension)" -Force; "{0} 완료" -f $name } else { "{0} 실패(영상 없음)" -f $name }
    }
  }
}
"ALL_DONE"
