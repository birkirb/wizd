#!/usr/local/bin/perl

$title_info="./dvd_titleinfo";
$dvd_path="/dev/dvd";

$genlist="genlist.cgi";
$buf = $ENV{'SCRIPT_NAME'};
$buf =~ s/\/[^\/]+$/\//;
$dvdplay="http://" . $ENV{'HTTP_HOST'} . $buf . "dvdplay.cgi";


open(IN, "$title_info $dvd_path|") || die;

print "Content-type: text/html\n";
print "\n";

$n = 0;
while (<IN>) {
	if (/chapters/) {
		s/^.*has ([0-9]+) chapters.*$/\1/;
		s/[\r|\n]//;
		$n++;
		print "<A HREF=\"$genlist?title=$n&chapters=$_&cgipath=$dvdplay\" vod=\"playlist\">Title $n ($_ chapters)</A><BR>\n";
	}
}
close(IN);

if ($n == 0) {
	print "No title...\n"
}
