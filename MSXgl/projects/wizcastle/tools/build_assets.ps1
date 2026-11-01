#===============================================================================
# THE WIZARD'S CASTLE - MSX2+ DELUXE EDITION
# Asset Pipeline & Memory Mapper Packager (tools/build_assets.ps1)
#
# Converts winwiz/tiles32.png and winwiz/images.png into WIZDATA.DAT
# (114,688 bytes = 7 x 16 KB Memory Mapper Segments):
#   - Seg 0 (0x00000): Sprites  0..15 (32x32 SCREEN 8 GRB 3-3-2, 16 KB)
#   - Seg 1 (0x04000): Sprites 16..31 (32x32 SCREEN 8 GRB 3-3-2, 16 KB)
#   - Seg 2 (0x08000): Sprites 32..39 (8 KB) + 10 Treasure Icons 16x16 Owned &
#                      Unowned (5 KB) + WIZDATA1 Header at 0x3F00
#   - Seg 3 (0x0C000): 40 Map Tiles 16x16 (10 KB) + Dungeon Viewport BGs 48x48
#   - Seg 4 (0x10000): Splash / Combat Artwork (160x100 SCREEN 12 YJK, 16 KB)
#   - Seg 5 (0x14000): Victory Artwork         (160x100 SCREEN 12 YJK, 16 KB)
#   - Seg 6 (0x18000): Game Over / Death Art   (160x100 SCREEN 12 YJK, 16 KB)
#===============================================================================

Add-Type -AssemblyName System.Drawing

$rootDir = "C:\dos\wizcastle"
$tilesPath = Join-Path $rootDir "winwiz\tiles32.png"
$imagesPath = Join-Path $rootDir "winwiz\images.png"
$outPath = Join-Path $rootDir "MSXgl\projects\wizcastle\emul\WIZDATA.DAT"

Write-Output "Loading source images from winwiz/..."
$bmpTiles = [System.Drawing.Bitmap]::FromFile($tilesPath)
$bmpImages = [System.Drawing.Bitmap]::FromFile($imagesPath)

# Exact (row, col) coordinates in tiles32.png from winwiz/wfuncs.cpp object_data[40]
$objectCoords = @(
    @(25,  9), # 0:  UNSEEN_ROOM
    @(21,  1), # 1:  EMPTY_ROOM
    @(21,  4), # 2:  CASTLE_ENTRANCE
    @(21, 11), # 3:  STAIRS_UP
    @(21, 12), # 4:  STAIRS_DOWN
    @(22, 17), # 5:  POOL
    @(14, 26), # 6:  CHEST
    @(14, 28), # 7:  GOLD_PIECES
    @(16,  3), # 8:  FLARES
    @(22, 11), # 9:  WARP
    @(22,  2), # 10: SINKHOLE
    @( 0, 32), # 11: CRYSTAL_ORB
    @(18,  0), # 12: BOOK
    @( 1, 21), # 13: KOBOLD
    @( 1, 35), # 14: ORC
    @( 0, 22), # 15: KRESH
    @( 1,  5), # 16: GOBLIN
    @( 5,  5), # 17: OGRE
    @( 4, 12), # 18: TROLL
    @( 8, 16), # 19: MINOTAUR
    @( 9, 12), # 20: UR_VILE
    @( 1, 14), # 21: GARGOYLE
    @( 7, 33), # 22: CHIMERA
    @( 7, 17), # 23: BALROG
    @( 3, 27), # 24: DRAGON
    @(14, 34), # 25: VENDOR
    @(19, 29), # 26: RUBY_RED
    @(19, 30), # 27: NORN_STONE
    @(20,  1), # 28: PALE_PEARL
    @(16, 13), # 29: OPAL_EYE
    @(19, 33), # 30: GREEN_GEM
    @(19, 31), # 31: BLUE_FLAME
    @(20,  3), # 32: PALANTIR
    @(20,  0), # 33: SILMARIL
    @(19, 25), # 34: RUNESTAFF
    @( 0, 33), # 35: ORB_OF_ZOT
    @( 8, 39), # 36: HUMAN
    @( 6,  2), # 37: DWARF
    @( 1,  4), # 38: HOBBIT
    @( 8, 24)  # 39: ELF
)

function Convert-RgbToGrb332([int]$r, [int]$g, [int]$b) {
    $g3 = [int][Math]::Floor(($g * 7 + 127) / 255)
    $r3 = [int][Math]::Floor(($r * 7 + 127) / 255)
    $b2 = [int][Math]::Floor(($b * 3 + 127) / 255)
    return [byte](($g3 -shl 5) -bor ($r3 -shl 2) -bor $b2)
}

