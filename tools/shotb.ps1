# GenryTB V1: like shot.ps1, but the fresh build (build\GenryTB.exe with Qt from its own folder) - app\ untouched
param([string]$page, [string]$out, [string]$project = "", [int]$delay = 3200)
$env:QT_FORCE_STDERR_LOGGING = "1"
$qt = if ($env:GENRYTB_QT) { $env:GENRYTB_QT } else { "C:\Qt\6.8.3\msvc2022_64" }
$env:PATH = "$qt\bin;" + $env:PATH
$build = Join-Path (Split-Path $PSScriptRoot -Parent) "build"
$log = "$out.log"
$a = @("--shot", $page, $out, "--delay", "$delay")
if ($project -ne "") { $a += @("--project", $project) }
$p = Start-Process -FilePath "$build\GenryTB.exe" -ArgumentList $a -PassThru -Wait -RedirectStandardError $log -WorkingDirectory $build
"exit $($p.ExitCode)"
if (Test-Path $log) { Get-Content $log | Select-Object -First 30 }
