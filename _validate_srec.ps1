[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Path
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

try {
    $resolvedPath = (Resolve-Path -LiteralPath $Path).Path
    $records = [Collections.Generic.List[object]]::new()
    [uint64]$totalDataBytes = 0
    $lineNumber = 0

    $addressBytesByType = @{
        '0' = 2
        '1' = 2
        '2' = 3
        '3' = 4
        '5' = 2
        '6' = 3
        '7' = 4
        '8' = 3
        '9' = 2
    }

    foreach ($sourceLine in [IO.File]::ReadLines($resolvedPath)) {
        $lineNumber++
        $line = $sourceLine.Trim()
        if ($line.Length -eq 0) {
            continue
        }
        if ($line -notmatch '^S[0-9][0-9A-Fa-f]+$' -or $line.Length -lt 4) {
            throw ('Line {0} is not a valid hexadecimal S-record.' -f $lineNumber)
        }

        $recordType = $line.Substring(1, 1)
        if (-not $addressBytesByType.ContainsKey($recordType)) {
            throw ('Line {0} uses unsupported S-record type S{1}.' -f $lineNumber, $recordType)
        }

        $count = [Convert]::ToByte($line.Substring(2, 2), 16)
        $expectedCharacters = 4 + (2 * $count)
        if ($line.Length -ne $expectedCharacters) {
            throw ('Line {0} has length {1}; its count byte requires {2} characters.' -f $lineNumber, $line.Length, $expectedCharacters)
        }

        $addressByteCount = [int]$addressBytesByType[$recordType]
        if ($count -lt ($addressByteCount + 1)) {
            throw ('Line {0} is too short for an S{1} address and checksum.' -f $lineNumber, $recordType)
        }

        $checksumSum = [int]$count
        for ($byteIndex = 0; $byteIndex -lt $count; $byteIndex++) {
            $checksumSum += [Convert]::ToByte($line.Substring(4 + (2 * $byteIndex), 2), 16)
        }
        if (($checksumSum -band 0xFF) -ne 0xFF) {
            throw ('Line {0} has an invalid S-record checksum.' -f $lineNumber)
        }

        [uint64]$address = 0
        for ($addressIndex = 0; $addressIndex -lt $addressByteCount; $addressIndex++) {
            $address = ($address -shl 8) -bor [Convert]::ToByte($line.Substring(4 + (2 * $addressIndex), 2), 16)
        }

        $dataLength = $count - $addressByteCount - 1
        if ($recordType -in @('1', '2', '3') -and $dataLength -gt 0) {
            [uint64]$endAddress = $address + $dataLength - 1
            if ($endAddress -gt [uint32]::MaxValue) {
                throw ('Line {0} extends beyond the S3 address space.' -f $lineNumber)
            }

            $records.Add([pscustomobject]@{
                Start  = $address
                End    = $endAddress
                Length = $dataLength
                Line   = $lineNumber
                Type   = $recordType
            })
            $totalDataBytes += $dataLength
        }
    }

    if ($records.Count -eq 0) {
        throw 'The file contains no S1, S2, or S3 data records.'
    }

    $sortedRecords = $records | Sort-Object -Property Start, End, Line
    $previous = $null
    foreach ($record in $sortedRecords) {
        if ($null -ne $previous -and $record.Start -le $previous.End) {
            [uint64]$overlapStart = $record.Start
            [uint64]$overlapEnd = [Math]::Min($record.End, $previous.End)
            throw ('Data overlap at ${0:X8}-${1:X8}: line {2} (${3:X8}-${4:X8}) conflicts with line {5} (${6:X8}-${7:X8}).' -f
                $overlapStart, $overlapEnd,
                $previous.Line, $previous.Start, $previous.End,
                $record.Line, $record.Start, $record.End)
        }

        if ($null -eq $previous -or $record.End -gt $previous.End) {
            $previous = $record
        }
    }

    $highestAddress = ($sortedRecords | Select-Object -Last 1).End
    Write-Host ('S-record validation passed: {0} data records, {1} data bytes, highest address ${2:X8}.' -f
        $records.Count, $totalDataBytes, $highestAddress)
    exit 0
}
catch {
    [Console]::Error.WriteLine(('ERROR: S-record validation failed: {0}' -f $_.Exception.Message))
    exit 1
}
