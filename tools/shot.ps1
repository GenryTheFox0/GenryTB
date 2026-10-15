# GenryTB V1: a page of the program drawn to a picture (checks) - its QML warnings into <out>.log
#   powershell -File tools\shot.ps1 <page> <out.png> [project] [delay ms]
param([string]$page, [string]$out, [string]$project = "", [int]$delay = 3200)
$env:QT_FORCE_STDERR_LOGGING = "1"
$log = "$out.log"
$a = @("--shot", $page, $out, "--delay", "$delay")
if ($project -ne "") { $a += @("--project", $project) }
$p = Start-Process -FilePath "$(Split-Path $PSScriptRoot -Parent)\app\GenryTB.exe" -ArgumentList $a -PassThru -Wait -RedirectStandardError $log
"exit $($p.ExitCode)"
if (Test-Path $log) { Get-Content $log | Select-Object -First 30 }