# Allocate 7 segments x 16,384 bytes = 114,688 bytes
$totalSize = 7 * 16384
$data = New-Object byte[] $totalSize

Write-Output "Extracting 40 sprites (32x32) and 40 map tiles (16x16) in SCREEN 8 GRB format..."
for ($idx = 0; $idx -lt 40; $idx++) {
    $row = $objectCoords[$idx][0]
    $col = $objectCoords[$idx][1]
    $srcX = $col * 32
    $srcY = $row * 32

    # 1. 32x32 Sprite in Seg 0..2 (offset = idx * 1024)
    $sprBase = $idx * 1024
    for ($y = 0; $y -lt 32; $y++) {
        for ($x = 0; $x -lt 32; $x++) {
            $c = $bmpTiles.GetPixel($srcX + $x, $srcY + $y)
            $data[$sprBase + ($y * 32) + $x] = Convert-RgbToGrb332 $c.R $c.G $c.B
        }
    }

    # 2. 16x16 Downscaled Map Tile in Seg 3 (base = 3 * 16384 + idx * 256)
    $tileBase = (3 * 16384) + ($idx * 256)
    for ($ty = 0; $ty -lt 16; $ty++) {
        for ($tx = 0; $tx -lt 16; $tx++) {
            if ($tx -eq 0 -or $ty -eq 0 -or $tx -eq 15 -or $ty -eq 15) {
                # Subtle dark stone grid frame for the 8x8 dungeon map
                if ($idx -eq 0) {
                    $data[$tileBase + ($ty * 16) + $tx] = 0x25 # Dark slate border for unseen room
                } else {
                    $data[$tileBase + ($ty * 16) + $tx] = 0x49 # Lit dungeon stone border
                }
            } else {
                $c0 = $bmpTiles.GetPixel($srcX + ($tx * 2),     $srcY + ($ty * 2))
                $c1 = $bmpTiles.GetPixel($srcX + ($tx * 2) + 1, $srcY + ($ty * 2))
                $c2 = $bmpTiles.GetPixel($srcX + ($tx * 2),     $srcY + ($ty * 2) + 1)
                $c3 = $bmpTiles.GetPixel($srcX + ($tx * 2) + 1, $srcY + ($ty * 2) + 1)
                $rAvg = [int](($c0.R + $c1.R + $c2.R + $c3.R) / 4)
                $gAvg = [int](($c0.G + $c1.G + $c2.G + $c3.G) / 4)
                $bAvg = [int](($c0.B + $c1.B + $c2.B + $c3.B) / 4)
                $data[$tileBase + ($ty * 16) + $tx] = Convert-RgbToGrb332 $rAvg $gAvg $bAvg
            }
        }
    }
}

# 3. Build 10 Treasure Icons (indices 26..35) in Seg 2:
#    - Owned (Gold border, full color) at Seg 2 + 0x2000 (10 x 256 = 2560 bytes)
#    - Unowned (Dark silhouette)       at Seg 2 + 0x2A00 (10 x 256 = 2560 bytes)
$ownedBase   = (2 * 16384) + 0x2000
$unownedBase = (2 * 16384) + 0x2A00
for ($t = 0; $t -lt 10; $t++) {
    $idx = 26 + $t
    $tileSrc = (3 * 16384) + ($idx * 256)
    for ($py = 0; $py -lt 16; $py++) {
        for ($px = 0; $px -lt 16; $px++) {
            $off = ($t * 256) + ($py * 16) + $px
            if ($px -eq 0 -or $py -eq 0 -or $px -eq 15 -or $py -eq 15) {
                $data[$ownedBase + $off]   = 0xFC # Bright Gold border when owned
                $data[$unownedBase + $off] = 0x24 # Dim Slate border when unowned
            } else {
                $pxVal = $data[$tileSrc + ($py * 16) + $px]
                $data[$ownedBase + $off] = $pxVal
                # Dim unowned silhouette
                $data[$unownedBase + $off] = [byte](($pxVal -shr 2) -band 0x24)
            }
        }
    }
}

