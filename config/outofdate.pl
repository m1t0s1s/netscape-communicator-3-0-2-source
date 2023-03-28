#!/usr/local/bin/perl

#Input: [-d dir] foo1.java foo2.java
#Compares with: foo1.class foo2.class (if -d specified, checks in 'dir', 
#  otherwise assumes .class files in same directory as .java files)
#Returns: list of input arguments which are newer than corresponding class
#files (non-existant class files are considered to be real old :-)


if ($ARGV[0] eq '-d') {
    $classdir = $ARGV[1];
    $classdir .= "/";
    shift;
    shift;
} else {
    $classdir = "./";
}

foreach $filename (@ARGV) {
    $classfilename = $classdir;
    $classfilename .= $filename;
    $classfilename =~ s/.java$/.class/;
    ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,$atime,$mtime,
     $ctime,$blksize,$blocks) = stat($filename);
    ($dev,$ino,$mode,$nlink,$uid,$gid,$rdev,$size,$atime,$classmtime,
     $ctime,$blksize,$blocks) = stat($classfilename);
    if ($mtime > $classmtime) {
        print $filename, " ";
    }
}

print "\n";
