# GenryTB V1: my own test window of the game (never the one already running) - started, photographed every
# few seconds by its window handle, closed. No input is sent anywhere.
#   powershell -File tools\selfrun.ps1 <out prefix> [seconds] [every]
param([string]$out, [int]$seconds = 34, [double]$every = 2.0)
$game = $env:TB_GAME
if (-not $game) {
    $steam = (Get-ItemProperty "HKCU:\Software\Valve\Steam" -ErrorAction SilentlyContinue).SteamPath
    $game = @("$steam/steamapps/common/TinyBunny") + ("CDEFGH".ToCharArray() | ForEach-Object { "${_}:\SteamLibrary\steamapps\common\TinyBunny" }) |
        Where-Object { Test-Path "$_\TinyBunny.exe" } | Select-Object -First 1
}
if (-not $game) { "Tiny Bunny not found: set TB_GAME"; exit 1 }
$root = Split-Path $PSScriptRoot -Parent
$p = Start-Process -FilePath "$game\TinyBunny.exe" -ArgumentList @("--savedir", "$root\work\tb_saves") -WorkingDirectory $game -PassThru
$n = 0
$sw = [Diagnostics.Stopwatch]::StartNew()
while ($sw.Elapsed.TotalSeconds -lt $seconds -and -not $p.HasExited) {
    Start-Sleep -Milliseconds ([int]($every * 1000))
    $p.Refresh()
    $h = $p.MainWindowHandle
    if ($h -ne 0) {
        $n++
        um win shot --hwnd $h --scale 0.5 ("{0}_{1:d2}.png" -f $out, $n) | Out-Null
    }
}
# the hook goes at once: the game started by hand must start as usual
Remove-Item -Recurse -Force "$game\game\mods\_genry_tb_preview" -ErrorAction SilentlyContinue
if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force }
"shots $n, exited $($p.HasExited)"
if (Test-Path "$game\traceback.txt") { Get-Content "$game\traceback.txt" -TotalCount 40 }
