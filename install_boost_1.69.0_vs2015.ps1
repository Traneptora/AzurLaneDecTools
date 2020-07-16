Write-Host "Installing boost 1.69.0..." -ForegroundColor Cyan

$StopWatch = New-Object System.Diagnostics.Stopwatch
$StopWatch.Start()

New-Item 'C:\Libraries' -ItemType Directory -Force

# 1.69.0
Measure-Command {
    Write-Host "Installing boost 1.69.0..." -ForegroundColor Cyan

    Write-Host "Downloading x86..."
    $exePath = "$env:TEMP\boost_1_69_0-msvc-14.0-32.exe"
    (New-Object Net.WebClient).DownloadFile('https://bintray.com/boostorg/release/download_file?file_path=1.69.0%2Fbinaries%2Fboost_1_69_0-msvc-14.0-32.exe', $exePath)

    Write-Host "Installing x86..."
    cmd /c start /wait "$exePath" /verysilent
    del $exePath
    
    [IO.Directory]::Move('C:\local\boost_1_69_0', 'C:\Libraries\boost_1_69_0')

    Remove-Item 'C:\local' -Force -Recurse

    Write-Host "Compressing..."

    Start-ProcessWithOutput "compact /c /i /q /s:C:\Libraries\boost_1_69_0" -IgnoreStdOut
}

$StopWatch.Stop()
Write-Host "Boost libraries installed in $("{0:hh}:{0:mm}:{0:ss}" -f $StopWatch.elapsed)" -ForegroundColor Green