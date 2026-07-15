param(
    [string[]]$EdgeId = @(),
    [double]$LengthTolerance = 0.15,
    [int]$ClearancePx = 8,
    [switch]$PixelObstacleCheck,
    [switch]$SkipNodeCheck
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$dataDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$nodes = Import-Csv (Join-Path $dataDir "nodes.csv") -Encoding UTF8
$edges = Import-Csv (Join-Path $dataDir "edges_curved.csv") -Encoding UTF8
$geometry = Import-Csv (Join-Path $dataDir "edge_geometry_points.csv") -Encoding UTF8
$nodeById = @{}
$geometryByEdge = @{}
$warnings = [System.Collections.Generic.List[string]]::new()

foreach ($node in $nodes) {
    $nodeById[$node.id] = $node
}
foreach ($point in $geometry) {
    if (-not $geometryByEdge.ContainsKey($point.edge_id)) {
        $geometryByEdge[$point.edge_id] = [System.Collections.Generic.List[object]]::new()
    }
    $geometryByEdge[$point.edge_id].Add($point)
}

if ($EdgeId.Count -gt 0) {
    $selected = @{}
    foreach ($value in $EdgeId) {
        foreach ($id in $value.Split(",", [System.StringSplitOptions]::RemoveEmptyEntries)) {
            $selected[$id.Trim()] = $true
        }
    }
    $edges = @($edges | Where-Object { $selected.ContainsKey($_.id) })
}

Add-Type -AssemblyName System.Drawing
$mapPath = Join-Path $dataDir "campus_map_new.png"
$map = [System.Drawing.Bitmap]::new($mapPath)

function Get-Distance([double]$x1, [double]$y1, [double]$x2, [double]$y2) {
    $dx = $x2 - $x1
    $dy = $y2 - $y1
    return [Math]::Sqrt($dx * $dx + $dy * $dy)
}

function Test-WaterPixel([System.Drawing.Color]$color) {
    return $color.B -gt 205 -and $color.B -gt ($color.G + 20) -and
           $color.B -gt ($color.R + 55) -and $color.R -lt 145
}

function Test-BuildingPixel([System.Drawing.Color]$color) {
    $purple = $color.R -ge 125 -and $color.R -le 220 -and
              $color.G -ge 70 -and $color.G -le 175 -and
              $color.B -ge 120 -and $color.B -le 220 -and
              $color.R -gt ($color.G + 20) -and $color.B -gt ($color.G + 15)
    $orange = $color.R -ge 190 -and $color.G -ge 80 -and
              $color.G -le 190 -and $color.B -lt 155
    $yellow = $color.R -ge 190 -and $color.G -ge 185 -and $color.B -lt 135
    return $purple -or $orange -or $yellow
}

function Get-ObstacleAt([int]$x, [int]$y, [bool]$AllowWater) {
    if ($x -lt 0 -or $y -lt 0 -or $x -ge $map.Width -or $y -ge $map.Height) {
        return $null
    }
    $color = $map.GetPixel($x, $y)
    if (Test-BuildingPixel $color) {
        return "building"
    }
    if (-not $AllowWater -and (Test-WaterPixel $color)) {
        return "water"
    }
    return $null
}

function Test-SegmentObstacle($from, $to, [bool]$AllowWater) {
    $x1 = [double]$from.x
    $y1 = [double]$from.y
    $x2 = [double]$to.x
    $y2 = [double]$to.y
    $length = Get-Distance $x1 $y1 $x2 $y2
    $steps = [Math]::Max(1, [int][Math]::Ceiling($length))
    $normalX = if ($length -gt 0.0) { -($y2 - $y1) / $length } else { 0.0 }
    $normalY = if ($length -gt 0.0) { ($x2 - $x1) / $length } else { 0.0 }
    for ($i = 0; $i -le $steps; $i++) {
        $centerX = $x1 + ($x2 - $x1) * $i / $steps
        $centerY = $y1 + ($y2 - $y1) * $i / $steps
        for ($offset = -$ClearancePx; $offset -le $ClearancePx; $offset++) {
            $x = [int][Math]::Round($centerX + $normalX * $offset)
            $y = [int][Math]::Round($centerY + $normalY * $offset)
            $kind = Get-ObstacleAt $x $y $AllowWater
            if ($null -ne $kind) {
                return [PSCustomObject]@{ Kind = $kind; X = $x; Y = $y }
            }
        }
    }
    return $null
}

foreach ($edge in $edges) {
    $walkable = 0
    if ($null -eq $edge.PSObject.Properties['walkable'] -or
        -not [int]::TryParse([string]$edge.walkable, [ref]$walkable) -or
        ($walkable -ne 0 -and $walkable -ne 1)) {
        $warnings.Add("$($edge.id) has invalid walkable value '$($edge.walkable)'; expected 0 or 1")
        continue
    }
    if (-not $geometryByEdge.ContainsKey($edge.id)) {
        $warnings.Add("$($edge.id) has no geometry points")
        continue
    }
    if (-not $nodeById.ContainsKey($edge.source) -or
        -not $nodeById.ContainsKey($edge.target)) {
        $warnings.Add("$($edge.id) references an unknown node")
        continue
    }

    $points = @($geometryByEdge[$edge.id] | Sort-Object { [int]$_.point_order })
    $expectedCount = [int]$edge.geometry_point_count
    if ($points.Count -ne $expectedCount) {
        $warnings.Add("$($edge.id) geometry count is $($points.Count), expected $expectedCount")
    }

    $source = $nodeById[$edge.source]
    $target = $nodeById[$edge.target]
    $first = $points[0]
    $last = $points[$points.Count - 1]
    if ([Math]::Abs([double]$first.x - [double]$source.x) -gt 0.001 -or
        [Math]::Abs([double]$first.y - [double]$source.y) -gt 0.001) {
        $warnings.Add("$($edge.id) geometry does not start at $($edge.source)")
    }
    if ([Math]::Abs([double]$last.x - [double]$target.x) -gt 0.001 -or
        [Math]::Abs([double]$last.y - [double]$target.y) -gt 0.001) {
        $warnings.Add("$($edge.id) geometry does not end at $($edge.target)")
    }

    $curvedLength = 0.0
    $obstacleHit = $null
    $allowWater = $edge.kind -eq "bridge"
    for ($i = 0; $i + 1 -lt $points.Count; $i++) {
        foreach ($point in @($points[$i], $points[$i + 1])) {
            if ([double]$point.x -lt 0 -or [double]$point.y -lt 0 -or
                [double]$point.x -ge $map.Width -or [double]$point.y -ge $map.Height) {
                $warnings.Add("$($edge.id) has a geometry point outside the map")
                break
            }
        }
        $curvedLength += Get-Distance ([double]$points[$i].x) ([double]$points[$i].y) `
                                      ([double]$points[$i + 1].x) ([double]$points[$i + 1].y)
        if ($PixelObstacleCheck -and $walkable -eq 1 -and $null -eq $obstacleHit) {
            $obstacleHit = Test-SegmentObstacle $points[$i] $points[$i + 1] $allowWater
        }
    }
    if ([Math]::Abs($curvedLength - [double]$edge.distance_px_curved) -gt $LengthTolerance) {
        $warnings.Add(("{0} curved length is {1:N2}px, CSV says {2:N2}px" -f
                      $edge.id, $curvedLength, [double]$edge.distance_px_curved))
    }
    if ([Math]::Abs($curvedLength * 0.8 - [double]$edge.distance_m_est_curved) -gt 0.15) {
        $warnings.Add(("{0} curved metric length is {1:N1}m, CSV says {2:N1}m" -f
                      $edge.id, ($curvedLength * 0.8), [double]$edge.distance_m_est_curved))
    }
    if ($null -ne $obstacleHit) {
        $warnings.Add("$($edge.id) violates $ClearancePx px $($obstacleHit.Kind) clearance near $($obstacleHit.X),$($obstacleHit.Y)")
    }
}

if ($PixelObstacleCheck -and -not $SkipNodeCheck) {
    foreach ($node in $nodes) {
        $x = [int][Math]::Round([double]$node.x)
        $y = [int][Math]::Round([double]$node.y)
        if ($x -ge 0 -and $y -ge 0 -and $x -lt $map.Width -and $y -lt $map.Height -and
            (Test-BuildingPixel $map.GetPixel($x, $y))) {
            $warnings.Add("node $($node.id) appears to be inside a building at $x,$y")
        }
    }
}

$map.Dispose()

if ($warnings.Count -eq 0) {
    Write-Host "OK: checked $($edges.Count) edges; no data warnings found."
    exit 0
}

foreach ($warning in $warnings) {
    Write-Warning $warning
}
Write-Host "Found $($warnings.Count) warning(s) while checking $($edges.Count) edge(s)."
exit 1
