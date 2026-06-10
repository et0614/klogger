param(
  [int]$Baud = 9600,
  [string]$IdentifyCmd = "ID?",
  [string]$IdentifyExpect = "K-Logger",
  [string]$TimeSetCmd = "SETTIME",
  [string]$TimeSetAck = "Time set OK",
  [string]$CalibrateCmd = "CALIBRATE",
  [string]$CalibrateAck = "CAL OK",
  [string]$MeasureCmd = "CMEASURE",
  [string]$MeasureAck = "Start continuous measuring",
  [string]$StopCmd = "STOP",
  [string]$StopAck = "Stop continuous measuring",
  [switch]$ContinuousMode, # 連続計測モード用スイッチ
  [int]$ReadTimeoutMs = 7000,
  [int]$WriteDelayMs = 50,
  [int]$CalibrateDistance = -1
)

# ---- Get candidate COM ports ----
function Get-CandidatePorts {
  return ([System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object)
}

# ---- Open COM port ----
function New-OpenPort([string]$name, [int]$baud) {
  $sp = New-Object System.IO.Ports.SerialPort $name,$baud,'None',8,'One'
  $sp.ReadTimeout  = $ReadTimeoutMs
  $sp.WriteTimeout = 500
  $sp.NewLine      = "`r`n"
  $sp.Encoding     = [System.Text.Encoding]::ASCII
  try { $sp.Open() } catch { return $null }
  return $sp
}

# ---- Send one line ----
function Send-Line([System.IO.Ports.SerialPort]$sp, [string]$line) {
  try {
    $sp.DiscardInBuffer()
    $sp.WriteLine($line)
    Start-Sleep -Milliseconds $WriteDelayMs
    return $true
  } catch { return $false }
}

# ---- Read one line safely ----
function Read-LineSafe([System.IO.Ports.SerialPort]$sp) {
  try { return $sp.ReadLine().Trim() } catch { return $null }
}


# ---- Main ----
$timestamp = Get-Date -Format 'yyyy-MM-ddTHH:mm:ss'

if ($ContinuousMode) {
    Write-Host "Mode: Continuous Measurement (Press Any Key to Stop)"
} elseif ($CalibrateDistance -ge 0) {
    Write-Host "Mode: Calibration, Target distance: $CalibrateDistance mm"
} else {
    Write-Host "Mode: Time Setting, Timestamp to send: $timestamp"
}

$candidates = Get-CandidatePorts
if ($candidates.Count -eq 0) {
  Write-Host "ERROR: No COM port found. Check USB connection and driver."
  exit 1
}

Write-Host "Candidate ports: $($candidates -join ', ')"
$target = $null

foreach ($p in $candidates) {
  Write-Host "Trying: $p ..."
  $sp = New-OpenPort -name $p -baud $Baud
  if (-not $sp) { Write-Host "  Could not open."; continue }

  if (-not (Send-Line $sp $IdentifyCmd)) { $sp.Close(); continue }

  $resp = Read-LineSafe $sp
  if ($resp -and $resp.Contains($IdentifyExpect)) {
    Write-Host "  Identification OK: '$resp'"
    $target = $sp
    break
  } else {
    Write-Host "  Identification failed: response='$resp'"
    $sp.Close()
  }
}

if (-not $target) {
  Write-Host "ERROR: Target device not found."
  exit 2
}

# --- Command Logic ---
if ($ContinuousMode) {
    # --- 連続計測モード ---
    
    # 1. 計測開始コマンド送信
    Write-Host "Sending: $MeasureCmd"
    if (-not (Send-Line $target $MeasureCmd)) {
        Write-Host "ERROR: Send failed"; $target.Close(); exit 3
    }
    
    # 2. 開始ACK確認
    $resp = Read-LineSafe $target
    # main.cppの出力に合わせて判定
    if ($resp -and $resp.Contains("Start continuous measuring")) {
        Write-Host "SUCCESS: Measurement Started."
        Write-Host "----------------------------------------"
        Write-Host " Press ANY KEY to Stop Measurement... "
        Write-Host "----------------------------------------"
    } else {
        Write-Host "WARNING: Unexpected start response. Response='$resp'"
        # ACKがおかしくてもデータが来るかもしれないので続行してみる
    }

    # 3. ループ処理 (キー入力監視)
    # ユーザー入力を受け付けるため、読み込みタイムアウトを一時的に短くする
    $originalTimeout = $target.ReadTimeout
    $target.ReadTimeout = 500 # 0.5秒ごとにキー入力をチェック

    try {
        while ($target.IsOpen) {
            # --- キー入力チェック ---
            if ([Console]::KeyAvailable) {
                # キーバッファを読み捨ててループを抜ける
                $null = [Console]::ReadKey($true)
                Write-Host "`r`nStopping requested by user..."
                break
            }

            # --- データ受信 ---
            try {
                $line = $target.ReadLine()
                if ($line) {
                    $trimmed = $line.Trim()
                    # "MES "で始まるデータなら整形して表示
                    if ($trimmed.StartsWith("MES ")) {
                        # "MES " (4文字) を削除して " mm" を付ける
                        $val = $trimmed.Substring(4)
                        Write-Host "$val mm"
                    } else {
                        # それ以外（エラーやACKなど）はそのまま表示
                        Write-Host $trimmed
                    }
                }
            } catch [System.TimeoutException] {
                # タイムアウトは無視してループ継続 (キー入力チェックのため)
            } catch {
                Write-Host "Read Error: $_"
                break
            }
        }
    } finally {
        # タイムアウト設定を戻す
        $target.ReadTimeout = $originalTimeout
    }

    # 4. 停止コマンド送信
    Write-Host "Sending: $StopCmd"
    # バッファに残っているデータを捨ててから送信
    $target.DiscardInBuffer()
    
    if (-not (Send-Line $target $StopCmd)) {
        Write-Host "ERROR: Stop Send failed"; $target.Close(); exit 3
    }

    # 5. 停止ACK確認
    $resp = Read-LineSafe $target
    $target.Close()

    if ($resp -and $resp.Contains("Stop continuous measuring")) {
        Write-Host "SUCCESS: Measurement Stopped cleanly."
        exit 0
    } else {
        Write-Host "WARNING: Stopped but unexpected response. Response='$resp'"
        exit 0
    }

} elseif ($CalibrateDistance -ge 0) {
    # Calibration command
    $cmd = "$CalibrateCmd $CalibrateDistance"
    Write-Host "Sending: $cmd"
    if (-not (Send-Line $target $cmd)) {
        Write-Host "ERROR: Send failed"; $target.Close(); exit 3
    }
    $resp = Read-LineSafe $target
    $target.Close()

    if ($resp -and $resp.Contains($CalibrateAck)) {
        Write-Host "SUCCESS: Calibration complete. Response='$resp'"
        exit 0
    } else {
        Write-Host "ERROR: Calibration failed. Response='$resp'"
        exit 4
    }
} else {
    # Time setting command
    $cmd = "$TimeSetCmd $timestamp"
    Write-Host "Sending: $cmd"
    if (-not (Send-Line $target $cmd)) {
      Write-Host "ERROR: Send failed"; $target.Close(); exit 3
    }

    $resp = Read-LineSafe $target
    $target.Close()

    if ($resp -and $resp.Contains($TimeSetAck)) {
      Write-Host "SUCCESS: Time set. Response='$resp'"
      exit 0
    } else {
      Write-Host "WARNING: No valid response. Response='$resp'"
      exit 4
    }
}