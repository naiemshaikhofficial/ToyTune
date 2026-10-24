Add-Type -AssemblyName System.Drawing

$installerDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$toyTuneDir = Split-Path -Parent $installerDir
$brandingDir = Join-Path $toyTuneDir "assets\branding"

$ptLogoPath = Join-Path $brandingDir "producer-toy-charcoal-logo.png"
$ttLogoPath = Join-Path $brandingDir "toytune-wordmark.png"
$mascotPath = Join-Path $brandingDir "manager-bear-mascot.png"

Write-Host "Branding files:"
Write-Host "PT Logo: $ptLogoPath - $(Test-Path $ptLogoPath)"
Write-Host "TT Logo: $ttLogoPath - $(Test-Path $ttLogoPath)"
Write-Host "Mascot:  $mascotPath - $(Test-Path $mascotPath)"

function Create-WizardBanner([int]$width, [int]$height, [string]$outFile) {
    $bmp = New-Object System.Drawing.Bitmap $width, $height, ([System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $g.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::ClearTypeGridFit

    # 1. Dark Luxury Matte Gradient (#0e0f12 to #050608)
    $pt1 = New-Object System.Drawing.Point 0, 0
    $pt2 = New-Object System.Drawing.Point 0, $height
    $cTop = [System.Drawing.Color]::FromArgb(16, 17, 21)
    $cBot = [System.Drawing.Color]::FromArgb(6, 7, 9)
    
    $brush = New-Object System.Drawing.Drawing2D.LinearGradientBrush ($pt1, $pt2, $cTop, $cBot)
    $rect = New-Object System.Drawing.Rectangle 0, 0, $width, $height
    $g.FillRectangle($brush, $rect)
    $brush.Dispose()

    # Subtle inner dark card well
    $scale = $width / 164.0
    $cardPadX = [int](8 * $scale)
    $cardPadY = [int](10 * $scale)
    $cardRect = New-Object System.Drawing.Rectangle $cardPadX, $cardPadY, ($width - 2 * $cardPadX), ($height - 2 * $cardPadY)
    $penCard = New-Object System.Drawing.Pen ([System.Drawing.Color]::FromArgb(34, 36, 44)), (1.0 * $scale)
    $g.DrawRectangle($penCard, $cardRect)
    $penCard.Dispose()

    # Right edge metallic accent line
    $penEdge = New-Object System.Drawing.Pen ([System.Drawing.Color]::FromArgb(45, 48, 58)), (1.2 * $scale)
    $g.DrawLine($penEdge, ($width - 1), 0, ($width - 1), $height)
    $penEdge.Dispose()

    # 2. Producer Toy Charcoal Logo at Top
    if (Test-Path $ptLogoPath) {
        $ptImg = [System.Drawing.Image]::FromFile($ptLogoPath)
        $ptTargetW = [int](120 * $scale)
        $ptTargetH = [int]($ptTargetW * ($ptImg.Height / $ptImg.Width))
        $ptX = [int](($width - $ptTargetW) / 2)
        $ptY = [int](22 * $scale)
        $g.DrawImage($ptImg, $ptX, $ptY, $ptTargetW, $ptTargetH)
        $ptImg.Dispose()
    }

    # Metallic hairline divider
    $divY = [int](68 * $scale)
    $penDiv = New-Object System.Drawing.Pen ([System.Drawing.Color]::FromArgb(50, 54, 66)), (1.0 * $scale)
    $g.DrawLine($penDiv, [int](24 * $scale), $divY, [int]($width - 24 * $scale), $divY)
    $penDiv.Dispose()

    # 3. ToyTune 3D Logo
    if (Test-Path $ttLogoPath) {
        $ttImg = [System.Drawing.Image]::FromFile($ttLogoPath)
        $ttTargetW = [int](136 * $scale)
        $ttTargetH = [int]($ttTargetW * ($ttImg.Height / $ttImg.Width))
        $ttX = [int](($width - $ttTargetW) / 2)
        $ttY = [int](80 * $scale)
        $g.DrawImage($ttImg, $ttX, $ttY, $ttTargetW, $ttTargetH)
        $ttImg.Dispose()
    }

    # Subtitle: REAL-TIME VOCAL TUNE
    $fontSize = [float](6.2 * $scale)
    $font = New-Object System.Drawing.Font ("Segoe UI", $fontSize, [System.Drawing.FontStyle]::Bold)
    $strFormat = New-Object System.Drawing.StringFormat
    $strFormat.Alignment = [System.Drawing.StringAlignment]::Center
    $brushTxt = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(145, 148, 160))
    $subY = [int](136 * $scale)
    $subRect = New-Object System.Drawing.RectangleF 0, $subY, $width, (16 * $scale)
    $g.DrawString("REAL-TIME VOCAL TUNE", $font, $brushTxt, $subRect, $strFormat)
    $font.Dispose()
    $brushTxt.Dispose()

    # 4. Angled Ventilation Grip Ridges
    $penVent = New-Object System.Drawing.Pen ([System.Drawing.Color]::FromArgb(40, 255, 255, 255)), (1.5 * $scale)
    for ($i = 0; $i -lt 4; $i++) {
        $vy = [int]((162 + $i * 8) * $scale)
        $g.DrawLine($penVent, [int](28 * $scale), $vy + [int](6 * $scale), [int](40 * $scale), $vy)
        $g.DrawLine($penVent, [int]($width - 40 * $scale), $vy + [int](6 * $scale), [int]($width - 28 * $scale), $vy)
    }
    $penVent.Dispose()

    # 5. Manager Bear Mascot at Bottom (Borderless, crisp drop shadow)
    if (Test-Path $mascotPath) {
        $mascotImg = [System.Drawing.Image]::FromFile($mascotPath)
        $mTargetSize = [int](92 * $scale)
        $mX = [int](($width - $mTargetSize) / 2)
        $mY = [int]($height - $mTargetSize - (18 * $scale))
        $g.DrawImage($mascotImg, $mX, $mY, $mTargetSize, $mTargetSize)
        $mascotImg.Dispose()
    }

    $g.Dispose()
    $bmp.Save($outFile, [System.Drawing.Imaging.ImageFormat]::Bmp)
    $bmp.Dispose()
    Write-Host "Created: $outFile"
}

function Create-WizardSmall([string]$outFile) {
    $width = 55
    $height = 55
    $bmp = New-Object System.Drawing.Bitmap $width, $height, ([System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality

    # Dark background
    $rect = New-Object System.Drawing.Rectangle 0, 0, $width, $height
    $brush = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(12, 13, 16))
    $g.FillRectangle($brush, $rect)
    $brush.Dispose()

    # Draw Mascot
    if (Test-Path $mascotPath) {
        $mascotImg = [System.Drawing.Image]::FromFile($mascotPath)
        $mSize = 46
        $mX = [int](($width - $mSize) / 2)
        $mY = [int](($height - $mSize) / 2)
        $g.DrawImage($mascotImg, $mX, $mY, $mSize, $mSize)
        $mascotImg.Dispose()
    }

    $g.Dispose()
    $bmp.Save($outFile, [System.Drawing.Imaging.ImageFormat]::Bmp)
    $bmp.Dispose()
    Write-Host "Created: $outFile"
}

function Create-AppIcon([string]$outFile) {
    if (Test-Path $mascotPath) {
        # Create a multi-resolution icon using System.Drawing
        $srcImg = [System.Drawing.Image]::FromFile($mascotPath)
        $sizes = @(256, 128, 64, 48, 32, 16)
        
        $ms = New-Object System.IO.MemoryStream
        $bw = New-Object System.IO.BinaryWriter $ms

        # ICONDIR header
        $bw.Write([uint16]0) # Reserved
        $bw.Write([uint16]1) # Type (1 = ICO)
        $bw.Write([uint16]$sizes.Count) # Image count

        $imageDataStreams = @()
        $offset = 6 + (16 * $sizes.Count)

        foreach ($s in $sizes) {
            $resized = New-Object System.Drawing.Bitmap $s, $s, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
            $rg = [System.Drawing.Graphics]::FromImage($resized)
            $rg.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
            $rg.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
            $rg.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
            $rg.DrawImage($srcImg, 0, 0, $s, $s)
            $rg.Dispose()

            $entryMs = New-Object System.IO.MemoryStream
            $resized.Save($entryMs, [System.Drawing.Imaging.ImageFormat]::Png)
            $entryBytes = $entryMs.ToArray()
            $entryMs.Dispose()
            $resized.Dispose()

            # ICONDIRENTRY
            $bWidth = if ($s -ge 256) { 0 } else { [byte]$s }
            $bHeight = if ($s -ge 256) { 0 } else { [byte]$s }
            $bw.Write([byte]$bWidth)
            $bw.Write([byte]$bHeight)
            $bw.Write([byte]0) # ColorCount
            $bw.Write([byte]0) # Reserved
            $bw.Write([uint16]1) # Planes
            $bw.Write([uint16]32) # BitCount
            $bw.Write([uint32]$entryBytes.Length) # BytesInRes
            $bw.Write([uint32]$offset) # ImageOffset

            $offset += $entryBytes.Length
            $imageDataStreams += ,$entryBytes
        }

        # Write actual image data
        foreach ($bytes in $imageDataStreams) {
            $bw.Write($bytes)
        }

        $srcImg.Dispose()
        [System.IO.File]::WriteAllBytes($outFile, $ms.ToArray())
        $bw.Dispose()
        $ms.Dispose()
        Write-Host "Created: $outFile"
    }
}

# Run generation
$largeBmp = Join-Path $installerDir "wizard_large.bmp"
$large2xBmp = Join-Path $installerDir "wizard_large_2x.bmp"
$smallBmp = Join-Path $installerDir "wizard_small.bmp"
$icoFile = Join-Path $installerDir "toytune.ico"

Create-WizardBanner 164 314 $largeBmp
Create-WizardBanner 328 628 $large2xBmp
Create-WizardSmall $smallBmp
Create-AppIcon $icoFile
Write-Host "All installer visual assets generated successfully!"