# 4. Build 2 Dungeon 48x48 Viewport Backgrounds in Seg 3 + 0x2800 (2 x 2304 = 4608 bytes)
$vpBase = (3 * 16384) + 0x2800
for ($vy = 0; $vy -lt 48; $vy++) {
    for ($vx = 0; $vx -lt 48; $vx++) {
        $isBorder = ($vx -lt 2 -or $vy -lt 2 -or $vx -ge 46 -or $vy -ge 46)
        $isBrick  = (($vy % 8) -eq 0) -or ((($vx + (($vy / 8) * 8)) % 16) -eq 0)
        if ($isBorder) {
            $data[$vpBase + ($vy * 48) + $vx] = 0xFC # Gold frame
            $data[$vpBase + 2304 + ($vy * 48) + $vx] = 0x1C # Crimson combat frame
        } elseif ($isBrick) {
            $data[$vpBase + ($vy * 48) + $vx] = 0x24
            $data[$vpBase + 2304 + ($vy * 48) + $vx] = 0x08
        } else {
            $data[$vpBase + ($vy * 48) + $vx] = 0x49
            $data[$vpBase + 2304 + ($vy * 48) + $vx] = 0x29
        }
    }
}

# 5. Write WIZDATA1 Magic Signature & Metadata in Seg 2 + 0x3F00
$sigBase = (2 * 16384) + 0x3F00
$sigBytes = [System.Text.Encoding]::ASCII.GetBytes("WIZDATA1")
for ($i = 0; $i -lt $sigBytes.Length; $i++) {
    $data[$sigBase + $i] = $sigBytes[$i]
}
$data[$sigBase + 8]  = 1  # Format Major
$data[$sigBase + 9]  = 7  # Segment Count (7 x 16KB = 112KB)
$data[$sigBase + 10] = 40 # Sprite Count (40)

# 6. Convert the 3 panels of winwiz/images.png (TILE_COMBAT, TILE_VICTORY, TILE_DEATH)
#    into 160x100 SCREEN 12 YJK images in Segments 4, 5, and 6 (16,000 bytes each)
Write-Output "Encoding 3 panels from images.png into SCREEN 12 YJK (Segments 4, 5, 6)..."
for ($panel = 0; $panel -lt 3; $panel++) {
    $segBase = (4 + $panel) * 16384
    $rect = New-Object System.Drawing.Rectangle(($panel * 359), 0, 359, 362)

    $scaled = New-Object System.Drawing.Bitmap(160, 100)
    $g = [System.Drawing.Graphics]::FromImage($scaled)
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.DrawImage($bmpImages, (New-Object System.Drawing.Rectangle(0, 0, 160, 100)), $rect, [System.Drawing.GraphicsUnit]::Pixel)
    $g.Dispose()

    for ($y = 0; $y -lt 100; $y++) {
        for ($gx = 0; $gx -lt 160; $gx += 4) {
            $r = @(0,0,0,0); $gr = @(0,0,0,0); $b = @(0,0,0,0)
            $rSum = 0.0; $gSum = 0.0; $bSum = 0.0
            for ($i = 0; $i -lt 4; $i++) {
                $px = $scaled.GetPixel($gx + $i, $y)
                $r[$i]  = [int]($px.R -shr 3)
                $gr[$i] = [int]($px.G -shr 3)
                $b[$i]  = [int]($px.B -shr 3)
                $rSum += $r[$i]; $gSum += $gr[$i]; $bSum += $b[$i]
            }
            $rAvg = $rSum / 4.0
            $gAvg = $gSum / 4.0
            $bAvg = $bSum / 4.0
            $yAvg = (4.0 * $bAvg + 2.0 * $rAvg + $gAvg) / 8.0

            $J = [int][Math]::Round($rAvg - $yAvg)
            if ($J -lt -32) { $J = -32 } elseif ($J -gt 31) { $J = 31 }
            $K = [int][Math]::Round($gAvg - $yAvg)
            if ($K -lt -32) { $K = -32 } elseif ($K -gt 31) { $K = 31 }

            $uJ = $J -band 0x3F
            $uK = $K -band 0x3F
            $chromaBits = @(
                ($uK -band 0x07),
                (($uK -shr 3) -band 0x07),
                ($uJ -band 0x07),
                (($uJ -shr 3) -band 0x07)
            )

            for ($i = 0; $i -lt 4; $i++) {
                $yi = [int][Math]::Round((4.0 * $b[$i] + 2.0 * $r[$i] + $gr[$i]) / 8.0)
                if ($yi -lt 0) { $yi = 0 } elseif ($yi -gt 31) { $yi = 31 }
                $data[$segBase + ($y * 160) + $gx + $i] = [byte](($yi -shl 3) -bor $chromaBits[$i])
            }
        }
    }
    $scaled.Dispose()
}

$bmpTiles.Dispose()
$bmpImages.Dispose()

[System.IO.File]::WriteAllBytes($outPath, $data)
Write-Output ("Successfully generated {0} ({1} bytes = {2} KB, 7 segments)" -f $outPath, $data.Length, ($data.Length / 1024))
