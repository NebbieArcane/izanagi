#!/usr/bin/php
<?php
//echo $argc,"\n";
//print_r($argv);
$version=$argv[1];
$build=$argv[2];
exec('git log -n 1',$commitinfo,$trash);
$folders=glob("src/*");
//print_r($folders);
$index=[];
foreach ($folders as $folder) {
    $fname=basename($folder);
    if ($fname[0]=='_') continue;
    if ($fname=="chiusura") continue;
    $zonefile=glob("$folder/*.zon");
    if (is_array($zonefile)) {
        $zonefile=array_shift($zonefile);
        if ($zonefile) {
            list($zone,$body)=explode('~',file_get_contents($zonefile),2);
            list($lastnum,$_)=explode(' ',trim($body),2);
            list($zonenumber,$zonedesc)=preg_split("/[\n\r]+/",$zone);
            $index[$lastnum]=['name'=>$zonedesc,'base'=>$zonenumber,'fname'=>$fname];
        }
        else {
            echo "broken $folder";
            print_r(glob("$folder/*"));
        }
    }
}
ksort($index);
$arealist='*!Machine generated on ' . date('Y/m/d H:i:s')." **DO NOT EDIT**\n";
//print_r($index);
$start=0;

foreach ($index as $lastnum=>$data) {
    extract($data);
    $arealist.=sprintf("%5d:%5d:aree/%s:%s\n",$start,$lastnum,$fname,$name);
    $start=$lastnum+1;
}
file_put_contents('mudroot/aree.index', $arealist);
$index[-1]=['fname'=>'_head'];
$index[$start]=['fname'=>'_tail'];
ksort($index);
$exts=['.wld','.mob','.obj','.shp','.spe','.zon'];
$world=[];
foreach ($exts as $ext) {
    $world[$ext]='';
}
foreach($index as $data) {
    foreach ($exts as $ext) {
        $fname=sprintf('src/%1$s/%1$s%2$s',$data['fname'],$ext);
        if (@file_exists($fname)) {
            if ($ext==".wld") {
                $world[$ext].=rtrim(file_get_contents($fname))."\n";
            }
            else {
                $world[$ext].=file_get_contents($fname);
            }
            if ($data['fname']=="_head" and $ext==".wld") {
                $world[$ext]=str_replace('<VERSION>', "$version ($build)", $world[$ext]);
                $world[$ext]=str_replace('<LOGNOTES>', implode("\n",$commitinfo), $world[$ext]);
            }
        }
    }
}
define('ZIP',class_exists('ZipArchive'));
if (ZIP) {
    $zip=new ZipArchive();
    $zip->open("mudroot/myst.zip",ZipArchive::CREATE);
}
foreach ($world as $ext=>$data) {
    echo "Generating $fname\n";
    $fname="mudroot/myst$ext";
    file_put_contents($fname,$data);
    if (ZIP) $zip->addFile($fname);
}
if(ZIP) {
    $zip->close();
    echo "Created mudroot/myst.zip\n";
}
