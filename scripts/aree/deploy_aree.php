#!/usr/bin/php
<?php
/**
 * Build monolith myst.* from per-area src/<area>/<area>.{zon,wld,mob,...}
 *
 * Canonical copy for NebbieArcane/Aree — replace repo-root deploy_aree.php with this file.
 *
 * Fix: strip EOF terminators from each area chunk before concat. Per-area files or
 * Izanagi zone-pack exports may end with #$ / %% / #0; leaving them in the middle
 * truncates NebbieArcane server loading (boot stops at the first terminator).
 */

$version = $argv[1];
$build = $argv[2];
exec('git log -n 1', $commitinfo, $trash);
$folders = glob('src/*');
$index = [];
foreach ($folders as $folder) {
    $fname = basename($folder);
    if ($fname[0] === '_') {
        continue;
    }
    if ($fname === 'chiusura') {
        continue;
    }
    $zonefile = glob("$folder/*.zon");
    if (is_array($zonefile)) {
        $zonefile = array_shift($zonefile);
        if ($zonefile) {
            list($zone, $body) = explode('~', file_get_contents($zonefile), 2);
            list($lastnum, $_) = explode(' ', trim($body), 2);
            list($zonenumber, $zonedesc) = preg_split("/[\n\r]+/", $zone);
            $index[$lastnum] = ['name' => $zonedesc, 'base' => $zonenumber, 'fname' => $fname];
        } else {
            echo "broken $folder";
            print_r(glob("$folder/*"));
        }
    }
}
ksort($index);
$arealist = '*!Machine generated on ' . date('Y/m/d H:i:s') . " **DO NOT EDIT**\n";
$start = 0;

foreach ($index as $lastnum => $data) {
    extract($data);
    $arealist .= sprintf("%5d:%5d:aree/%s:%s\n", $start, $lastnum, $fname, $name);
    $start = $lastnum + 1;
}
file_put_contents('mudroot/aree.index', $arealist);
$index[-1] = ['fname' => '_head'];
$index[$start] = ['fname' => '_tail'];
ksort($index);
$exts = ['.wld', '.mob', '.obj', '.shp', '.spe', '.zon'];
$world = [];
foreach ($exts as $ext) {
    $world[$ext] = '';
}

/**
 * Remove server EOF markers from a single area file (not _tail).
 */
function strip_area_eof_markers(string $content, string $ext): string
{
    if ($content === '') {
        return '';
    }
    $lines = preg_split('/\r\n|\n|\r/', rtrim($content, "\r\n"));
    while (!empty($lines)) {
        $last = trim($lines[count($lines) - 1]);
        $is_eof = false;
        if ($ext === '.zon' && $last === '#$') {
            $is_eof = true;
        } elseif (($ext === '.mob' || $ext === '.obj') && ($last === '%%' || $last === '%%~')) {
            $is_eof = true;
        } elseif ($ext === '.wld' && $last === '#0') {
            $is_eof = true;
        }
        if (!$is_eof) {
            break;
        }
        array_pop($lines);
    }
    if (empty($lines)) {
        return '';
    }
    return implode("\n", $lines);
}

foreach ($index as $data) {
    $is_tail = ($data['fname'] === '_tail');
    foreach ($exts as $ext) {
        $fname = sprintf('src/%1$s/%1$s%2$s', $data['fname'], $ext);
        if (!@file_exists($fname)) {
            continue;
        }
        $chunk = file_get_contents($fname);
        if (!$is_tail) {
            $chunk = strip_area_eof_markers($chunk, $ext);
            if ($chunk === '') {
                continue;
            }
        }
        if ($data['fname'] === '_head' && $ext === '.wld') {
            $chunk = str_replace('<VERSION>', "$version ($build)", $chunk);
            $chunk = str_replace('<LOGNOTES>', implode("\n", $commitinfo), $chunk);
        }
        if ($ext === '.wld') {
            $world[$ext] .= rtrim($chunk) . "\n";
        } else {
            $world[$ext] .= rtrim($chunk) . "\n";
        }
    }
}

define('ZIP', class_exists('ZipArchive'));
if (ZIP) {
    $zip = new ZipArchive();
    $zip->open('mudroot/myst.zip', ZipArchive::CREATE);
}
foreach ($world as $ext => $data) {
    $out = "mudroot/myst$ext";
    echo "Generating $out\n";
    file_put_contents($out, $data);
    if (ZIP) {
        $zip->addFile($out);
    }
}
if (ZIP) {
    $zip->close();
    echo "Created mudroot/myst.zip\n";
}
